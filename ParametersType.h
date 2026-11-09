#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <cstdint>

/* utils */
bool is_equal_case_insensitive(std::string_view s1, std::string_view s2);

enum HashAlgoEnum {
    HASH_ALGO_NONE = 0,
    HASH_ALGO_CRC32,
    HASH_ALGO_MD5,
};

struct ParametersType {
    constexpr static const char* scan_dir_default = ".";
    constexpr static const char* mask_file_default = "*";
    constexpr static const char* hash_name[] = {"none", "crc32", "md5"};
    constexpr static HashAlgoEnum hash_algo_default = HASH_ALGO_CRC32;
    constexpr static size_t min_file_size_default = 0;
    constexpr static size_t block_size_min = 1;
    constexpr static size_t block_size_max = 1024 * 1024;
    constexpr static size_t block_size_default = 512;
    constexpr static uint32_t scan_depth_default = 1;
    constexpr static uint32_t scan_depth_max = 1024;

    std::vector<std::string> scan_dir = {scan_dir_default};
    std::vector<std::string> mask_file = {mask_file_default};
    std::vector<std::string> exclude = {};
    HashAlgoEnum hash_algo = hash_algo_default;
    size_t min_file_size = min_file_size_default;
    size_t block_size = block_size_default;
    uint32_t scan_depth = scan_depth_default;

    static bool isValidScanDir(const std::string& dir) noexcept { return dir.size() != 0; }
    static bool isValidExcludeDir(const std::string& dir) noexcept { return dir.size() != 0; }
    static bool isValidMaskFile(const std::string& mask) noexcept { return mask.size() != 0; }
    static bool isValidHash(HashAlgoEnum hash) noexcept { return hash != HASH_ALGO_NONE; }
    static bool isValidMinFileSize(size_t /*min_file_size*/) noexcept { return true; }
    static bool isValidBlockSize(size_t block_size) noexcept { return block_size >= block_size_min && block_size <= block_size_max; }
    static bool isValidDepth(uint32_t scan_depth) noexcept { return scan_depth <= scan_depth_max; }

    static HashAlgoEnum string_to_hash(const std::string_view& hash_algo_arg) noexcept
    {
        for (uint32_t i = 1; i < std::size(hash_name); ++i) {
            if (is_equal_case_insensitive(hash_name[i], hash_algo_arg))
                return static_cast<HashAlgoEnum>(i);
        }
        return HASH_ALGO_NONE;
    }
    std::string hash_to_string() const { return hash_name[hash_algo]; }

    bool isValid() const noexcept
    {
        if (scan_dir.size() == 0 || mask_file.size() == 0)
            return false;

        if (!std::all_of(scan_dir.cbegin(), scan_dir.cend(), ParametersType::isValidScanDir))
            return false;
        if (!std::all_of(mask_file.cbegin(), mask_file.cend(), ParametersType::isValidMaskFile))
            return false;
        if (exclude.size() > 0 && !std::all_of(exclude.cbegin(), exclude.cend(), ParametersType::isValidExcludeDir))
            return false;

        return  isValidHash(hash_algo)
            &&  isValidMinFileSize(min_file_size)
            &&  isValidBlockSize(block_size)
            &&  isValidDepth(scan_depth);
    }
};
