#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <exception>
#include <sstream>
#include <utility>
#include <type_traits>
#include <limits.h>

constexpr size_t IPV4_ADDRESS_LENGTH = 16;

struct IPRepresentation {
    // std::string str[4];
    union {
        std::uint32_t uint32;
        std::uint8_t uint8[4];
    };

    bool set_ipv4(const char *ipstr)
    {
        assert(std::strlen(ipstr) > 0);

        const char *cur = ipstr;
        std::uint8_t i;
        std::memset(uint8, 0, sizeof(uint8));
        for (i = 0; i < sizeof(uint8); ++i)
        {
            char *endp = NULL;
            const long val = std::strtol(cur, & endp, 10);

            if (    endp == NULL || endp == cur 
                ||  (i < 3 && *endp != '.')
                ||  val < 0 || val > 255)
            {
                return false;
            }

            cur = endp + 1;
            uint8[sizeof(uint8) - i - 1] = (std::uint8_t)val;
            // str[i] = std::to_string(val);
        }
        return true;
    }

    std::string to_string() const {
        std::stringstream ss;
        ss  << static_cast<std::uint16_t>(uint8[3]) << '.'
            << static_cast<std::uint16_t>(uint8[2]) << '.'
            << static_cast<std::uint16_t>(uint8[1]) << '.'
            << static_cast<std::uint16_t>(uint8[0]);
        return ss.str();
        // return str[0] + "." + str[1] + "." + str[2] + "." + str[3];
    }
};

using ip_pool_t = std::vector<IPRepresentation>;

static void make_ip_pool(ip_pool_t& ip_pool)
{
    ip_pool.clear();
    ip_pool.reserve(256);
    for(std::string line; std::getline(std::cin, line);)
    {
        ip_pool.push_back(IPRepresentation());
        IPRepresentation& ip = ip_pool.back();

        const size_t stop = line.find_first_of('\t');
        if (stop == std::string::npos) {
            std::stringstream ss;
            ss << "Wrong input format string in line number " << ip_pool.size() + 1 << std::endl;
            throw std::invalid_argument(ss.str());
        } else if (stop >= IPV4_ADDRESS_LENGTH) {
            std::stringstream ss;
            ss << "IP address (" << line.substr(stop) << ") length error" << std::endl;
            throw std::invalid_argument(ss.str());
        }

        line[stop] = 0;
        if (!ip.set_ipv4(line.c_str())) {
            std::stringstream ss;
            ss << "IP address (" << line.substr(stop) << ") is wrong" << std::endl;
            throw std::invalid_argument(ss.str());
        }
    }
}

static void ip_pool_dump(const ip_pool_t& ip_pool)
{
    for(auto ip = ip_pool.cbegin(); ip != ip_pool.cend(); ++ip)
        std::cout << ip->to_string() << std::endl;
}

struct ip_filter_args_t {
    uint16_t num;
    int16_t o[4];

    ip_filter_args_t() noexcept : num(0), o() {}
};

template<typename T>
static void error_arguments_list(T a)
{
    std::cout << a << std::endl;
}
template <typename T1, typename T2, typename... Args>
static void error_arguments_list(T1 a, T2 b, Args... args)
{
    std::cout << a << ", ";
    error_arguments_list(b, args...);
}

static ip_pool_t ip_filter_any(const ip_pool_t& ip_pool, uint8_t o)
{
    ip_pool_t filter_ip;
    filter_ip.reserve(256);

    std::copy_if(ip_pool.cbegin(), ip_pool.cend(),
        std::back_inserter(filter_ip),
        [o](const auto& ip){ 
            for (uint16_t i = 0; i < 4; ++i) {
                if (o == ip.uint8[i])
                    return true;
            }
            return false;
        });

    return filter_ip;
}

