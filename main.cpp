#include "Matrix.h"
#include <iostream>

using namespace matrix;

template<typename T, size_t Dim>
void print_matrix(const Matrix<T, Dim>& m)
{
    std::cout << "Matrix size: " << m.size() << std::endl;
    for (auto e: m)
        std::cout << e.indices.to_string() << " = " << e.value << std::endl;
}

int main() {
    Matrix<int, 2> mi2;
    std::cout << "Matrix<int, 2>:" << std::endl;
    std::cout << "Empty matrix size: " << mi2.size() << std::endl;
    for (int i = 0, j = 9; i < 10; ++i, --j) {
        mi2[i][i] = i;
        mi2[i][j] = 9 - i;
    }
    std::cout << std::endl << "Fill Matrix:" << std::endl;
    print_matrix(mi2);

    std::cout << std::endl << "Unknown element:" << std::endl;
    auto el = mi2[1024][129];
    std::cout << el.get_key().to_string() << " = " << int(el) << std::endl << std::endl;

    Matrix<double, 4> md4;
    for (int i = 0; i < 10; ++i)
        md4[i][i][i][i] = i * 1.5;
    std::cout << "Matrix<double, 4>:" << std::endl;
    print_matrix(md4);

    md4.erase({5, 5, 5, 5});
    std::cout << std::endl << "After erase [5][5][5][5]:" << std::endl;
    std::cout << "Result of contains(5, 5, 5, 5): " << (md4.contains({5,5,5,5}) ? "true" : "false") << std::endl;
    print_matrix(md4);
}