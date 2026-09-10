cmake_minimum_required(VERSION 3.20)
project(pcpd CXX)

# Prefix
if (NOT PREFIX)
    set(PREFIX ${CMAKE_CURRENT_BINARY_DIR})
endif (NOT PREFIX)
set(CMAKE_INSTALL_PREFIX ${PREFIX} CACHE STRING "Installation prefix." FORCE)

if (CXX)
    set(CMAKE_CXX_COMPILER ${CXX})
endif (CXX)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Release or Debug
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif ()


# Detect PNL
find_package(Pnl REQUIRED)
set(LIBS ${LIBS} ${PNL_LIBRARIES})
include_directories(${PNL_INCLUDE_DIRS})
if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-DPNL_RANGE_CHECK_OFF)
endif()

# Find nlohmann_json to parse json files
# Add nlohmann_json::nlohmann_json to target_link_libraries
find_package(nlohmann_json QUIET)
if (NOT nlohmann_json_FOUND)
    if (${CMAKE_VERSION} VERSION_GREATER "3.24.0")
        CMAKE_POLICY(SET CMP0135 NEW)
    endif()
    include(FetchContent)
    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
    FetchContent_MakeAvailable(json)
endif()

### Ne rien modifier dans la partie au dessus de cette ligne.

include(CTest)

# Test the parser
add_executable(test_json_reader src/json_helper.cpp test/test_json_reader.cpp)
target_include_directories(test_json_reader PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_json_reader
    ${PNL_LIBRARIES}
    nlohmann_json::nlohmann_json
)
pnl_add_postbuild(test_json_reader)

add_executable(test_compute_lastdate test/test_compute_lastdate.cpp)

# Pour compiler vos exécutables, compléter les lignes ci-dessous.
# La cible price0 est définie plus bas (après PRICER_SOURCES / IO_SOURCES).

# add_executable(hedge src/hedge.cpp ${PRICER_SOURCES} ${IO_SOURCES})
# target_include_directories(hedge PRIVATE ${PROJECT_SOURCE_DIR}/src)
# target_link_libraries(hedge
#     ${PNL_LIBRARIES}
#     nlohmann_json::nlohmann_json
# )
# pnl_add_postbuild(hedge)
# TODO hedge : nécessite le prix/delta en t > 0 (pas encore implémenté).

set(PRICER_SOURCES
    src/Option.cpp
    src/Basket.cpp
    src/AsianOption.cpp
    src/BlackScholesModel.cpp
    src/MonteCarlo.cpp
)

# Sources d'entrées/sorties (lecture JSON, format d'affichage) partagées par
# les exécutables price0 / hedge.
set(IO_SOURCES
    src/json_helper.cpp
    src/json_reader.cpp
    src/option_factory.cpp
    src/pricing_results.cpp
)

add_executable(price0 src/price0.cpp ${PRICER_SOURCES} ${IO_SOURCES})
target_include_directories(price0 PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(price0
    ${PNL_LIBRARIES}
    nlohmann_json::nlohmann_json
)
pnl_add_postbuild(price0)

add_executable(test_pricer test/test_pricer.cpp ${PRICER_SOURCES})
target_include_directories(test_pricer PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_pricer
    ${PNL_LIBRARIES}
    nlohmann_json::nlohmann_json
)
pnl_add_postbuild(test_pricer)

if (BUILD_TESTING)
    find_package(GTest QUIET)
    if (NOT GTest_FOUND)
        include(FetchContent)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
            URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(unit_tests
        test/unit/test_utils.cpp
        test/unit/test_options.cpp
        test/unit/test_black_scholes_model.cpp
        test/unit/test_monte_carlo.cpp
        test/unit/test_json_and_results.cpp
        test/unit/test_portfolio.cpp
        src/json_helper.cpp
        src/pricing_results.cpp
        src/portfolio.cpp
        ${PRICER_SOURCES}
    )
    target_compile_features(unit_tests PRIVATE cxx_std_17)
    target_include_directories(unit_tests PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/test/unit
    )
    target_link_libraries(unit_tests PRIVATE
        ${PNL_LIBRARIES}
        nlohmann_json::nlohmann_json
        GTest::gtest_main
    )
    pnl_add_postbuild(unit_tests)

    include(GoogleTest)
    gtest_discover_tests(unit_tests)

    add_test(NAME legacy_compute_lastdate COMMAND test_compute_lastdate)
    add_test(NAME legacy_json_reader
        COMMAND test_json_reader ${PROJECT_SOURCE_DIR}/test/data/json_reader_input.json)
    add_test(NAME legacy_pricer COMMAND test_pricer)
endif()


# Print compiler flags
get_directory_property(DirDefs COMPILE_DEFINITIONS)
message(STATUS "COMPILE_DEFINITIONS = ${DirDefs}")
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_LINKER_EXE_FLAGS}" )
else()
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_LINKER_EXE_FLAGS}" )
endif()
