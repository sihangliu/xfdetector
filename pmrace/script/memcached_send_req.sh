#!/bin/bash
for i in `seq 1 "$1"` ; do
	echo "set g"$i" 1 0 11"
	echo "ALOHA!!!!!!"
	#echo "get g$((1))"
done
echo "shutdown"
sleep 1

