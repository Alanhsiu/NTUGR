data_list=("ariane133_51")
input_path="/home/b09901066/ISPD-NTUEE/NTUGR/input"
output_path="/home/b09901066/ISPD-NTUEE/NTUGR/output"
for data in "${data_list[@]}"
do
    # run the whole framework
    echo "data: $data"
    ./evaluator $input_path/$data.cap $input_path/$data.net $output_path/$data.PR_output
done

