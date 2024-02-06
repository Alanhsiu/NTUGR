# mkdir -p build && cd build && cmake ../src && make
# ./route -cap ../input/ariane133_51.cap -net ../input/ariane133_51.net -output ../output/ariane133_51.PR_output -threads 8
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../src && make
./route -cap ../input/ariane133_51.cap -net ../input/ariane133_51.net -output ../output/ariane133_51.PR_output -threads 8