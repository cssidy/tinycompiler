cmake_minimum_required(VERSION 3.20)
project(tinycompiler)

set(LLVM_DIR /opt/homebrew/Cellar/llvm/22.1.1/lib/cmake/llvm)
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core)

add_executable(tinycompiler
        Lexer.cpp Parser.cpp Sema.cpp CodeGen.cpp ClassGen.cpp main.cpp
)

target_link_libraries(tinycompiler ${llvm_libs})
target_compile_features(tinycompiler PRIVATE cxx_std_17)