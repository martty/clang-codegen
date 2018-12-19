project(clangdep)
cmake_minimum_required(VERSION 3.10)

set(LLVM_VERSION_MAJOR 8)
set(LLVM_VERSION_MINOR 0)
set(LLVM_VERSION_PATCH 0)
set(LLVM_INCLUDE_TESTS OFF CACHE INTERNAL "" FORCE)
set(LLVM_INCLUDE_UTILS OFF)
set(LLVM_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(LLVM_INCLUDE_TOOLS ON)
set(LLVM_BUILD_TOOLS OFF CACHE INTERNAL "" FORCE)
set(LLVM_INCLUDE_BENCHMARKS OFF CACHE INTERNAL "" FORCE)
set(LLVM_INCLUDE_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(LLVM_TARGETS_TO_BUILD "" CACHE INTERNAL "" FORCE)
set(LLVM_ENABLE_DIA_SDK OFF CACHE INTERNAL "" FORCE)
set(CLANG_ENABLE_STATIC_ANALYZER OFF CACHE INTERNAL "" FORCE)
set(CLANG_ENABLE_ARCMT OFF CACHE INTERNAL "" FORCE)
set(LLVM_ENABLE_PROJECTS "clang;clang-tools-extra" CACHE INTERNAL "" FORCE)
set(LLVM_EXTERNAL_CLANG_SOURCE_DIR ext/clang CACHE INTERNAL "" FORCE)
set(LLVM_TOOL_CLANG_TOOLS_EXTRA_BUILD ON CACHE INTERNAL "" FORCE)


add_subdirectory(ext/llvm)

# apply our patch to clang
#execute_process(COMMAND git apply --directory=ext/clang ext/0001-Basic-PKY-attributes.patch WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(LLVM_LINK_COMPONENTS support)
add_clang_tool(clangdep main.cc)
target_include_directories(clangdep PRIVATE "ext/llvm/include" "ext/clang/include" "build/ext/llvm/include" "build/ext/llvm/tools/clang/include")
add_dependencies(clangdep clang-headers)
target_link_libraries(clangdep PRIVATE clangAST clangASTMatchers clangBasic clangFrontend clangRewrite clangTooling)