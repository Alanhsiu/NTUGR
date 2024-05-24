#!/usr/bin/env python3
import argparse
import matplotlib.pyplot as plt
import math
import os
import numpy as np

plt.switch_backend('agg')

def visualize(base_path, heatmap_name):
    print('visializing heatmap...')
    file = open(heatmap_name, 'r')
    
    line = file.readline().split(' ')
    n_layers = int(line[0])
    x_size = int(line[1])
    y_size = int(line[2])
    
    n_cols = 4
    n_rows = math.ceil(n_layers / n_cols)
    v_max = 10.0
    
    layer_names = []
    heatmaps = []
    
    for layer_idx in range(n_layers):
        layer_name = file.readline()
        heatmap = [[0.0 for x in range(x_size)] for y in range(y_size)]
        for y in range(y_size):
            line = file.readline().split(' ')
            for x in range(x_size):
                heatmap[y][x] = float(line[x])
        layer_names.append(layer_name)
        heatmaps.append(heatmap)
    
    fig_directory = os.path.join(base_path, heatmap_name.split('.')[0])
    os.system('mkdir -p {}'.format(fig_directory))
    
    min_figsize = 10
    scale = 0.02
    # for layer_idx in range(n_layers):
    #     fig = plt.figure(
    #         figsize=(
    #             max(x_size * scale, min_figsize), 
    #             max(y_size * scale, min_figsize)
    #         )
    #     )
    #     # plt.subplot(n_rows, n_cols, layer_idx + 1)
    #     plt.imshow(heatmaps[layer_idx], vmin=-v_max, vmax=v_max, cmap='coolwarm_r')
    #     plt.title(layer_names[layer_idx])
        
    #     ax = plt.gca()
    #     ax.invert_yaxis()
    #     cax = fig.add_axes([ax.get_position().x1 + 0.005, ax.get_position().y0, 0.005, ax.get_position().height])
    #     plt.colorbar(cax=cax)
        
    #     plt.savefig('{}/heatmap_{}.png'.format(fig_directory, layer_idx))
    
    threshold = 11  # Threshold for plotting

    for layer_idx in range(n_layers):
        fig = plt.figure(
            figsize=(
                max(x_size * scale, min_figsize),
                max(y_size * scale, min_figsize)
            )
        )
        # plt.subplot(n_rows, n_cols, layer_idx + 1)

        # Modify heatmaps[layer_idx] to only include values less than the threshold
        heatmaps_array = np.array(heatmaps[layer_idx]) 
        masked_heatmap = np.ma.masked_where(heatmaps_array >= threshold, heatmaps_array) 

        plt.imshow(masked_heatmap, vmin=-v_max, vmax=v_max, cmap='coolwarm_r')
        plt.title(layer_names[layer_idx])

        ax = plt.gca()
        ax.invert_yaxis()
        cax = fig.add_axes([ax.get_position().x1 + 0.005, ax.get_position().y0, 0.005, ax.get_position().height])
        plt.colorbar(cax=cax)

        plt.savefig('{}/heatmap_{}.png'.format(fig_directory, layer_idx))

base_path = '/home/b09901066/ISPD-NTUEE/NTUGR/heatmaps'                
path = os.path.join(base_path, 'partial_cap.txt')
visualize(base_path, path)