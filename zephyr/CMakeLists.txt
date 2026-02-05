cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(soem_sample)

target_sources(app PRIVATE main.c)

# 包含 SOEM 库的构建定义
include(soem.cmake)
