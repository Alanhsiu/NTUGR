import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Given data: a series of lines in 3D space
# Read the data from a file data.txt
lines = []
with open('data.txt', 'r') as f:
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

# Setting labels
ax.set_xlabel('X axis')
ax.set_ylabel('Y axis')
ax.set_zlabel('Z axis')

plt.title("3D Visualization of Lines")
plt.savefig('3d_visualization.png')  # Save the 3D visualization as an image
plt.close(fig)  # Close the figure to release memory
