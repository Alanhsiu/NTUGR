def parse_file(filename):
    bounding_boxes = []

    with open(filename, 'r') as f:
        lines = f.readlines()
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            if line.startswith('('):
                min_x, min_y = float('inf'), float('inf')
                max_x, max_y = float('-inf'), float('-inf')
                while not line.startswith(')'):
                    if line.startswith('['):
                        vertices = eval(line)
                        for v in vertices:
                            x, y = v[1], v[2]
                            min_x = min(min_x, x)
                            min_y = min(min_y, y)
                            max_x = max(max_x, x)
                            max_y = max(max_y, y)
                    i += 1
                    line = lines[i].strip()
                bounding_boxes.append([(min_x, min_y), (max_x, max_y)])
            i += 1

    return bounding_boxes


# filename = "../input/ariane133_51.net"
# bounding_boxes = parse_file(filename)
# print(bounding_boxes[3])