#include <cuda_runtime.h>
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc != 7) {
        std::cerr << "Usage: ./route -cap $data.cap -net $data.net -output $data.output" << std::endl;
        return 1;
    }

    std::string cap_file = argv[2];
    std::string net_file = argv[4];
    std::string output_file = argv[6];

    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    std::ofstream outputFile(output_file);

    for (int i = 0; i < deviceCount; ++i) {
        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, i);
        outputFile << "Device " << i << ": " << deviceProp.name << std::endl;
    }

    return 0;
}
