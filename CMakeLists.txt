cmake_minimum_required(VERSION 3.20)
project(tinycompiler)

set(LLVM_DIR /opt/homebrew/Cellar/llvm/22.1.1/lib/cmake/llvm)
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})
add_definitions(${LLVM_DEFINITIONS})

# tinytblgen: standalone TableGen tool that generates TinyCPU.inc
add_executable(tinytblgen TinyTableGen.cpp InstEmitter.cpp)
target_link_libraries(tinytblgen
        LLVMTableGen LLVMTableGenCommon LLVMTableGenBasic LLVMSupport)
target_compile_features(tinytblgen PRIVATE cxx_std_17)

# run tinytblgen at build time to produce TinyCPU.inc
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/TinyCPU.inc
    COMMAND $<TARGET_FILE:tinytblgen>
            --gen-insts
            -I ${CMAKE_CURRENT_SOURCE_DIR}
            -o ${CMAKE_CURRENT_BINARY_DIR}/TinyCPU.inc
            ${CMAKE_CURRENT_SOURCE_DIR}/TinyCPU.td
    DEPENDS tinytblgen ${CMAKE_CURRENT_SOURCE_DIR}/TinyCPU.td
    COMMENT "Generating TinyCPU.inc from TinyCPU.td"
)
add_custom_target(TinyCPUInc DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/TinyCPU.inc)

llvm_map_components_to_libnames(llvm_libs
        support core passes analysis transformutils scalaropts
        instcombine ipo)

add_executable(tinycompiler
        Lexer.cpp Parser.cpp Sema.cpp CodeGen.cpp ClassGen.cpp
        Optimizer.cpp TinyCPUBackend.cpp main.cpp
)

add_dependencies(tinycompiler TinyCPUInc)
target_include_directories(tinycompiler PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(tinycompiler ${llvm_libs})
target_compile_features(tinycompiler PRIVATE cxx_std_17)
