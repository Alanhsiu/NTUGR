mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../src && make
./route -cap ../input/example.cap -net ../input/example.net -output ../output/example.PR_output -threads 8