#!/bin/bash
echo "converting input to binary format..."
./cli convert images/512x512.bmp 512 512 images/512x512.bin


rm -rf ps.log
source monitor.sh log-mmap-${1}.out&
monitor=$!
echo "monitor running with pid: $monitor"


echo $(date)

mkdir testout

echo "starting convolutions..."
for i in $(eval echo {1..${1}})
do
    ./cli mmap images/512x512.bin 512 512 testout/out-${i}.bin&
    pids[${i}]=$!
    echo $(date)
done

echo "waiting for multiplies"
# wait for all pids
for pid in ${pids[*]}; do
    wait $pid
done
echo $(date)

rm -rf testout
kill $monitor