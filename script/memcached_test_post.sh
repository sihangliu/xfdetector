#!/bin/bash
#echo $1
./memcached_send_req.sh $1 | telnet localhost 11212
