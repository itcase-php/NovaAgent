cmake_minimum_required(VERSION 3.15)
project(NovaAgent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译警告选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# 可执行文件目标
add_executable(NovaAgent
    Json.h
    Json.cpp
    MemoryManager.h
    MemoryManager.cpp
    ToolRegistry.h
    ToolRegistry.cpp
    LLMClient.h
    LLMClient.cpp
    NovaAgent.h
    NovaAgent.cpp
    main.cpp
)
