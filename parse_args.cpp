#include "fwd.h"
#include <boost/program_options.hpp>
// #include <filesystem>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

// namespace fs = std::filesystem;
namespace bpo = boost::program_options;

using ParamArrayType = std::vector<std::string>;

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
    ss << "WideCharToMultiByte() is failed\n";
    throw std::invalid_argument(ss.str());
}
#endif

static ParamArrayType get_command_line_strings(int argc, const char* argv[])
{
    ParamArrayType result;

#ifdef _WIN32
    (void)argc; (void)argv;
    int win_argc;
    LPWSTR* win_argv = CommandLineToArgvW(GetCommandLineW(), &win_argc);
    
    if (win_argv != nullptr) {
        assert(win_argc == argc);
        for (int i = 0; i < win_argc; ++i) {
            result.push_back(win_wchar_to_utf8(win_argv[i]));
        }
        LocalFree(win_argv);
    } else {
        std::stringstream ss;
        ss << "CommandLineToArgvW() is failed\n"
        throw std::runtime_error(ss.str());
    }
#else
    for (int i = 0; i < argc; ++i) {
        result.push_back(argv[i]);
    }
#endif

    return result;
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

bool parse_args(int argc, const char* argv[])
{
    ParamArrayType varg = get_command_line_strings(argc, argv);
    bpo::options_description desc{"Options"};
    bpo::variables_map vm;

    try {
        desc.add_options()
        ("help,h", "Help screen")
        ("ncmd,n", bpo::value<uint32_t>()->default_value(params.number_of_command), "Number of commands for batch processing")
        ("file,f", bpo::value<std::vector<std::string>>()->default_value(params.files, "stdin")/*->implicit_value(params.files, params.files[0])*/, 
            "File for reading commands.\nMay be specified more than once.\nIf no value is specified, stdin is used.\nEach file can be specified only once.")
        ("cmd_max_len,c", bpo::value<uint32_t>()->default_value(params.cmd_len_max), "Maximum command length in bytes")
        ("thread_num", bpo::value<uint32_t>()->default_value(params.thread_num), "Number of thread for save files");

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

    if (vm.count("ncmd")) {
        params.number_of_command = vm["ncmd"].as<uint32_t>();
        if (!CmdLineParameters::IsValidNrOfCmd(params.number_of_command))
            throw_invalid_argument("ncmd");
    }
    if (vm.count("cmd_max_len")) {
        params.cmd_len_max = vm["cmd_max_len"].as<uint32_t>();
        if (!CmdLineParameters::IsValidCmdLenMax(params.cmd_len_max))
            throw_invalid_argument("cmd_max_len");
    }
    if (vm.count("thread_num")) {
        params.thread_num = vm["thread_num"].as<uint32_t>();
        if (!CmdLineParameters::IsValidThreadNum(params.thread_num))
            throw_invalid_argument("thread_num");
    }
    if (vm.count("file")) {
        // std::cout << "File count: " << vm.count("file") << std::endl;
        // auto& v = vm["file"].as<std::vector<std::string>>();
        // for (auto& s: v)
        //     std::cout << s << std::endl;

        params.files = std::move(vm["file"].as<std::vector<std::string>>());
        if (!CmdLineParameters::IsValidFiles(params.files))
            throw_invalid_argument("file");
    }

    return true;
}