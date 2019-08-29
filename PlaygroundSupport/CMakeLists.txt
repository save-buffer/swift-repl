set(CMAKE_LINK_LIBRARY_FLAG "-l")
add_library(PlaygroundSupport SHARED
  PlaygroundSupport.swift)
target_link_libraries(PlaygroundSupport PUBLIC
  SwiftWin32
  swift-playground)
target_compile_options(PlaygroundSupport PRIVATE
  "SHELL:-Xcc -I..")
set_target_properties(PlaygroundSupport PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/swift
  INTERFACE_LINK_DIRECTORIES $<TARGET_LINKER_FILE_DIR:PlaygroundSupport>)
