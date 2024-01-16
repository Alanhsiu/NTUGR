import torch
import time

def find_available_devices():
    if torch.cuda.is_available():
        num_devices = torch.cuda.device_count()
        device_list = [torch.cuda.get_device_name(i) for i in range(num_devices)]
        print(f"Found {num_devices} CUDA device(s):")
        for i, device_name in enumerate(device_list):
            print(f"  Device {i}: {device_name}")
    else:
        print("No CUDA devices found. Using CPU.")

if __name__ == "__main__":
    while True:
        find_available_devices()
        time.sleep(10)