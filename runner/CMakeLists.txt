cmake_minimum_required(VERSION 3.15)
project(runner)

set(CMAKE_CXX_STANDARD 17)

# ==== JSON library setup
include(FetchContent)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.7.3
)
FetchContent_MakeAvailable(nlohmann_json)

### ==== Runner executable

add_executable(runner
        src/interceptors.cpp
        src/logging.cpp
        src/main.cpp
        src/read_size_shrink_interceptor.cpp
        src/tracee_controller.cpp
        src/tracing.x86-64.cpp
        src/tracing.cpp)
target_link_libraries(runner nlohmann_json::nlohmann_json)

enable_testing()
add_test(NAME runner_tests
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test/runner_tests.py $<TARGET_FILE:runner>
        )
