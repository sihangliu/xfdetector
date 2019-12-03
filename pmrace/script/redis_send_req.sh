#!/bin/bash
for i in `seq 1 "$1"` ; do
	echo "set g"$i"b"$2" $i"
	#echo "ALOHA!!!!!!"
	#echo "get g$((1))"
done
sleep 1
echo "shutdown save"


