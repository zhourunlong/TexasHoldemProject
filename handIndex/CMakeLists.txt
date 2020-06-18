cmake_minimum_required(VERSION 3.15)
project(HandIndex)

set (CMAKE_CXX_STANDARD 17)

add_subdirectory (pybind11)
aux_source_directory(hand-isomorphism/src HAND_ISOMORPHISM_SRC)
pybind11_add_module(index index.cpp ${HAND_ISOMORPHISM_SRC})

