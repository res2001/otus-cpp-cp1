#include "fwd.h"
#include "hash.h"
#include "DuplicateScanDescription.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

using RegexContainer = std::vector<boost::u32regex>;

struct FileDescriptionTemp {
    std::ifstream stream;
    HashBlockContainer block_hashes;
    bool is_fully_read;

    FileDescriptionTemp(const std::string& file_path)
    : stream(file_path, std::ios::binary), block_hashes(), is_fully_read(false) {}
};

#if defined(DEBUG_LOG_ENABLE)
    std::ostream& dlog = std::cout;
#else
    NullStream dlog;
#endif

static std::vector<char> gbuffer;

/* Функция конвертации маски (* и ?) в регулярное выражение (работает с UTF-8 строками) */
static boost::u32regex wildcard_to_regex(const std::string& wildcard)
{
    std::string regex_str;
    regex_str.reserve(wildcard.size());
    
    for (char c : wildcard) {
        switch (c) {
            // Экранируем спецсимволы регулярных выражений
            case '.': regex_str += "\\."; break;
            case '\\': regex_str += "\\\\"; break;
            case '^': regex_str += "\\^"; break;
            case '$': regex_str += "\\$"; break;
            case '+': regex_str += "\\+"; break;
            case '(': regex_str += "\\("; break;
            case ')': regex_str += "\\)"; break;
            case '[': regex_str += "\\["; break;
            case ']': regex_str += "\\]"; break;
            case '{': regex_str += "\\{"; break;
            case '}': regex_str += "\\}"; break;
            case '|': regex_str += "\\|"; break;
            
            // Заменяем символы маски ОС
            case '*': regex_str += ".*"; break;  // * -> ноль или более любых символов
            case '?': regex_str += "."; break;   // ? -> ровно один любой символ
            
            default: regex_str += c; break;
        }
    }
    
    // Создаем UTF-8 совместимое регулярное выражение с флагом регистронезависимости
    // boost::make_u32regex компилирует UTF-8 строку в ICU-совместимое выражение
    dlog << "regex: " << regex_str << std::endl;
    return boost::make_u32regex(regex_str, boost::regex_constants::ECMAScript | boost::regex_constants::icase);
}

static RegexContainer make_mask_regex(const std::vector<std::string>& wildcards)
{
    RegexContainer mask_regex;
    mask_regex.reserve(wildcards.size());
    for (const auto& wildcard: wildcards) {
        mask_regex.push_back(wildcard_to_regex(wildcard));
    }
    return mask_regex;
}

static HashBlockType create_block_hash(const char* data, size_t size,  HashAlgoEnum algo)
{
    assert(data && size > 0 && algo != HASH_ALGO_NONE);
    if (algo == HASH_ALGO_MD5) {
        md5::md5 hasher;
        hasher.process_bytes(data, size);
        md5::md5::digest_type digest;
        hasher.get_digest(digest);
        return std::make_unique<Md5Hash>(digest);
    } else {
        boost::crc_32_type hasher;
        hasher.process_bytes(data, size);
        return std::make_unique<Crc32Hash>(hasher.checksum());
    }
}

/* Return first - fash; second - read_bytes */
static std::pair<HashBlockType, size_t> calculate_next_block_hash(
    std::ifstream& file,
    const HashAlgoEnum algo)
{
    assert(algo != HASH_ALGO_NONE);
 
    file.read(gbuffer.data(), gbuffer.size());
    size_t read_bytes = file.gcount();

    if (read_bytes > 0)
        return { create_block_hash(gbuffer.data(), read_bytes, algo), read_bytes };

    return {nullptr, 0};
}

