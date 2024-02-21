mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../src && make
# ./route -cap ../input/mempool_cluster.cap -net ../input/mempool_cluster.net -output ../output/mempool_cluster.PR_output -threads 8
./route -cap ../input/mempool_tile.cap -net ../input/mempool_tile.net -output ../output/mempool_tile.PR_output -threads 8
# ./route -cap ../input/mempool_group.cap -net ../input/mempool_group.net -output ../output/mempool_group.PR_output -threads 8
# ./route -cap ../input/nvdla.cap -net ../input/nvdla.net -output ../output/nvdla.PR_output -threads 8
# ./route -cap ../input/bsg_chip.cap -net ../input/bsg_chip.net -output ../output/bsg_chip.PR_output -threads 8
# ./route -cap ../input/ariane133_68.cap -net ../input/ariane133_68.net -output ../output/ariane133_68.PR_output -threads 8
# ./route -cap ../input/ariane133_51.cap -net ../input/ariane133_51.net -output ../output/ariane133_51.PR_output -threads 8