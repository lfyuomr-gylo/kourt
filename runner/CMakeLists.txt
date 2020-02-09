cmake_minimum_required(VERSION 3.15)
project(runner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==== JSON library setup
include(FetchContent)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.7.3
)
FetchContent_MakeAvailable(nlohmann_json)

### ==== Main sources

include_directories(src/include)

add_library(runner_lib
        src/interceptors.cpp
        src/logging.cpp
        src/runner_main.cpp
        src/read_size_shrink_interceptor.cpp
        src/tracee_controller.cpp
        src/tracing.x86-64.cpp
        src/tracing.cpp
        src/elf.cpp)
target_link_libraries(runner_lib nlohmann_json::nlohmann_json)

add_executable(runner src/main.cpp)
target_link_libraries(runner runner_lib)

### ==== Tests

enable_testing()
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0
)
FetchContent_MakeAvailable(googletest)


include(GoogleTest)

add_executable(functional_tests test/functional_tests.cpp)
target_link_libraries(functional_tests runner_lib gtest_main nlohmann_json::nlohmann_json)
gtest_discover_tests(functional_tests)
