cmake_minimum_required(VERSION 3.12)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VESRION 0.0.${PATCH_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(async VERSION ${PROJECT_VESRION})

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.74 REQUIRED COMPONENTS program_options regex)
find_library(ICU_UC NAMES icuuc REQUIRED)

configure_file(version.h.in version.h)

add_library(${PROJECT_NAME} SHARED async.cpp async.h)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VESRION} SOVERSION 0)

add_executable(bulk main.cpp parse_args.cpp utils.cpp fwd.h async.h)
target_link_libraries(bulk async Boost::program_options Boost::regex icuuc icui18n)
target_include_directories(bulk PRIVATE "${CMAKE_BINARY_DIR}")

if (MSVC)
    add_compile_options(/W4)
else ()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
install(TARGETS bulk RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-all-dev (>= 1.74.0)")
include(CPack)

if(WITH_BOOST_TEST)
    enable_testing()
    add_test(test_version test_version)
endif()
