#include "Matrix.h"
#include <iostream>

using namespace matrix;

template <typename T, size_t Dim, bool IsConst>
std::ostream& operator<<(std::ostream& os, const ElementImpl<T, Dim, IsConst>& e) {
    // 1. Превращаем наш элемент в tuple. Компилятор сам подставит нужный тип tuple<size_t, ..., T&>
    // auto t = static_cast<std::tuple<size_t, size_t, std::conditional_t<IsConst, const T&, T&>>>(e);
    auto t = static_cast<typename ElementImpl<T, Dim, IsConst>::TupleType>(e);
    
    os << "(";
    // 2. С помощью std::apply распаковываем tuple и выводим элементы через запятую
    std::apply([&os](const auto& first, const auto&... args) {
        os << first;
        // C++17 Fold Expression (свертка): печатает ", элемент" для каждого оставшегося аргумента
        ((os << ", " << args), ...);
    }, t);
    os << ")";
    
    return os;
}
template<typename T, size_t Dim, typename Default = MatrixDefaultGenerator<T>>
void print_matrix(const Matrix<T, Dim, Default>& m)
{
    std::cout << "Matrix size: " << m.size() << std::endl;
    for (auto e: m)
        std::cout << e.indices.to_string() << " = " << e.value << std::endl;
}

int main() {
    /*
     * For C++20
     * constexpr auto intm1 = [](){ return -1; };
     * Matrix<int, 2, decltype(intm1)> mi2;
     */
    struct MatrixIntDefaultGenerator {
        constexpr int operator()() const { return -1; }
    };
    Matrix<int, 2, MatrixIntDefaultGenerator> mi2;
    std::cout << "Matrix<int, 2>:" << std::endl;
    std::cout << "Empty matrix size: " << mi2.size() << std::endl;
    for (int i = 0, j = 9; i < 10; ++i, --j) {
        mi2[i][i] = i;
        mi2[i][j] = 9 - i;
    }
    // std::cout << std::endl << "Fill Matrix:" << std::endl;
    // print_matrix(mi2);
    std::cout << std::endl << "Fill Matrix (enumerated by std::tie):" << std::endl;
    for (auto e: mi2) {
        size_t x, y;
        int v;
        std::tie(x, y, v) = e;
        std::cout << "[" << x << ", " << y << "] = " << v << std::endl;
    }
    std::cout << std::endl << "Fill Matrix (enumerated by structured bindings):" << std::endl;
    for (auto [x, y, v]: mi2) {
        std::cout << "[" << x << ", " << y << "] = " << v << std::endl;
    }

    std::cout << std::endl << "Fill Matrix (elements as std::tuple):" << std::endl;
    for (auto e: mi2) {
        std::cout << e << std::endl;
    }

    {
        std::cout << std::endl << "Unknown element:" << std::endl;
        auto el = mi2[1024][129];
        std::cout << el.get_key().to_string() << " = " << int(el) << std::endl << std::endl;
    }

    struct MatrixDoubleDefaultGenerator {
        constexpr double operator()() const { return -1.; }
    };
    Matrix<double, 4, MatrixDoubleDefaultGenerator> md4;
    for (int i = 0; i < 10; ++i)
        md4[i][i][i][i] = i * 1.5;
    std::cout << "Matrix<double, 4>:" << std::endl;
    print_matrix(md4);

    md4.erase({5, 5, 5, 5});
    std::cout << std::endl << "After erase [5][5][5][5]:" << std::endl;
    std::cout << "Result of contains(5, 5, 5, 5): " << (md4.contains({5,5,5,5}) ? "true" : "false") << std::endl;
    print_matrix(md4);
    {
        std::cout << std::endl << "Unknown element:" << std::endl;
        auto el = md4[1024][129][384][0];
        std::cout << el.get_key().to_string() << " = " << int(el) << std::endl << std::endl;
    }

    /* Test not found */
    assert(mi2.find({1, 1})->value == 1);
    assert(mi2.find({1024, 129}) == mi2.end());
    assert(md4.find({1024, 129, 384, 0}) == md4.end());
}