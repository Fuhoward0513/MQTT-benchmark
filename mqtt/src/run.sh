#!/bin/bash

sample_t_lst=(10 20 33 100 1000)
payload_lst=(8 80 200 500 1000 2000 1000000 5000000 10000000 20000000 30000000 50000000)
pub_num_lst=(1 2 5 10)
sub_num_lst=(1 2 5 10)

for pub_num in "${pub_num_lst[@]}"
do
    for sub_num in "${sub_num_lst[@]}"
    do
        for sample_t in "${sample_t_lst[@]}"
        do
            for payload in "${payload_lst[@]}"
            do
                # run (1 v N) v M
                for ((i=0; i<pub_num; i++)); do
                    ./MQTTAsync_publish "$sample_t" "$payload" "$i" &
                    ./MQTTAsync_subscribe "$sample_t" "$payload" "$sub_num" "$i" &
                done
                ./system_status/sys_status "$payload" "$sample_t" "$sub_num" "$pub_num" &
                wait

                # conbine the csv file
                python3 file.py $sub_num $pub_num $sample_t $payload    
            done
        done
    done
done