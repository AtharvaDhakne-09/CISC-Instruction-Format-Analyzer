cmake_minimum_required(VERSION 3.15)

project(cisc_instruction_analyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_executable(cisc_analyzer
    backend/main.cpp
    backend/analyzer.cpp
)

target_include_directories(cisc_analyzer SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/crow"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include"
)

target_compile_definitions(cisc_analyzer PRIVATE
    "CISC_FRONTEND_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/frontend\""
)

if(WIN32)
    target_link_libraries(cisc_analyzer PRIVATE ws2_32 mswsock)
endif()

target_link_libraries(cisc_analyzer PRIVATE Threads::Threads)

if(MINGW)
    # Keep the executable self-contained so it does not need MinGW runtime DLLs.
    target_link_options(cisc_analyzer PRIVATE -static)
endif()

if(MSVC)
    target_compile_options(cisc_analyzer PRIVATE /W4)
else()
    target_compile_options(cisc_analyzer PRIVATE -Wall -Wextra -Wpedantic)
endif()
