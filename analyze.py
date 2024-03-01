import matplotlib.pyplot as plt

def parse_data_from_file(file_path):
    with open(file_path, 'r') as file:
        data = file.read()
    
    nets = {}
    lines = data.split('\n')
    current_net = None
    for line in lines:
        if line.startswith('Net'):
            current_net = line.strip()
            nets[current_net] = []
        elif line.startswith('['):
            points = eval(line.strip())
            for point in points:
                nets[current_net].append(point[1:3])  # 只获取x和y坐标
    return nets

def draw_and_save_bounding_boxes_on_one_plot(nets):
    plt.figure()
    for net, points in nets.items():
        x_coords, y_coords = zip(*points)
        min_x, max_x = min(x_coords), max(x_coords)
        min_y, max_y = min(y_coords), max(y_coords)
        
        # 绘制bounding box
        plt.plot([min_x, max_x, max_x, min_x, min_x], [min_y, min_y, max_y, max_y, min_y], label=f'{net} bounding box')
        # 绘制接入点
        plt.scatter(x_coords, y_coords, label=f'{net} access points')
    
    plt.title('All Nets Bounding Boxes and Access Points')
    plt.xlabel('X')
    plt.ylabel('Y')
    plt.legend()
    plt.grid(True)
    plt.savefig('all_nets_bounding_boxes.png')
    plt.show()

# 文件路径
file_path = 'input/example.net'

nets = parse_data_from_file(file_path)
draw_and_save_bounding_boxes_on_one_plot(nets)
