cmake_minimum_required(VERSION 2.8)
project(simtoi_models CXX)

# Assemble all of the model source
file(GLOB SOURCE *.cpp)
file(GLOB REMOVE_SOURCE CVoronoi.cpp)
list(REMOVE_ITEM SOURCE ${REMOVE_SOURCE})

# Now add the library
add_library(simtoi_models ${SOURCE})
