#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <tuple>
#include <type_traits>
#include <cstdint>
#include <cassert>
#include <climits>

/* 
 * @brief Перегрузка функции print_ip для целочисленных типов.
 * Каждый октет целочисленного типа будет выведен отдельно в десятичном виде, октеты разделяются символом '.'
 * @tparam Любой целочисленный тип. Можно использоать как беззнаковые, так и знаковые типы. При выводе знаковых типов каждый октет итерпретируется как беззнаковый.
 * @param Целое число для вывода.
 * @return Нет
 * @throws Исключения генерируемые std::cout
 */
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

/* 
 * @brief Вспомогательная шаблонная структура has_cbegin_cend для проверки существования методов cbegin() и cend() для переданного типа.
 * Вариант по умолчанию - std::false_type указывает для SFINAE о том, что тип не удовлентворяет условию.
 * @tparam Любой тип
 * @return Определяется переменная член структуры value, содержащее значение false
 */
template<typename T, typename = void>
struct has_cbegin_cend : std::false_type {};
/* 
 * @brief Вспомогательная шаблонная структура has_cbegin_cend для проверки существования методов cbegin() и cend() для переданного типа.
 * Специализация для has_cbegin_cend, проверяющая наличие методов cbegin() и cend().
 * @tparam Контейнерный тип, удовлетворяющий условию отбора по SFINAE (см.выше).
 * @return Определяется переменная член структуры value, содержащее значение true в случае выполнения условия.
 */
template<typename T>
struct has_cbegin_cend<T, std::void_t<
    decltype(std::declval<T>().cbegin()),
    decltype(std::declval<T>().cend())
>> : std::true_type {};
/* 
 * @brief Шаблон вспомогательной переменной, упрощающей использование has_cbegin_cend
 * @tparam Любой тип
 * @return Возвращает has_cbegin_cend<T>::value
 */
template<typename T>
inline constexpr bool has_cbegin_cend_v = has_cbegin_cend<T>::value;

static_assert(has_cbegin_cend_v<std::vector<int>>, "Has cbegin/cend");
static_assert(has_cbegin_cend_v<std::list<int>>, "Has cbegin/cend");
static_assert(!has_cbegin_cend_v<int>, "int doesn't have cbegin/cend");

/* 
 * @brief Перегрузка функции print_ip для конрейнерных типов, поддерживающих константные итераторы (методы cbegin() и cend()). Например: std::vector, std::list, ...
 * Каждый элемент контейнера выводится отдельно как есть. При выводе элементы контейнера разделяются символом '.'
 * @tparam Контейнерный тип, удовлетворяющий условию отбора по SFINAE.
 * @param Константная ссылка на контейнер.
 * @return Нет
 * @throws Исключения генерируемые std::cout
 */
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

/* 
 * @brief Вспомогательное константное значение all_types_are_same типа bool, вычисляемое на этапе компиляции.
 * true - Если все шаблонные параметры имеют один и тот же тип, иначе - false.
 * Может испльзоваться, например, для проверки типов составляющих std::tuple.
 * @tparam Произвольный набор типов.
 * @See https://cppreference.com/cpp/types/conjunction
 */
template<typename T, typename... Ts>
constexpr bool all_types_are_same = std::conjunction_v<std::is_same<T, Ts>...>;

static_assert(all_types_are_same<int, int, int>);
static_assert(!all_types_are_same<int, int&, int>);

/* 
 * @brief Перегрузка функции print_ip для не пустого std::tuple с произвольным набором элементов.
 * Каждый элемент кортежа выводится отдельно как есть. При выводе элементы кортежа разделяются символом '.'
 * @tparam Не пустой std::tuple.
 * @param Константная ссылка на кортеж.
 * @return Нет
 * @throws Исключения генерируемые std::cout
 */
template<typename T, typename... Args>
std::enable_if_t<all_types_are_same<T, Args...>, void>
print_ip(const std::tuple<T, Args...>& t) {
    std::apply([](const auto&... args) {
        std::size_t n{0};
        ((std::cout << args << (++n != sizeof...(args) ? "." : "")), ...);
        std::cout << std::endl;
    }, t);
}

/* 
 * @brief Перегрузка функции print_ip для пустого std::tuple<>.
 * Ничего не делается. Используется как заглушка, для предотвращения ошибок компилятора при печате пустого кортежа.
 * @tparam Пустой std::tuple.
 * @param Константная ссылка на кортеж.
 * @return Нет
 * @throws Нет
 */
template<typename... Args>
std::enable_if_t<sizeof...(Args) == 0, void>
print_ip(const std::tuple<Args...>&) { std::cout << "Empty std::tuple" << std::endl; }

/* 
 * @brief Перегрузка шаблонной функции print_ip для std::string.
 * std::string выводится как есть.
 * @param Константная ссылка на std::string.
 * @return Нет
 * @throws Исключения генерируемые std::cout
 */
template<>
void print_ip<std::string>(const std::string& val) {
    std::cout << val << std::endl;
}

int main(int, char **) {
    print_ip( int8_t{-1} ); // 255 
    print_ip( int16_t{0} ); // 0.0
    print_ip( int32_t{2130706433} ); // 127.0.0.1 
    print_ip( int64_t{8875824491850138409} );  // 123.45.67.89.101.112.131.41 
    print_ip( std::string{"Hello, World!"} ); // Hello, World! 
    print_ip( std::vector<int>{100, 200, 300, 400} ); // 100.200.300.400 
    print_ip( std::list<short>{400, 300, 200, 100} ); // 400.300.200.100 
    print_ip( std::make_tuple(123, 456, 789, 0) ); // 123.456.789.0
    // print_ip( std::make_tuple() );
}
