from actor import DQNAgent
from trainer import generate_data, train, evaluate
from inference import inference
import torch
import time
import os

def load_agent(state_size, action_size, model_file):
    agent = DQNAgent(state_size, action_size)
    agent.model.load_state_dict(torch.load(model_file))
    agent.model.eval()  # Set the model to evaluation mode
    return agent

def main(m, n, l, num_nets, episodes):
    ts = time.time()
    ts = time.strftime('%m%d-%H%M', time.localtime(ts))
    # ts = '0502-2149'
    
    ckpt_dir = f'ckpt/{ts}'
    os.makedirs(ckpt_dir, exist_ok=True)
    
    agent = train(m, n, l, num_nets, episodes, ts)  # train
    ckpt_path = f'{ckpt_dir}/best_{ts}.pth'
    agent = load_agent(m * n * l, 4 * num_nets, ckpt_path)  # load the model
    print(f"Model loaded from {ckpt_path}")
    
    nets, capacity_map = generate_data(m, n, l, num_nets, seed=42)  # Generate new data for inference
    inference(m, n, l, nets, capacity_map, agent, ts)

if __name__ == "__main__":
    m, n, l, num_nets, episodes = 10, 10, 2, 10, 1000
    main(m, n, l, num_nets, episodes)
