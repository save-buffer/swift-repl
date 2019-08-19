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

add_custom_target(test COMMAND ${Python_EXECUTABLE} ${LIT} ${SwiftREPL_TESTS_DIR} ${LIT_ARGS_DEFAULT})

add_executable(swift-repl
  main.cpp
  REPL.cpp
  JIT.cpp
  TransformAST.cpp
  TransformIR.cpp
  CommandLineOptions.cpp
  Logging.cpp)
target_include_directories(swift-repl PRIVATE ${SWIFT_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_link_libraries(swift-repl PRIVATE
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
