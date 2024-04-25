import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Given data: a series of lines in 3D space
# Read the data from a file data.txt
lines = []
with open('/home/b09901066/ISPD-NTUEE/NTUGR/scripts/data.txt', 'r') as f:
    for line in f:
        lines.append([float(x) for x in line.split()])

# Creating a 3D plot
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

# Adding lines to the plot
for line in lines:
    x = [line[0], line[3]]
    y = [line[1], line[4]]
    z = [line[2], line[5]]
    ax.plot(x, y, z)
    
    for i, line in enumerate(lines):
        for j in range(0, len(line), 3):
            ax.text(line[j], line[j+1], line[j+2], f'({int(line[j])}, {int(line[j+1])}, {int(line[j+2])})', color='red')
# Setting labels to display only integers on the axes
ax.set_xticks(range(int(min(x)), int(max(x))+1))
ax.set_yticks(range(int(min(y)), int(max(y))+1))
ax.set_zticks(range(int(min(z)), int(max(z))+1))

plt.title("3D Visualization of Lines")
plt.savefig('3d_visualization.png')  # Save the 3D visualization as an image
plt.close(fig)  # Close the figure to release memory
