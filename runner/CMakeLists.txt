cmake_minimum_required(VERSION 3.15)
project(runner)

set(CMAKE_CXX_STANDARD 17)

# ==== JSON library setup
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(thirdparty/nlohmann_json)

add_executable(runner main.cpp)
target_link_libraries(runner nlohmann_json::nlohmann_json)