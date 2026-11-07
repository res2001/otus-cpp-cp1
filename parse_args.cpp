#include "fwd.h"
#include <boost/program_options.hpp>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace fs = std::filesystem;
namespace bpo = boost::program_options;

#ifdef _WIN32
static std::string win_wchar_to_utf8(const wchar_t* wstr)
{
    do {
        if (!wstr || *wstr == L'\0') return "";

        const int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        if (size_needed <= 0) 
            break;

        std::string result(size_needed - 1, 0);

        if (WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), size_needed, nullptr, nullptr) == 0)
            break;
        
        return result;
    } while(false);

    std::stringstream ss;
    ss << "WideCharToMultiByte() is failed\n"
    throw std::invalid_argument(ss.str());
}
#endif

using param_array_type = std::vector<std::string>;
static param_array_type get_command_line_strings(int argc, const char* argv[])
{
    param_array_type result_strings;

#ifdef _WIN32
    (void)argc; (void)argv;
    int win_argc;
    LPWSTR* win_argv = CommandLineToArgvW(GetCommandLineW(), &win_argc);
    
    if (win_argv != nullptr) {
        assert(win_argc == argc);
        for (int i = 0; i < win_argc; ++i) {
            result_strings.push_back(win_utf16_to_utf8(win_argv[i]));
        }
        LocalFree(win_argv);
    } else {
        std::stringstream ss;
        ss << "CommandLineToArgvW() is failed\n"
        throw std::runtime_error(ss.str());
    }
#else
    for (int i = 0; i < argc; ++i) {
        result_strings.push_back(argv[i]);
    }
#endif

    return result_strings;
}

static void throw_invalid_argument(const std::string_view& arg_name)
{
    std::stringstream ss;
    ss << arg_name << " option is wrong\n";
    throw std::invalid_argument(ss.str());
}

template<typename CheckCallableType>
static void check_vector_string_arg(const std::vector<std::string>& vec, CheckCallableType check, const std::string_view& name)
{
    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
        if (!check(it->c_str()))
            throw_invalid_argument(name);
    }
}

static bool normalize_base_directories(std::vector<std::string>& scan_dir) {
    assert(scan_dir.size() > 0);
    std::error_code ec;

    // Делаем пути абсолютными и проверяем на существование
    auto remover = [](std::string& dir) {
        std::error_code ec;
        fs::path p = fs::canonical(dir, ec);
        if (!ec && fs::is_directory(p)) {
            dir = p.u8string();
            return false;
        }
        std::cerr << "Directory not found or not access. Skip scan directory: " << dir << std::endl;
        return true;
    };
    scan_dir.erase(std::remove_if(scan_dir.begin(), scan_dir.end(), remover), scan_dir.end());    

    if (auto s = scan_dir.size(); s <= 1) 
        return s == 1;

    // Сортируем пути по длине строки. Короткие пути окажутся в начале.
    std::sort(scan_dir.begin(), scan_dir.end(), 
        [](const std::string& a, const std::string& b) { return a.size() < b.size(); });

    // Отсекаем вложенные каталоги
    auto cur = scan_dir.cbegin() + 1;
    while (cur != scan_dir.cend()) {
        bool is_nested = false;
        
        for (auto master = scan_dir.cbegin(); master != cur; ++master) {
            // Проверяем, начинается ли текущий путь (cur) с одного из уже утвержденных путей
            auto [m_mismatch, c_mismatch] = std::mismatch(master->cbegin(), master->cend(), cur->cbegin(), cur->cend());
            
            // Если master полностью совпал с началом current, значит current вложен в master
            if (m_mismatch == master->cend()) {
                std::cerr << "Skip nested directory: " << *cur << std::endl;
                scan_dir.erase(cur);
                is_nested = true;
                break;
            }
        }

        if (!is_nested)
            ++cur;
    }

    return true;
}

bool parse_args(int argc, const char* argv[], ParametersType& param)
{
    param_array_type varg = get_command_line_strings(argc, argv);
    bpo::options_description desc{"Options"};
    bpo::variables_map vm;

    try {
        desc.add_options()
        ("help,h", "Help screen")
        ("scan_dir,d", bpo::value<std::vector<std::string>>()->default_value(param.scan_dir, param.scan_dir[0]), "Scan directory\nMay be specified more than once")
        ("mask", bpo::value<std::vector<std::string>>()->default_value(param.mask_file, param.mask_file[0]), "Subpath mask.\nMay be specified more than once")
        ("exclude,e", bpo::value<std::vector<std::string>>(), "Subpath exclude mask\nMay be specified more than once")
        ("hash,H", bpo::value<std::string>()->default_value(param.hash_to_string()), "Hash algorithm\nAllowed value: md5 or crc32")
        ("depth,l", bpo::value<uint32_t>()->default_value(param.scan_depth), "Scan depth.\n\t0 - Scan without restrictions.")
        ("min_file_size,s", bpo::value<size_t>()->default_value(param.min_file_size), "Minimum file size for scanning\nThe value must be greater than 0")
        ("block_size,b", bpo::value<size_t>()->default_value(param.block_size), (std::string("Read block size\nAllowed range: [") + std::to_string(ParametersType::block_size_min) + 
            "; " + std::to_string(ParametersType::block_size_max) + "]").c_str());

        bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
        bpo::notify(vm);

        if (vm.count("help")) {
            std::cerr << desc << '\n';
            return false;
        }
    } catch (const bpo::error &e) {
        std::cerr << e.what() << '\n';
        return false;
    }    


    if (vm.count("scan_dir")) {
        param.scan_dir = std::move(vm["scan_dir"].as<std::vector<std::string>>());
        check_vector_string_arg(param.scan_dir, ParametersType::isValidScanDir, "scan_dir");
        if (!normalize_base_directories(param.scan_dir)) {
            throw std::invalid_argument("scan_dir option is empty\n");
        }
        // std::for_each(param.scan_dir.cbegin(), param.scan_dir.cend(), [](const std::string& dir){ std::cout << dir << std::endl; });
    }
    if (vm.count("mask")) {
        param.mask_file = std::move(vm["mask"].as<std::vector<std::string>>());
        check_vector_string_arg(param.mask_file, ParametersType::isValidMaskFile, "mask");
    }
    if (vm.count("exclude")) {
        param.exclude = std::move(vm["exclude"].as<std::vector<std::string>>());
        check_vector_string_arg(param.exclude, ParametersType::isValidExcludeDir, "exclude");
    }
    if (vm.count("hash")) {
        param.hash_algo = param.string_to_hash(vm["hash"].as<std::string>().c_str());
        if (!ParametersType::isValidHash(param.hash_algo))
            throw_invalid_argument("hash");
    }
    if (vm.count("min_file_size")) {
        param.min_file_size = vm["min_file_size"].as<size_t>();
        if (!ParametersType::isValidMinFileSize(param.min_file_size))
            throw_invalid_argument("min_file_size");
    }
    if (vm.count("block_size")) {
        param.block_size = vm["block_size"].as<size_t>();
        if (!ParametersType::isValidBlockSize(param.block_size))
            throw_invalid_argument("block_size");
    }
    if (vm.count("depth")) {
        param.scan_depth = vm["depth"].as<uint32_t>();
        if (!ParametersType::isValidDepth(param.scan_depth))
            throw_invalid_argument("depth");
    }

    return true;
}
