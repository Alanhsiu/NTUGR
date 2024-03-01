data_list=("ariane133_51" "ariane133_68"  "bsg_chip" "mempool_tile" "mempool_group" "cluster")
benchmark_path="/workspace/benchmarks"
router_path="/workspace/router"
solution_path="/workspace/solutions"
evaluation_path="/workspace/evaluation"
log_path="/workspace/logs"
mkdir -p $solution_path
mkdir -p $log_path
for data in "${data_list[@]}"
do
    echo "data: $data"
    start=$(date +%s)
    cd $router_path
    $router_path/route  -cap $benchmark_path/$data.cap -net $benchmark_path/$data.net -output $solution_path/$data.output
    end=$(date +%s)
    runtime=$((end-start))
    cd ..
    echo "runtime for $data is $runtime"
    $evaluation_path/evaluator $benchmark_path/$data.cap $benchmark_path/$data.net $solution_path/$data.output
done