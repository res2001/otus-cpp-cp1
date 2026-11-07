#pragma once
#include "ParametersType.h"
#include <memory>
#include <iostream>
#include <sstream>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <cassert>

struct NullStream {
    template <typename T>
    NullStream& operator<<(const T&) { return *this; }
    // NullStream& operator<<(std::ostream& (*)(std::ostream&)) { return *this; }
};

#if !defined(NDEBUG)
#define DEBUG_LOG_ENABLE
#endif

#if defined(DEBUG_LOG_ENABLE)
extern std::ostream& dlog;
#else
extern NullStream dlog;
#endif

bool parse_args(int argc, const char* argv[], ParametersType& param);
void scan_dir(const ParametersType& param);