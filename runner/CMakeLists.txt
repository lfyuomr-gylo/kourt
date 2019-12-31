cmake_minimum_required(VERSION 3.15)
project(runner)

set(CMAKE_CXX_STANDARD 17)

# ==== JSON library setup
set(JSON_BuildTests OFF CACHE INTERNAL "")
include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.7.3
)
FetchContent_MakeAvailable(nlohmann_json)

### ==== Runner executable

add_executable(runner main.cpp)
target_link_libraries(runner nlohmann_json::nlohmann_json)