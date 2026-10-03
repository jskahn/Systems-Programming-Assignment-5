#!/bin/bash

rm -rf ps.log


source monitor.sh log-normal-${1}.out&
monitor=$!
echo "monitor running with pid: $monitor"


echo $(date)
echo "starting convolutions..."

mkdir testout

for i in $(eval echo {1..${1}})
do
    ./cli kernel images/512x512.bmp 512 512 testout/out-${i}.bin&
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