import matplotlib.pyplot as plt
from parser import parse_file

plt.switch_backend('agg')

def draw_box(diagonals, fig_name):
    fig, ax = plt.subplots()
    for diagonal in diagonals:
        x1, y1 = diagonal[0]
        x2, y2 = diagonal[1]
        
        # Calculate other vertices
        vertices = [
            [x1, y1],
            [x2, y1],
            [x2, y2],
            [x1, y2]
        ]

        # Plotting vertices
        ax.scatter([v[0] for v in vertices], [v[1] for v in vertices], c='r', marker='o')

        # Connecting vertices to form the box
        for i in range(4):
            ax.plot([vertices[i][0], vertices[(i+1) % 4][0]], [vertices[i][1], vertices[(i+1) % 4][1]], c='b')

    plt.savefig(fig_name)

# Example diagonal of a 2D box
bounding_boxes = parse_file("../input/ariane133_51.net")
draw_box(bounding_boxes, "plot.jpg")
