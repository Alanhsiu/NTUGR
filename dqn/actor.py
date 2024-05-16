import random
import torch
import torch.nn as nn
import torch.optim as optim
import numpy as np
from collections import deque

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
        # actions = [np.argmax(action_values[0, i*2:(i+1)*2].numpy()) for i in range(num_nets)]
        actions = [np.argmax(action_values[0, i*4:(i+1)*4].numpy()) for i in range(num_nets)]
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
        current_q_values = model_output.gather(1, batch_action).squeeze(1)
        next_q_values = self.model(batch_next_state).max(1)[0].detach()
        expected_q_values = batch_reward + (self.gamma * next_q_values * (1 - batch_done))

        loss = self.criterion(current_q_values, expected_q_values)
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
