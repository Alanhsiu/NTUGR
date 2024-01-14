data_list=("ariane133_51")
input_path="/home/scratch.rliang_hardware/ISPD24_benchmark/nangate45/log"
output_path="/home/scratch.rliang_hardware/ISPD24_benchmark/nangate45/log"
for data in "${data_list[@]}"
do
    # run the whole framework
    echo "data: $data"
    ./evaluator $input_path/$data.cap $input_path/$data.net $output_path/$data.PR_output
done

