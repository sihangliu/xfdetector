#!/bin/bash
#echo $1
$(dirname $0)/redis_read.sh $1 $2 | telnet localhost 6380
