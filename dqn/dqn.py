import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from collections import deque
import random
import matplotlib.pyplot as plt

class GridEnvironment:
    def __init__(self, m, n, nets, capacity_map):
        self.m = m
        self.n = n
        self.nets = nets
        self.capacity_map = capacity_map
        self.demand = np.zeros((m, n))

    def reset(self):
        self.demand = np.zeros((self.m, self.n))
        return self.get_state()

    def get_state(self):
        state = np.copy(self.demand)
        for net in self.nets:
            if not net['connected']:
                print('net:', net)
                state[net['pins'][0]] += 10  # Highlight unconnected nets
                state[net['pins'][1]] += 10
        return state

    def step(self, actions):
        total_cost = 0
        for i, action in enumerate(actions):
            if i >= len(self.nets):  # 檢查索引是否超出範圍
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
        path = []
        if connection_type == 0:  # L-shape Type 1
            for y in range(min(pin1[1], pin2[1]), max(pin1[1], pin2[1]) + 1):
                path.append((pin1[0], y))
            for x in range(min(pin1[0], pin2[0]), max(pin1[0], pin2[0]) + 1):
                if (x, pin2[1]) not in path:
                    path.append((x, pin2[1]))
        else:  # L-shape Type 2
            for x in range(min(pin1[0], pin2[0]), max(pin1[0], pin2[0]) + 1):
                path.append((x, pin1[1]))
            for y in range(min(pin1[1], pin2[1]), max(pin1[1], pin2[1]) + 1):
                if (pin2[0], y) not in path:
                    path.append((pin2[0], y))
        return path

    def calculate_cost(self, path):
        cost = 0
        for x, y in path:
            cost += np.exp(self.demand[x][y] - self.capacity_map[x][y])
        return cost

    def update_demand(self, path):
        for x, y in path:
            self.demand[x][y] += 1

class DQN(nn.Module):
    def __init__(self, input_dim, output_dim):
        super(DQN, self).__init__()
        self.net = nn.Sequential(
            nn.Linear(input_dim, 128),
            nn.ReLU(),
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Linear(64, output_dim)
        )

    def forward(self, x):
        x = x.view(x.size(0), -1)
        return self.net(x)

class DQNAgent:
    def __init__(self, state_size, action_size, batch_size=64, gamma=0.99):
        self.state_size = state_size
        self.action_size = action_size
        self.memory = deque(maxlen=10000)
        self.batch_size = batch_size
        self.gamma = gamma
        self.model = DQN(state_size, action_size)
        self.optimizer = optim.Adam(self.model.parameters(), lr=0.001)
        self.criterion = nn.MSELoss()

    def remember(self, state, actions, reward, next_state, done):
        self.memory.append((state, actions, reward, next_state, done))

    def act(self, state, num_nets):
        state_tensor = torch.tensor(state, dtype=torch.float32).unsqueeze(0)
        with torch.no_grad():
            action_values = self.model(state_tensor)
        # Select actions based on the model's output and num_nets
        actions = [np.argmax(action_values[0, i*2:(i+1)*2].numpy()) for i in range(num_nets)]
        return actions

    def replay(self):
        if len(self.memory) < self.batch_size:
            return
        minibatch = random.sample(self.memory, self.batch_size)

        batch_state = torch.tensor(np.array([item[0] for item in minibatch]), dtype=torch.float32)
        batch_action = torch.tensor([item[1][0] for item in minibatch], dtype=torch.long).unsqueeze(1)
        batch_reward = torch.tensor([item[2] for item in minibatch], dtype=torch.float32)
        batch_next_state = torch.tensor(np.array([item[3] for item in minibatch]), dtype=torch.float32)
        batch_done = torch.tensor([item[4] for item in minibatch], dtype=torch.float32)

        model_output = self.model(batch_state)
        # print("batch_state shape:", batch_state.shape)  # Expect [64, feature_size]
        # print("batch_action shape:", batch_action.shape)  # Expect [64, 1]
        # print("model_output shape:", model_output.shape)  # Expect [64, action_size]

        current_q_values = model_output.gather(1, batch_action).squeeze(1)
        next_q_values = self.model(batch_next_state).max(1)[0].detach()
        expected_q_values = batch_reward + (self.gamma * next_q_values * (1 - batch_done))

        loss = self.criterion(current_q_values, expected_q_values)
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()

def generate_random_data(m, n, num_nets):
    capacity_map = np.random.randint(0, 3, size=(m, n))
    nets = []
    for _ in range(num_nets):
        pin1 = (np.random.randint(0, m), np.random.randint(0, n))
        pin2 = (np.random.randint(0, m), np.random.randint(0, n))
        while pin1 == pin2:
            pin2 = (np.random.randint(0, m), np.random.randint(0, n))
        nets.append({'pins': [pin1, pin2], 'connected': False})
    return nets, capacity_map

def train(m, n, num_nets, episodes):
    state_size = m * n
    action_size = 2 * num_nets
    agent = DQNAgent(state_size, action_size)
    
    for episode in range(episodes):
        nets, capacity_map = generate_random_data(m, n, num_nets)
        env = GridEnvironment(m, n, nets, capacity_map)
        state = env.reset()
        total_reward = 0
        done = False
        while not done:
            actions = agent.act(state, num_nets)
            next_state, reward, done = env.step(actions)
            agent.remember(state, actions, reward, next_state, done)
            agent.replay()
            state = next_state
            total_reward += reward
        print(f"Episode {episode + 1}: Total Reward = {total_reward}")
    
    return agent

def load_agent(state_size, action_size, model_file):
    agent = DQNAgent(state_size, action_size)
    agent.model.load_state_dict(torch.load(model_file))
    agent.model.eval()  # Set the model to evaluation mode
    return agent

def main(m, n, num_nets, episodes):
    nets, capacity_map = generate_random_data(m, n, num_nets)  # Generate new data for inference
    
    # Optionally load the model from file (uncomment the next line if starting from a saved model)
    agent = train(m, n, num_nets, episodes)
    # agent = load_agent(m * n, 2 * num_nets, 'ckpt/trained_model.pth')
    
    inference(m, n, nets, capacity_map, agent)

plt.switch_backend('agg')

def inference(m, n, nets, capacity_map, agent):
    env = GridEnvironment(m, n, nets, capacity_map)
    state = env.reset()
    done = False
    while not done:
        actions = agent.act(state, len(nets))
        state, reward, done = env.step(actions)
        print(f"Actions taken: {actions}, Reward: {reward}")
    
    # Plot the path of the nets according to the actions
    try:
        # breakpoint()
        plt.imshow(capacity_map, cmap='gray')
        for i in range(len(capacity_map)):
            for j in range(len(capacity_map[0])):
                plt.text(j, i, str(capacity_map[i][j]), ha='center', va='center', color='black')
        for i, net in enumerate(nets):
            pin1, pin2 = net['pins']
            turning_point = (pin1[0], pin2[1]) if actions[i] == 0 else (pin2[0], pin1[1])
            plt.plot([pin1[1], turning_point[1]], [pin1[0], turning_point[0]], 'C'+str(i), linewidth=2)
            plt.plot([turning_point[1], pin2[1]], [turning_point[0], pin2[0]], 'C'+str(i), linewidth=2)
            plt.plot(pin1[1], pin1[0], 'o', color='C'+str(i))
            plt.plot(pin2[1], pin2[0], 'o', color='C'+str(i))
    except Exception as e:
        print(f"An error occurred: {str(e)}")

    plt.savefig('inference.png')

if __name__ == "__main__":
    m, n, num_nets, episodes = 10, 10, 10, 10
    main(m, n, num_nets, episodes)