static bool compare_two_block(
    const ScanDescription& sd, 
    const size_t block_idx, 
    const HashBlockType& block_hash1, bool is_fully_read1, 
    FileDescriptionTemp& fdt2)
{
    if (block_idx < fdt2.block_hashes.size()) {
        return *fdt2.block_hashes[block_idx] == *block_hash1;

    } else {
        if (!fdt2.stream.good()) {
            fdt2.is_fully_read = true;
            return false;
        }

        auto p2 = calculate_next_block_hash(fdt2.stream, sd.param.hash_algo);
        if (p2.second < sd.param.block_size || !fdt2.stream.good()) {
            fdt2.is_fully_read = true;
            if (p2.second == 0)
                return block_hash1.get() == nullptr;
        }

        fdt2.block_hashes.push_back(std::move(p2.first));

        if (    (fdt2.is_fully_read && !is_fully_read1)
            ||  *fdt2.block_hashes.back() != *block_hash1)
        {
            return false;
        }
    }

    return true;
}

static bool compare_two_files(
    const ScanDescription& sd,
    FileDescription& fd1,
    FileDescriptionTemp& fdt2)
{
    assert(gbuffer.size() == sd.param.block_size && !fd1.dublicate.empty());
    size_t block_idx = 0;

    for (; block_idx < fd1.block_hashes.size(); ++block_idx) {
        if (!compare_two_block(sd, block_idx, fd1.block_hashes[block_idx], fd1.is_fully_read, fdt2))
            return false;
    }

    if(fd1.is_fully_read)
        return fdt2.is_fully_read || compare_two_block(sd, block_idx, HashBlockType() , fd1.is_fully_read, fdt2);

    std::ifstream fd1_stream(sd.GetFullPath(fd1.dublicate.front()), std::ios::binary);
    fd1_stream.seekg(gbuffer.size() * block_idx);

    for (; fd1_stream.good(); ++block_idx) {
        auto p1 = calculate_next_block_hash(fd1_stream, sd.param.hash_algo);
        if (p1.second < sd.param.block_size || !fd1_stream.good()) {
            fd1.is_fully_read = true;
        }

        if (p1.second > 0) {
            fd1.block_hashes.push_back(std::move(p1.first));
            if (!compare_two_block(sd, block_idx, fd1.block_hashes.back(), fd1.is_fully_read, fdt2))
                return false;
        } else {
            if (!compare_two_block(sd, block_idx, p1.first, fd1.is_fully_read, fdt2))
                return false;
        }
    }

    fd1.is_fully_read = true;
    return fdt2.is_fully_read;
}

