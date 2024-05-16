from enviroment import GridEnvironment
import matplotlib.pyplot as plt
import os

plt.switch_backend('agg')

# def inference(m, n, l, nets, capacity_map, agent, ts="00-00"):
#     env = GridEnvironment(m, n, l, nets, capacity_map)
#     state = env.reset()
#     done = False
#     while not done:
#         actions = agent.act(state, len(nets))
#         state, reward, done = env.step(actions)
#         print(f"Actions taken: {actions}, Reward: {reward}")
    
#     plt.imshow(capacity_map, cmap='viridis')
#     plt.colorbar()
#     for i in range(len(capacity_map)):
#         for j in range(len(capacity_map[0])):
#             plt.text(j, i, str(capacity_map[i][j]), ha='center', va='center', color='black')
#     for i, net in enumerate(nets):
#         pin1, pin2 = net['pins']
#         turning_point = (pin1[0], pin2[1]) if actions[i] == 0 else (pin2[0], pin1[1])
#         plt.plot([pin1[1], turning_point[1]], [pin1[0], turning_point[0]], 'C'+str(i), linewidth=2)
#         plt.plot([turning_point[1], pin2[1]], [turning_point[0], pin2[0]], 'C'+str(i), linewidth=2)
#         plt.plot(pin1[1], pin1[0], 'o', color='C'+str(i))
#         plt.plot(pin2[1], pin2[0], 'o', color='C'+str(i))

#     output_dir = 'output'
#     plt.savefig(f'{output_dir}/inference_{ts}.png')

def inference(m, n, l, nets, capacity_map, agent, ts="00-00"):
    env = GridEnvironment(m, n, l, nets, capacity_map)
    state = env.reset()
    done = False
    while not done:
        actions = agent.act(state, len(nets))
        state, reward, done = env.step(actions)
        print(f"Actions taken: {actions}, Reward: {reward}")
    
    output_dir = f'output/{ts}'
    os.makedirs(output_dir, exist_ok=True)

    for z_layer in range(l):
        plt.figure()
        plt.imshow(capacity_map[:, :, z_layer], cmap='viridis')
        plt.colorbar()
        for i in range(capacity_map.shape[0]):
            for j in range(capacity_map.shape[1]):
                plt.text(j, i, str(capacity_map[i, j, z_layer]), ha='center', va='center', color='black')

        for i, net in enumerate(nets):
            pin1, pin2 = net['pins']
            if actions[i] == 0:
                turning_point = (pin1[0], pin2[1], pin1[2])
            elif actions[i] == 1:
                turning_point = (pin2[0], pin1[1], pin1[2])
            elif actions[i] == 2:
                turning_point = (pin1[0], pin2[1], pin2[2])
            elif actions[i] == 3:
                turning_point = (pin2[0], pin1[1], pin2[2])
            
            if pin1[2] == z_layer and turning_point[2] == z_layer:
                plt.plot([pin1[1], turning_point[1]], [pin1[0], turning_point[0]], 'C'+str(i), linewidth=2)
                plt.plot([turning_point[1], pin2[1]], [turning_point[0], pin2[0]], 'C'+str(i), linewidth=2)
                plt.plot(pin1[1], pin1[0], 'o', color='C'+str(i))
                plt.plot(pin2[1], pin2[0], 'o', color='C'+str(i))

        plt.savefig(f'{output_dir}/inference_z{z_layer}_{ts}.png')
        plt.close()