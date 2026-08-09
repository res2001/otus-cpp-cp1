#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
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

static void make_ip_pool(std::vector<IPRepresentation>& ip_pool)
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

int main([[maybe_unused]]int argc, [[maybe_unused]]char const *argv[])
{
    try
    {
        std::vector<IPRepresentation> ip_pool;
        make_ip_pool(ip_pool);

        std::sort(ip_pool.begin(), ip_pool.end(), [](auto& a, auto& b) { return a.uint32 > b.uint32; });
        // reverse lexicographically sort
        // std::sort(ip_pool.begin(), ip_pool.end(), [](const auto& a, const auto& b) { 
        //     int res = a.str[0].compare(b.str[0]);
        //     if (res != 0) return res > 0;

        //     res = a.str[1].compare(b.str[1]);
        //     if (res != 0) return res > 0;

        //     res = a.str[2].compare(b.str[2]);
        //     if (res != 0) return res > 0;

        //     res = a.str[3].compare(b.str[3]);
        //     if (res != 0) return res > 0;

        //     return false;
        // });

        for(auto ip = ip_pool.cbegin(); ip != ip_pool.cend(); ++ip)
            std::cout << ip->to_string() << std::endl;

        // 222.173.235.246
        // 222.130.177.64
        // 222.82.198.61
        // ...
        // 1.70.44.170
        // 1.29.168.152
        // 1.1.234.8

        // TODO filter by first byte and output
        // ip = filter(1)
        // 1.231.69.33
        // 1.87.203.225
        // 1.70.44.170
        // 1.29.168.152
        // 1.1.234.8
        for_each(ip_pool.cbegin(), ip_pool.cend(), 
            [](const auto& ip){
                if (ip.uint8[3] == 1)
                    std::cout << ip.to_string() << std::endl;
            });

        // TODO filter by first and second bytes and output
        // ip = filter(46, 70)
        // 46.70.225.39
        // 46.70.147.26
        // 46.70.113.73
        // 46.70.29.76
        for_each(ip_pool.cbegin(), ip_pool.cend(), 
            [](const auto& ip){
                if (ip.uint8[3] == 46 && ip.uint8[2] == 70)
                    std::cout << ip.to_string() << std::endl;
            });

        // TODO filter by any byte and output
        // ip = filter_any(46)
        // 186.204.34.46
        // 186.46.222.194
        // 185.46.87.231
        // 185.46.86.132
        // 185.46.86.131
        // 185.46.86.131
        // 185.46.86.22
        // 185.46.85.204
        // 185.46.85.78
        // 68.46.218.208
        // 46.251.197.23
        // 46.223.254.56
        // 46.223.254.56
        // 46.182.19.219
        // 46.161.63.66
        // 46.161.61.51
        // 46.161.60.92
        // 46.161.60.35
        // 46.161.58.202
        // 46.161.56.241
        // 46.161.56.203
        // 46.161.56.174
        // 46.161.56.106
        // 46.161.56.106
        // 46.101.163.119
        // 46.101.127.145
        // 46.70.225.39
        // 46.70.147.26
        // 46.70.113.73
        // 46.70.29.76
        // 46.55.46.98
        // 46.49.43.85
        // 39.46.86.85
        // 5.189.203.46
        for_each(ip_pool.cbegin(), ip_pool.cend(), 
            [](const auto& ip){
                if (ip.uint8[0] == 46 || ip.uint8[1] == 46 || ip.uint8[2] == 46 || ip.uint8[3] == 46)
                    std::cout << ip.to_string() << std::endl;
            });
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
