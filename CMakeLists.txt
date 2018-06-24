cmake_minimum_required(VERSION 3.10)
project(Test_task C)

set(CMAKE_C_STANDARD 99)

add_executable(Test_task main.c shm_b.h shm_c1.h)