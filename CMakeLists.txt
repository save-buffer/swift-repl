cmake_minimum_required(VERSION 3.15.0)

project(swift-repl LANGUAGES CXX)

find_package(LLVM CONFIG REQUIRED)
find_package(Swift CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)
find_package(Python COMPONENTS Interpreter)

set(SwiftREPL_TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)
set(LIT ${LLVM_TOOLS_BINARY_DIR}/llvm-lit.py)

configure_file(${SwiftREPL_TESTS_DIR}/lit.cfg.py.in ${SwiftREPL_TESTS_DIR}/lit.cfg.py)

if(NOT DEFINED LIT_ARGS_DEFAULT)
  set(LIT_ARGS_DEFAULT -v -vv)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(DYLIB_EXTENSION ".dll")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DYLIB_EXTENSION ".dylib")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(DYLIB_EXTENSION ".so")
endif()

add_custom_target(test COMMAND ${Python_EXECUTABLE} ${LIT} ${SwiftREPL_TESTS_DIR} ${LIT_ARGS_DEFAULT})
configure_file(Config.h.in Config.h)

set(ALL_INCLUDE_DIRS ${SWIFT_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

add_library(REPL STATIC
  REPL.cpp
  JIT.cpp
  TransformAST.cpp
  TransformIR.cpp
  CommandLineOptions.cpp
  Logging.cpp
  LibraryLoading.cpp)
target_include_directories(REPL PRIVATE ${ALL_INCLUDE_DIRS})
target_link_libraries(REPL PRIVATE
  LLVMExecutionEngine
  LLVMOrcJIT
  swiftAST
  swiftBasic
  swiftFrontend
  swiftImmediate
  swiftParse
  swiftParseSIL
  swiftSema
  swiftSIL
  swiftSyntax
  swiftSyntaxParse)

add_executable(swift-repl swift-repl.cpp)
target_include_directories(swift-repl PRIVATE ${ALL_INCLUDE_DIRS})
target_link_libraries(swift-repl PRIVATE REPL)

add_executable(swift-playground WIN32 swift-playground.cpp)
target_include_directories(swift-playground PRIVATE ${ALL_INCLUDE_DIRS})
target_link_libraries(swift-playground PRIVATE
  REPL
  Kernel32
  User32
  Gdi32
  Msvcrt)

add_dependencies(swift-repl REPL)
add_dependencies(swift-playground REPL)
add_dependencies(test swift-repl)
