#pragma once
#include "async.h"
#include <string>
#include <sstream>
#include <string_view>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cassert>

bool is_equal_case_insensitive(std::string_view s1, std::string_view s2);

struct CmdLineParameters {
    static constexpr const uint32_t number_of_command_min = 1;
    static constexpr const uint32_t number_of_command_max = 1024;
    static constexpr const uint32_t number_of_command_default = 3;
    static constexpr const uint32_t cmd_len_max_min = 1;
    static constexpr const uint32_t cmd_len_max_max = 4096;
    static constexpr const uint32_t cmd_len_max_default = 1024;
    static constexpr const uint32_t thread_num_min = 1;
    static constexpr const uint32_t thread_num_max = 16;
    static constexpr const uint32_t thread_num_default = 2;
    static constexpr const char* file_default=ASYNC_STDIN_NAME;

    uint32_t number_of_command = number_of_command_default;
    uint32_t cmd_len_max = cmd_len_max_default;
    uint32_t thread_num = thread_num_default;
    std::vector<std::string> files{file_default};

    static bool IsValidNrOfCmd(uint32_t ncmd) noexcept { return number_of_command_min <= ncmd && ncmd <= number_of_command_max; }
    static bool IsValidCmdLenMax(uint32_t cmd_len) noexcept { return cmd_len_max_min <= cmd_len && cmd_len <= cmd_len_max_max; }
    static bool IsValidThreadNum(uint32_t num) noexcept { return thread_num_min <= num && num <= thread_num_max; }
    static bool IsValidFiles(std::vector<std::string>& files) noexcept
    {
        const size_t files_size = files.size();
        if (files_size == 0)
            return false;

        if (files_size == 1)
            return true;

#ifdef _WIN32
        std::sort(files.begin(), files.end(), 
            [](const std::string& a, const std::string& b) {
                return std::lexicographical_compare(
                    a.begin(), a.end(),
                    b.begin(), b.end(),
                    [](unsigned char c1, unsigned char c2) { return std::tolower(c1) < std::tolower(c2); }
                );
            });
#else
        std::sort(files.begin(), files.end()); 
#endif

        for (size_t i = 1; i < files_size; ++i) {
            if (is_equal_case_insensitive(files[i - 1], files[i]))
                return false;
        }
        return true;
    }
};

extern CmdLineParameters params;
bool parse_args(int argc, const char* argv[]);