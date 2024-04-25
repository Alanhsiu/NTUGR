#include <iostream>
#include <cuda_runtime.h>

// CUDA核心函數: 計算陣列元素相加
__global__ void vecAdd(float* a, float* b, float* c, int n) {
    // 獲取全域索引
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    // 確保索引在範圍內
    if (idx < n) {
        c[idx] = a[idx] + b[idx];
    }
}

int main() {
    const int N = 1000000;  // 陣列大小

    // 分配主機記憶體
    float* h_a = new float[N];
    float* h_b = new float[N];
    float* h_c = new float[N];

    // 初始化資料
    for (int i = 0; i < N; i++) {
        h_a[i] = static_cast<float>(i);
        h_b[i] = static_cast<float>(i * 2);
    }

    // 分配裝置記憶體
    float* d_a, * d_b, * d_c;
    cudaMalloc(&d_a, N * sizeof(float));
    cudaMalloc(&d_b, N * sizeof(float));
    cudaMalloc(&d_c, N * sizeof(float));

    // 將資料從主機複製到裝置
    cudaMemcpy(d_a, h_a, N * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, h_b, N * sizeof(float), cudaMemcpyHostToDevice);

    // 設定區塊和執行緒配置
    const int blockSize = 256;
    const int numBlocks = (N + blockSize - 1) / blockSize;

    // 執行核心函數
    vecAdd<<<numBlocks, blockSize>>>(d_a, d_b, d_c, N);

    // 將結果從裝置複製到主機
    cudaMemcpy(h_c, d_c, N * sizeof(float), cudaMemcpyDeviceToHost);

    // 釋放裝置記憶體
    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);

    // 驗證結果
    bool success = true;
    for (int i = 0; i < N; i++) {
        if (h_c[i] != h_a[i] + h_b[i]) {
            success = false;
            break;
        }
    }

    if (success) {
        std::cout << "Test passed!" << std::endl;
    } else {
        std::cout << "Test failed!" << std::endl;
    }

    // 釋放主機記憶體
    delete[] h_a;
    delete[] h_b;
    delete[] h_c;

    return 0;
}