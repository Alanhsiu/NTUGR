cmake_minimum_required(VERSION 3.18)
project(VectorAdd LANGUAGES CXX CUDA)

set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)

# 設置CUDA架構代碼
set(CUDA_ARCH_FLAGS "-arch=sm_75")

# 添加CUDA核心函數文件
cuda_add_library(kernel STATIC kernel.cu)
target_compile_options(kernel PRIVATE ${CUDA_ARCH_FLAGS})

# 添加主機代碼文件
add_executable(vector_add main.cpp)

# 鏈接CUDA庫
target_link_libraries(vector_add PRIVATE kernel ${CUDA_LIBRARIES})

# 設置CUDA可分離編譯
set_target_properties(kernel PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_PTX_COMPILATION ON
)