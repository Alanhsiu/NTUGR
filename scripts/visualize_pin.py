import matplotlib.pyplot as plt
from collections import defaultdict
from tqdm import tqdm
import numpy as np
import os

# Function to read the net data from a file line by line
def read_net_data(file_path):
    with open(file_path, 'r') as file:
        for line in file:
            yield line.strip()

# Function to parse the net data and categorize by z layer without using regex
def parse_net_data(file_path):
    z_dict = defaultdict(list)

    for line in read_net_data(file_path):
        if not line or line.startswith('(') or line.startswith(')'):
            continue
        if '[' in line and ']' in line and '(' not in line:
            current_net = line
        elif line.startswith('['):
            access_points = eval(line)
            if isinstance(access_points, tuple):
                z, x, y = access_points
                z_dict[z].append((x, y))
            elif isinstance(access_points, list) and access_points:
                z, x, y = access_points[0]
                z_dict[z].append((x, y))
    
    return z_dict

# Function to plot a heatmap of pin counts in 2D
def plot_pins_heatmap(z_dict, z_filter=None, save_path=None):
    # Define the size of the heatmap
    heatmap_size = (475, 644)
    heatmap = np.zeros(heatmap_size)

    if z_filter is not None and z_filter in z_dict:
        pins = z_dict[z_filter]
        for x, y in pins:
            if 0 <= x < heatmap_size[1] and 0 <= y < heatmap_size[0]:
                heatmap[y, x] += 1

    plt.figure(figsize=(6.44, 4.75))  # Adjust figure size to match the heatmap size
    cmap = plt.get_cmap('hot')
    cmap.set_under(color='white')  # Set color for under vmin (pin count = 0)
    plt.imshow(heatmap, cmap=cmap, interpolation='nearest', vmin=1)
    plt.colorbar(label='Pin Count')
    plt.xlabel('X')
    plt.ylabel('Y')
    
    if save_path:
        plt.title(f'Heatmap of Pins at Z={z_filter}')
        plt.savefig(save_path)
        plt.close()
    else:
        plt.show()

def main():
    # file_path = 'pin.txt' 
    file_path = '/home/b09901066/ISPD-NTUEE/NTUGR/input/mempool_tile.net' 
    z_dict = parse_net_data(file_path)

    num_pins = sum(len(pins) for pins in z_dict.values())
    print(f'Parsed {num_pins} pins')
    
    dir = 'pins'
    os.makedirs(dir, exist_ok=True)

    # Plot and save heatmap at each z level from 0 to 9
    for z in tqdm(range(10), desc='Processing Z layers'):
        save_path = os.path.join(dir, f'pins_z_{z}.png')
        plot_pins_heatmap(z_dict, z_filter=z, save_path=save_path)

# Run the main function
if __name__ == "__main__":
    main()
