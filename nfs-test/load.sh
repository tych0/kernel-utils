#!/bin/bash

i=0
while true; do
    echo $i $(date) >> /nfs/load.txt
    i=$(($i+1))
    sleep 1s
done
