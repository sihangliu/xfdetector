#!/bin/bash
#echo $1
./redis_send_req.sh $1 $2 | telnet localhost 6379
