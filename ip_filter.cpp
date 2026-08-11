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

template <bool is_or = false>
static ip_pool_t ip_filter(const ip_pool_t& ip_pool, short o1 = -1, short o2 = -1, short o3 = -1, short o4 = -1)
{
    assert(o1 );
    ip_pool_t filter_ip;

    do {
        if (    (o1 <= 0  && o2 < 0   && o3 < 0   && o4 < 0)
            ||  (o1 > 255 || o2 > 255 || o3 > 255 || o4 > 255)) {
            std::stringstream ss;
            ss << "ip_filter: Invalid argument: " << o1 << ", " << o2 << ", " << o3 << ", " << o4 << std::endl;
            throw std::invalid_argument(ss.str());
        }

        filter_ip.reserve(256);

        if constexpr (is_or) {
            std::copy_if(ip_pool.cbegin(), ip_pool.cend(),
                std::back_inserter(filter_ip),
                [o1, o2, o3, o4](const auto& ip){ 
                    if (o1 > 0  && o1 == ip.uint8[3])
                        return true;
                    if (o2 >= 0 && o2 == ip.uint8[2])
                        return true;
                    if (o2 >= 0 && o2 == ip.uint8[1])
                        return true;
                    if (o2 >= 0 && o2 == ip.uint8[0])
                        return true;
                    return false;
                });
        } else {
            uint32_t filter_addr = 0;
            uint32_t filter_mask = 0;
            if (o1 > 0) {
                filter_addr |= static_cast<uint8_t>(o1) << 24;
                filter_mask |= 0xff << 24;
            }
            if (o2 > 0) {
                filter_addr |= static_cast<uint8_t>(o2) << 16;
                filter_mask |= 0xff << 16;
            }
            if (o3 > 0) {
                filter_addr |= static_cast<uint8_t>(o3) << 8;
                filter_mask |= 0xff << 8;
            }
            if (o4 > 0) {
                filter_addr |= static_cast<uint8_t>(o4);
                filter_mask |= 0xff;
            }

            std::copy_if(ip_pool.cbegin(), ip_pool.cend(),
                std::back_inserter(filter_ip),
                [filter_addr, filter_mask](const auto& ip){ return (ip.uint32 & filter_mask) == filter_addr; }
            );
        }
    } while(false);

    return filter_ip;
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
        
        filter_ip = ip_filter<true>(ip_pool, 46, 46, 46, 46);
        ip_pool_dump(filter_ip);
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
