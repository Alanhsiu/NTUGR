mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../src && make

input_path="/docker_data/b09901066/input"
output_PR_path="/docker_data/b09901066/output"
output_log_path="/home/b09901066/ISPD-NTUEE/NTUGR/output"
echo "Running mempool_tile"
./route -cap $input_path/mempool_tile.cap -net $input_path/mempool_tile.net -output $output_PR_path/mempool_tile.PR_output > $output_log_path/mempool_tile.log
echo "Running mempool_group"
./route -cap $input_path/mempool_group.cap -net $input_path/mempool_group.net -output $output_PR_path/mempool_group.PR_output > $output_log_path/mempool_group.log
echo "Running nvdla"
./route -cap $input_path/nvdla.cap -net $input_path/nvdla.net -output $output_PR_path/nvdla.PR_output > $output_log_path/nvdla.log
echo "Running bsg_chip"
./route -cap $input_path/bsg_chip.cap -net $input_path/bsg_chip.net -output $output_PR_path/bsg_chip.PR_output > $output_log_path/bsg_chip.log
echo "Running ariane133_68"
./route -cap $input_path/ariane133_68.cap -net $input_path/ariane133_68.net -output $output_PR_path/ariane133_68.PR_output > $output_log_path/ariane133_68.log
echo "Running ariane133_51"
./route -cap $input_path/ariane133_51.cap -net $input_path/ariane133_51.net -output $output_PR_path/ariane133_51.PR_output > $output_log_path/ariane133_51.log
# echo "Running mempool_cluster"
# ./route -cap $input_path/mempool_cluster.cap -net $input_path/mempool_cluster.net -output $output_PR_path/mempool_cluster.PR_output > $output_log_path/mempool_cluster.log
# echo "Running mempool_cluster_large"
# ./route -cap $input_path/mempool_cluster_large.cap -net $input_path/mempool_cluster_large.net -output $output_PR_path/mempool_cluster_large.PR_output > $output_log_path/mempool_cluster_large.log

cd ..
cd benchmark/evaluation_script
bash evaluation.sh