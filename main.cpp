#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <tuple>
#include <type_traits>
#include <cstdint>
#include <cassert>
#include <climits>

template<typename T>
std::enable_if_t< std::is_integral_v<T>, void>
print_ip(const T val) {
    for (uint32_t i = 0; i < sizeof(val) ; ++i) {
        const uint64_t shift = (sizeof(val) - i - 1)  * CHAR_BIT;
        const T mask = static_cast<T>(0xFFLLU << shift);
        const uint16_t cur_octet = static_cast<uint16_t>(static_cast<uint8_t>((val & mask) >> shift));
        std::cout << (i == 0 ? "" : ".") << cur_octet;
    }
    std::cout << std::endl;
}

void print_ip(const std::string& val) {
    std::cout << val << std::endl;
}

template<typename T, typename = void>
struct has_cbegin_cend : std::false_type {};

template<typename T>
struct has_cbegin_cend<T, std::void_t<
    decltype(std::declval<T>().cbegin()),
    decltype(std::declval<T>().cend())
>> : std::true_type {};
template<typename T>
inline constexpr bool has_cbegin_cend_v = has_cbegin_cend<T>::value;

static_assert(has_cbegin_cend_v<std::vector<int>>, "Has cbegin/cend");
static_assert(has_cbegin_cend_v<std::list<int>>, "Has cbegin/cend");
static_assert(!has_cbegin_cend_v<int>, "int doesn't have cbegin/cend");

template <typename T>
std::enable_if_t< has_cbegin_cend_v<T>, void>
print_ip(const T& val) {
    bool is_first = true;
    for (auto it = val.cbegin(); it != val.cend(); ++it) {
        std::cout << (is_first ? "" : ".") << *it;
        is_first = false;
    }
    if (!is_first)
        std::cout << std::endl;
}

template<typename T, typename... Ts>
constexpr bool all_types_are_same = std::conjunction_v<std::is_same<T, Ts>...>;

static_assert(all_types_are_same<int, int, int>);
static_assert(!all_types_are_same<int, int&, int>);

template<typename T, typename... Args>
std::enable_if_t<all_types_are_same<T, Args...>, void>
print_ip(const std::tuple<T, Args...>& t) {
    std::apply([](const auto&... args) {
        std::size_t n{0};
        ((std::cout << args << (++n != sizeof...(args) ? "." : "")), ...);
        std::cout << std::endl;
    }, t);
}
void print_ip(const std::tuple<>&) {}

int main(int, char **) {
    print_ip( int8_t{-1} ); // 255 
    print_ip( int16_t{0} ); // 0.0
    print_ip( int32_t{2130706433} ); // 127.0.0.1 
    print_ip( int64_t{8875824491850138409} );  // 123.45.67.89.101.112.131.41 
    print_ip( std::string{"Hello, World!"} ); // Hello, World! 
    print_ip( std::vector<int>{100, 200, 300, 400} ); // 100.200.300.400 
    print_ip( std::list<short>{400, 300, 200, 100} ); // 400.300.200.100 
    print_ip( std::make_tuple(123, 456, 789, 0) ); // 123.456.789.0
}
