cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CUDA_COMPILER $ENV{CUDA_DIR})

project(CudaTest LANGUAGES C CXX CUDA)

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} main.cpp test.cu)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(${PROJECT_NAME} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)