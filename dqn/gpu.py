import torch

# Check if a GPU is available
if torch.cuda.is_available():
    device = torch.device('cuda')
    print("GPU is available and will be used.")
else:
    device = torch.device('cpu')
    print("No GPU available, using CPU instead.")

print(torch.__version__)
