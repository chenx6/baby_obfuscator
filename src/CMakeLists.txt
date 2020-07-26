cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 14)

add_library(obfuscate
  MODULE
  ObfuscateString.cpp
  BogusFlow.cpp
  Substitution.cpp
)
