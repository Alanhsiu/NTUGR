import matplotlib.pyplot as plt

# Set up the figure and subplots
fig, axs = plt.subplots(1, 2, figsize=(12, 6))
font_size = 20

# Define points for the multi-pin net
points_a = {'A': (2.5, 2.5), 'B': (4.5, 5.5), 'C': (5.5, 3.5), 'D': (4.5, 3.5)}

# 为两个子图设置相同的坐标轴界限
xlims = (2, 6)
ylims = (2, 6)

# 设置第一个子图的坐标轴界限
axs[0].set_xlim(xlims)
axs[0].set_ylim(ylims)

# 设置第一个子图的坐标轴刻度及其标签
axs[0].set_xticks(range(2, 7))
axs[0].set_yticks(range(2, 7))
axs[0].set_xticklabels([])
axs[0].set_yticklabels([])

# 设置第二个子图的坐标轴界限
axs[1].set_xlim(xlims)
axs[1].set_ylim(ylims)

# 设置第二个子图的坐标轴刻度及其标签
axs[1].set_xticks(range(2, 7))
axs[1].set_yticks(range(2, 7))
axs[1].set_xticklabels([])
axs[1].set_yticklabels([])

# Plot multi-pin net
for label, (x, y) in points_a.items():
    if label == 'D':
        continue
    axs[0].plot(x, y, 'ko')
    if label == 'A':
        axs[0].text(x + 0.2, y - 0.2, label, fontsize=font_size)
    else:
        axs[0].text(x + 0.1, y, label, fontsize=font_size)
# axs[0].set_title('(a) A Multi-pin Net.',y=-0.1)
axs[0].grid(True)

# Define points and lines for the RSMT
lines_b = [('A', 'D'), ('D', 'B'), ('D', 'C')]
# For RSMT, adjust points to be at the center of the grid cells, just like in points_a
# points_a = {'A': (2.5, 2.5), 'B': (4.5, 5.5), 'C': (5.5, 3.5), 'D': (4.5, 3.5)}


# Plot RSMT
for start, end in lines_b:
    axs[1].plot([points_a[start][0], points_a[end][0]], [points_a[start][1], points_a[end][1]], 'k--', marker='o')
for label, (x, y) in points_a.items():
    if label != 'e':  # 'e' is not labeled in the example image
        if label == 'D':
            axs[1].text(x + 0.1, y + 0.1, label, fontsize=font_size)
        elif label == 'A':
            axs[1].text(x + 0.2, y - 0.2, label, fontsize=font_size)
        else:
            axs[1].text(x + 0.1, y, label, fontsize=font_size)
        
# axs[1].set_title('(b) The RSMT derived via the Flute algorithm .',y=-0.1)

# Adjust layout to prevent overlapping
plt.tight_layout()
plt.subplots_adjust(wspace=0.15) 

# Show plot
# plt.savefig('steiner.png')

# save high quality image
plt.savefig('steiner.png', dpi=300)