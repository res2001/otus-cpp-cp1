cmake_minimum_required(VERSION 3.15)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VESRION 0.0.${PATCH_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(bayan VERSION ${PROJECT_VESRION})

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.74 REQUIRED COMPONENTS program_options regex)
find_library(ICU_UC NAMES icuuc REQUIRED)
#include_directories(${Boost_INCLUDE_DIRS})

configure_file(version.h.in version.h)

add_executable(${PROJECT_NAME} 
    main.cpp 
    parse_args.cpp 
    scan_dir.cpp 
    utils.cpp 
    DuplicateScanDescription.h 
    ParametersType.h 
    hash.h
    hash_mix.h
    fwd.h)
target_link_libraries(${PROJECT_NAME} Boost::program_options Boost::regex icuuc icui18n)
#icudata

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -pedantic
    )
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
include(CPack)
