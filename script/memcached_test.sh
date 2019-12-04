#!/bin/bash
#echo $1
$(dirname $0)/memcached_send_req.sh $1 | telnet localhost 11211
