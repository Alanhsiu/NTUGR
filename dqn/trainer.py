from enviroment import GridEnvironment
from actor import DQNAgent
import numpy as np
import torch
from torch.utils.tensorboard import SummaryWriter

from tqdm import tqdm
import os

def train(m, n, l, num_nets, episodes, ts):
    state_size = m * n * l
    action_size = 4 * num_nets
    agent = DQNAgent(state_size, action_size)
    
    best_reward = -float('inf') 
    ckpt_dir = f'ckpt/{ts}'
    step_count = 0  
    writer = SummaryWriter(ckpt_dir)
    
    output_dir = f'output/{ts}'
    os.makedirs(output_dir, exist_ok=True)
    
    # Wrap the episode range in tqdm for progress bar display
    for episode in tqdm(range(episodes), desc="Training Progress", unit="episode"):
        nets, capacity_map = generate_data(m, n, l, num_nets)
        env = GridEnvironment(m, n, l, nets, capacity_map)
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

            step_count += 1

        if episode % 10 == 0:
            total_reward = evaluate(m, n, l, num_nets, agent)
            writer.add_scalar('Total Reward', total_reward, episode)

            if total_reward > best_reward:
                best_reward = total_reward
                torch.save(agent.model.state_dict(), f'{ckpt_dir}/best_{ts}.pth')
                with open(f'output/{ts}/training_log.txt', 'a') as file:
                    file.write(f"Step {step_count}: New best model saved with reward: {total_reward}\n")

    torch.save(agent.model.state_dict(), f'{ckpt_dir}/trained_model_{ts}.pth')
    writer.close()
    
    return agent

def generate_data(m, n, l, num_nets, seed=None):
    if seed is not None:
        np.random.seed(seed)
    capacity_map = np.random.randint(0, 3, size=(m, n, l))
    nets = []
    for _ in range(num_nets):
        pin1 = (np.random.randint(0, m), np.random.randint(0, n), np.random.randint(0, l))
        pin2 = (np.random.randint(0, m), np.random.randint(0, n), np.random.randint(0, l))
        while pin1[:2] == pin2[:2]:
            pin2 = (np.random.randint(0, m), np.random.randint(0, n), np.random.randint(0, l))
        nets.append({'pins': [pin1, pin2], 'connected': False})
    return nets, capacity_map

def evaluate(m, n, l, num_nets, agent):
    nets, capacity_map = generate_data(m, n, l, num_nets, seed=42)
    env = GridEnvironment(m, n, l, nets, capacity_map)
    state = env.reset()
    total_reward = 0
    done = False
    while not done:
        actions = agent.act(state, num_nets)
        state, reward, done = env.step(actions)
        total_reward += reward
    # print(f"Evaluation: Total Reward = {total_reward}")
    return total_reward