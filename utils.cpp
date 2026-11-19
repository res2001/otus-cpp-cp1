#include "fwd.h"
#include <algorithm>
#include <cwctype>

bool is_equal_case_insensitive(std::string_view s1, std::string_view s2)
{
    if (s1.size() != s2.size())
        return false;

    return std::equal(s1.begin(), s1.end(), s2.begin(), 
        [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}
