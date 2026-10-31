cmake_minimum_required(VERSION 3.15)

project(cisc_instruction_analyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------------------------------------------------
# Sources (reuse existing instruction + comparison logic; add api layer)
# -------------------------------------------------------------------------
add_executable(cisc_instruction_analyzer
    src/main.cpp
    src/instruction.cpp
    src/comparison.cpp
    src/api.cpp
)

target_include_directories(cisc_instruction_analyzer PRIVATE
    include
    third_party/crow
    third_party/asio/include
)

# Crow requires ASIO standalone mode and Winsock on Windows
target_compile_definitions(cisc_instruction_analyzer PRIVATE
    ASIO_STANDALONE
)

if(WIN32)
    target_link_libraries(cisc_instruction_analyzer PRIVATE ws2_32 wsock32)
endif()

if(MSVC)
    target_compile_options(cisc_instruction_analyzer PRIVATE /W3 /EHsc)
else()
    target_compile_options(cisc_instruction_analyzer PRIVATE
        -Wall -Wextra -Wno-unused-parameter
    )
endif()