static void scan_directory(
    ScanDescription& sd, 
    RegexContainer& mask_regex,
    RegexContainer& exclude_regex,
    const fs::path& scan_path,
    size_t base_dir_index)
{
    static size_t count = 0;
    assert(fs::is_directory(scan_path));
    std::error_code ec;
    auto it = fs::recursive_directory_iterator(scan_path);
    auto end = fs::recursive_directory_iterator();

    // Обход каталога и вложенных подкаталогов до заданной глубины (уровня)
    while (it != end)
    {
        std::string full_path = it->path().u8string();
        if (sd.param.scan_depth > 0 && it.depth() > static_cast<int>(sd.param.scan_depth - 1)) {
            dlog << "  Skip depth: \"" << full_path << "\"\n";
            it.pop(ec); 
            if (!ec)
                continue;

            std::cerr << ec.message() << std::endl;
            std::cerr << "Abort scan directory: \"" << scan_path.u8string() << "\"\n";
            return;
        }

        do {

            if (!fs::is_regular_file(it->path(), ec))
                break;

            const uintmax_t file_size = fs::file_size(*it, ec);
            if (ec)
                break;

            if (file_size == 0) {
                dlog << "  Skip empty file: \"" << full_path << "\"\n";
                break;
            }

            // 1. Вычисляем относительный путь относительно базового пути
            fs::path relative_path = fs::relative(it->path(), scan_path, ec);
            if (ec)
                break;

            const std::string relative = relative_path.u8string();
            if (exclude_regex.size() > 0) {
                if (!std::none_of(exclude_regex.begin(), exclude_regex.end(), [&relative](auto& mask){ return boost::u32regex_match(relative, mask); })) {
                    dlog << "  Skip exclude mask: \"" << full_path << "\"" << std::endl;
                    break;
                }
            }

            if (!std::any_of(mask_regex.begin(), mask_regex.end(), [&relative](auto& mask){ return boost::u32regex_match(relative, mask); })) {
                dlog << "  Skip filter mask: \"" << full_path << "\"" << std::endl;
                break;
            }

            // 2. Выделяем промежуточный путь и имя файла
            std::string subdir = relative_path.parent_path().u8string();
            std::string filename = it->path().filename().u8string();
            dlog << "  [" << ++count << "]: \"" << full_path << "\" size: " << file_size << std::endl;

            // 3. Регистрирует промежуточный каталог в мапе (если его там нет) и возвращает итератор
            const SubDirectoryDescription& subdesc = sd.AddSubdir(base_dir_index, std::move(subdir));
            // 4. Создаем временный описатель файла
            // SameFileDescription sfd2(subdesc, std::move(filename));
            FileDescriptionTemp fdt2(full_path);

            bool is_same = false;
            auto file_size_range = sd.files.equal_range(file_size);
            for (auto it = file_size_range.first; it != file_size_range.second; ++it) {
                FileDescription& fd = it->second;
                if (compare_two_files(sd, fd, fdt2)) {
                    fd.dublicate.emplace_back(subdesc, std::move(filename));
                    sd.double_files.emplace(& fd);
                    is_same = true;
                    break;
                }
            }

            if (!is_same) {
                auto it = sd.files.emplace(std::make_pair(file_size, SameFileDescription(subdesc, std::move(filename))));
                FileDescription& fd2 = it->second;
                fd2.block_hashes = std::move(fdt2.block_hashes);
                fd2.is_fully_read = fdt2.is_fully_read;
            }

        } while(false);

        if (ec) {
            std::cerr << ec.message() << ". Skip: \"" << it->path().u8string() << "\"\n";
            ec.clear();
        }

        it.increment(ec);
        if (ec) {
            std::cerr << ec.message() << std::endl;
            return;
        }
    }
}

static void print_dublicate_file(const ScanDescription& sd)
{
    if (sd.double_files.empty()) {
        std::cout << "No dublicate files.\n";
        return;
    }

    std::cout << "Dublicate files:\n";
    for (const auto fdit: sd.double_files) {
        assert(fdit->is_fully_read && fdit->block_hashes.size() > 0);

        for (const auto& dit: fdit->dublicate) {
            std::cout << " " << sd.GetFullPath(dit) << std::endl;
        }
        std::cout << std::endl;
    }
}

void scan_dir(const ParametersType& param)
{
    try {
        std::error_code ec;
        ScanDescription sd(param);
        RegexContainer mask_regex = make_mask_regex(param.mask_file);
        RegexContainer exclude_regex;

        if (param.exclude.size() > 0)
            exclude_regex = make_mask_regex(param.exclude);

        gbuffer.assign(param.block_size, 0);

        // Цикл по каталогам
        for (size_t i = 0; i < param.scan_dir.size(); ++i) {
            fs::path scan_path(param.scan_dir[i]);
            dlog << "Scan directory: \"" << scan_path.u8string() << "\"" << std::endl;

            if (fs::is_directory(scan_path, ec))
                scan_directory(sd, mask_regex, exclude_regex, scan_path, i);

            if (ec) {
                std::cerr << ec.message() << std::endl;
                std::cerr << "Abort scan directory: \"" << scan_path.u8string() << "\"\n";
                ec.clear();
                break;
            }
        }

        print_dublicate_file(sd);
    // } catch (const std::filesystem::filesystem_error& e) {
    //     std::cerr << e.what() << std::endl;
    //     return;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return;
    }
 }
