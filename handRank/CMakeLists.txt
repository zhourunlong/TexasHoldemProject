cmake_minimum_required(VERSION 3.15)
project(Abstraction)

set (CMAKE_CXX_STANDARD 17)

add_subdirectory (pybind11)

pybind11_add_module(handrank rank_hand.cpp)
