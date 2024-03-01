mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../src && make

echo "Running mempool_tile"
./route -cap ../input/mempool_tile.cap -net ../input/mempool_tile.net -output /docker_data/b09901066/output/mempool_tile.PR_output > ../output/mempool_tile.log
echo "Running mempool_group"
./route -cap ../input/mempool_group.cap -net ../input/mempool_group.net -output /docker_data/b09901066/output/mempool_group.PR_output > ../output/mempool_group.log
echo "Running nvdla"
./route -cap ../input/nvdla.cap -net ../input/nvdla.net -output /docker_data/b09901066/output/nvdla.PR_output > ../output/nvdla.log
echo "Running bsg_chip"
./route -cap ../input/bsg_chip.cap -net ../input/bsg_chip.net -output /docker_data/b09901066/output/bsg_chip.PR_output > ../output/bsg_chip.log
echo "Running ariane133_68"
./route -cap ../input/ariane133_68.cap -net ../input/ariane133_68.net -output /docker_data/b09901066/output/ariane133_68.PR_output > ../output/ariane133_68.log
echo "Running ariane133_51"
./route -cap ../input/ariane133_51.cap -net ../input/ariane133_51.net -output /docker_data/b09901066/output/ariane133_51.PR_output > ../output/ariane133_51.log
echo "Running mempool_cluster"
./route -cap ../input/mempool_cluster.cap -net ../input/mempool_cluster.net -output /docker_data/b09901066/output/mempool_cluster.PR_output > ../output/mempool_cluster.log
# echo "Running mempool_cluster_large"
# ./route -cap ../input/mempool_cluster_large.cap -net ../input/mempool_cluster_large.net -output ../output/mempool_cluster_large.PR_output > ../output/mempool_cluster_large.log

cd ..
cd benchmark/evaluation_script
bash evaluation.sh