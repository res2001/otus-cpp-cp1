#include "fwd.h"
#include <iostream>
#include <locale>

#ifdef _WIN32
    #include <windows.h>
#endif

static void initialize_console()
{
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    
    // Включаем поддержку ANSI-последовательностей (виртуальный терминал), 
    // чтобы консоль Windows 10/11 адекватно принимала UTF-8 потоки
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#else
    std::setlocale(LC_ALL, "");
#endif
}

int main(int argc, const char* argv[]) {
    initialize_console();

    ParametersType param;
    try {
        if (!parse_args(argc, argv, param))
            return EXIT_FAILURE;
        scan_dir(param);
    } catch(const std::exception& e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}