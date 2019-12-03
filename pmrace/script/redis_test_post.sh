#!/bin/bash
#echo $1
./redis_read.sh $1 $2 | telnet localhost 6380
