cmake_minimum_required(VERSION 3.15.0)

project(swift-repl LANGUAGES CXX)

find_package(LLVM CONFIG REQUIRED)
find_package(Swift CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)

add_executable(swift-repl
  main.cpp
  REPL.cpp
  JIT.cpp
  TransformAST.cpp
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
