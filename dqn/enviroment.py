import numpy as np

class GridEnvironment:
    def __init__(self, m, n, l, nets, capacity_map):
        self.m = m
        self.n = n
        self.l = l
        self.nets = nets
        self.capacity_map = capacity_map
        self.demand = np.zeros((m, n, l))

    def reset(self):
        self.demand = np.zeros((self.m, self.n, self.l))
        return self.get_state()

    def get_state(self):
        state = np.copy(self.demand)
        # for net in self.nets:
        #     # if not net['connected']:
        #     #     print('net:', net)
        return state

    def step(self, actions):
        total_cost = 0
        for i, action in enumerate(actions):
            if i >= len(self.nets):
                break
            if not self.nets[i]['connected']:
                net = self.nets[i]
                path = self.calculate_path(net['pins'], action)
                cost = self.calculate_cost(path)
                self.update_demand(path)
                net['connected'] = True
                total_cost += cost
        done = all(net['connected'] for net in self.nets)
        return self.get_state(), -total_cost, done


    def calculate_path(self, pins, connection_type):
        pin1, pin2 = pins
        # path = []
        # if connection_type == 0:  # L-shape Type 1
        #     for y in range(min(pin1[1], pin2[1]), max(pin1[1], pin2[1]) + 1):
        #         path.append((pin1[0], y))
        #     for x in range(min(pin1[0], pin2[0]), max(pin1[0], pin2[0]) + 1):
        #         if (x, pin2[1]) not in path:
        #             path.append((x, pin2[1]))
        # else:  # L-shape Type 2
        #     for x in range(min(pin1[0], pin2[0]), max(pin1[0], pin2[0]) + 1):
        #         path.append((x, pin1[1]))
        #     for y in range(min(pin1[1], pin2[1]), max(pin1[1], pin2[1]) + 1):
        #         if (pin2[0], y) not in path:
        #             path.append((pin2[0], y))
        path = set()
        if connection_type == 0:
            for y in range(min(pin1[1], pin2[1]), max(pin1[1], pin2[1]) + 1):
                path.add((pin1[0], y, pin1[2]))
            for x in range(min(pin1[0], pin2[0]), max(pin1[0], pin2[0]) + 1):
                path.add((x, pin2[1], pin1[2]))
        elif connection_type == 1:
            for x in range(min(pin1[0], pin2[0]), max(pin1[0], pin2[0]) + 1):
                path.add((x, pin1[1], pin1[2]))
            for y in range(min(pin1[1], pin2[1]), max(pin1[1], pin2[1]) + 1):
                path.add((pin2[0], y, pin1[2]))
        elif connection_type == 2:
            for y in range(min(pin1[1], pin2[1]), max(pin1[1], pin2[1]) + 1):
                path.add((pin1[0], y, pin2[2]))
            for x in range(min(pin1[0], pin2[0]), max(pin1[0], pin2[0]) + 1):
                path.add((x, pin2[1], pin2[2]))
        else:
            for x in range(min(pin1[0], pin2[0]), max(pin1[0], pin2[0]) + 1):
                path.add((x, pin1[1], pin2[2]))
            for y in range(min(pin1[1], pin2[1]), max(pin1[1], pin2[1]) + 1):
                path.add((pin2[0], y, pin2[2]))
        
        return path

    def calculate_cost(self, path):
        cost = 0
        for x, y, z in path:
            cost += np.exp(self.demand[x][y][z] - self.capacity_map[x][y][z])
        return cost

    def update_demand(self, path):
        for x, y, z in path:
            self.demand[x][y][z] += 1