template<bool is_or>
static ip_pool_t ip_filter_impl(const ip_pool_t& ip_pool, ip_filter_args_t& sargs)
{
    if (sargs.o[0] <= 0 && sargs.o[1] < 0 && sargs.o[2] < 0 && sargs.o[3] < 0) {
        std::stringstream ss;
        ss << "ip_filter: Invalid argument: " << sargs.o[0] << ", " << sargs.o[0] << ", " << sargs.o[0] << ", " << sargs.o[0] << std::endl;
        throw std::invalid_argument(ss.str());
    }

    ip_pool_t filter_ip;
    filter_ip.reserve(256);

    if constexpr (is_or) {
        std::copy_if(ip_pool.cbegin(), ip_pool.cend(),
            std::back_inserter(filter_ip),
            [sargs = std::as_const(sargs)](const auto& ip){ 
                for (uint16_t i = 0; i < sargs.num; ++i) {
                    if (sargs.o[i] > 0 && sargs.o[i] == ip.uint8[i])
                        return true;
                }
                return false;
            });
    } else {
        uint32_t filter_addr = 0;
        uint32_t filter_mask = 0;
        for (uint16_t i = 0; i < sargs.num; ++i) {
            if (sargs.o[i] > 0) {
                const int shift = 32 - (i + 1) * CHAR_BIT;
                filter_addr |= static_cast<uint8_t>(sargs.o[i]) << shift;
                filter_mask |= 0xff << shift;
            }
        }

        std::copy_if(ip_pool.cbegin(), ip_pool.cend(),
            std::back_inserter(filter_ip),
            [filter_addr, filter_mask](const auto& ip){ return (ip.uint32 & filter_mask) == filter_addr; }
        );
    }

    return filter_ip;
}
template <bool is_or, typename T, typename... Args>
static ip_pool_t ip_filter_impl(const ip_pool_t& ip_pool, ip_filter_args_t& sargs, T o, Args... args)
{
    static_assert(std::is_convertible_v<T, int16_t>);

    if (sargs.num < 4) {
        if (o > 255) {
            std::stringstream ss;
            ss << "ip_filter: Invalid argument: ";
            for (uint16_t i = 0; i < sargs.num; ++i)
                ss << sargs.o[i] << ", ";
            ss << o << std::endl;
            throw std::invalid_argument(ss.str());
        }
        if (o >= 0)
            sargs.o[sargs.num++] = static_cast<int16_t>(o);
        else
            sargs.o[sargs.num++] = -1;

        return ip_filter_impl<is_or>(ip_pool, sargs, args...);
    }
    if constexpr (sizeof...(Args) > 0) {
        std::cout << "Any extra arguments will be ignored: ";
        error_arguments_list(args...);
    }
    return ip_filter_impl<is_or>(ip_pool, sargs);
}

template <bool is_or = false, typename... Args>
static ip_pool_t ip_filter(const ip_pool_t& ip_pool, Args... args)
{
    ip_filter_args_t filter_args;
    return ip_filter_impl<is_or>(ip_pool, filter_args, args...);
}

int main([[maybe_unused]]int argc, [[maybe_unused]]char const *argv[])
{
    try
    {
        ip_pool_t ip_pool;
        make_ip_pool(ip_pool);

        std::sort(ip_pool.begin(), ip_pool.end(), [](auto& a, auto& b) { return a.uint32 > b.uint32; });
        // reverse lexicographically sort
        // std::sort(ip_pool.begin(), ip_pool.end(), [](const auto& a, const auto& b) { 
        //     const int res = a.str[0].compare(b.str[0]);
        //     if (res != 0) return res > 0;

        //     res = a.str[1].compare(b.str[1]);
        //     if (res != 0) return res > 0;

        //     res = a.str[2].compare(b.str[2]);
        //     if (res != 0) return res > 0;

        //     res = a.str[3].compare(b.str[3]);
        //     if (res != 0) return res > 0;

        //     return false;
        // });

        ip_pool_dump(ip_pool);

        ip_pool_t filter_ip = ip_filter(ip_pool, 1);
        ip_pool_dump(filter_ip);

        filter_ip = ip_filter(ip_pool, 46, 70);
        ip_pool_dump(filter_ip);
        
        filter_ip = ip_filter_any(ip_pool, 46);
        ip_pool_dump(filter_ip);
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
