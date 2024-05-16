import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

def connect_points(p1, p2):
    x1, y1, z1 = p1
    x2, y2, z2 = p2
    paths = []

    # Helper function to check if a layer is odd
    def is_odd(z):
        return z % 2 == 1

    if z1 == z2:
        if is_odd(z1):
            paths.append([(x1, y1, z1), (x2, y1, z1), (x2, y2, z1)])
        else:
            paths.append([(x1, y1, z1), (x1, y2, z1), (x2, y2, z1)])
    else:
        if is_odd(z1):
            if is_odd(z2):
                paths.append([(x1, y1, z1), (x2, y1, z1), (x2, y1, z2), (x2, y2, z2)])
                paths.append([(x1, y1, z1), (x1, y2, z1), (x1, y2, z2), (x2, y2, z2)])
            else:
                paths.append([(x1, y1, z1), (x2, y1, z1), (x2, y1, z2), (x2, y2, z2)])
                paths.append([(x1, y1, z1), (x1, y2, z1), (x1, y2, z2), (x2, y2, z2)])
        else:
            if is_odd(z2):
                paths.append([(x1, y1, z1), (x1, y1, z2), (x2, y1, z2), (x2, y2, z2)])
                paths.append([(x1, y1, z1), (x1, y2, z1), (x1, y2, z2), (x2, y2, z2)])
            else:
                paths.append([(x1, y1, z1), (x1, y1, z2), (x2, y1, z2), (x2, y2, z2)])
                paths.append([(x1, y1, z1), (x1, y2, z1), (x1, y2, z2), (x2, y2, z2)])

    return paths

def plot_paths(paths):
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    
    colors = ['r', 'g', 'b', 'c', 'm', 'y', 'k']
    for i, path in enumerate(paths):
        xs, ys, zs = zip(*path)
        ax.plot(xs, ys, zs, marker='o', color=colors[i % len(colors)], label=f'Path {i + 1}')
    
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.legend()
    plt.savefig('path.png')

# Example usage
p1 = (1, 1, 3)
p2 = (3, 3, 1)

paths = connect_points(p1, p2)
print("Paths:")
for path in paths:
    print(path)

plot_paths(paths)
