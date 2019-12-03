#!/bin/bash
set -x

# Workload
WORKLOAD=redis
# PM Image file
PMIMAGE=/mnt/pmem0/"$WORKLOAD"
TEST_ROOT="$PWD"/..


# Remove old pmimage and fifo files
rm /mnt/pmem0/*
rm /tmp/*fifo

# Sizes of the workloads
TESTSIZES=(1)

for i in ${TESTSIZES[@]} ; do
	TIMING_OUT="$WORKLOAD"_time.txt
	echo "$WORKLOAD" size: "$i" >> $TIMING_OUT
	DEBUG_OUT="$WORKLOAD"_"$i"_debug.txt
	echo "$WORKLOAD" size: "$i" >> $DEBUG_OUT
	echo "runningTestSize $i"
	# Init the pmImage
	"$TEST_ROOT"/redis-nvml/src/redis-server "$TEST_ROOT"/redis-nvml/redis.conf pmfile $PMIMAGE 8mb & (sleep 5 ; ./redis_test.sh $i 7)
	wait
 	# Generate config file
 	CONFIG_FILE="$WORKLOAD"_config_"$i".txt
 	rm $CONFIG_FILE
 	echo "PINTOOL_PATH $TEST_ROOT/pmrace/pintool/obj-intel64/pintool.so" >> $CONFIG_FILE
 	echo "EXEC_PATH $TEST_ROOT/redis-nvml/src/redis-server" >> $CONFIG_FILE
 	echo "PM_IMAGE $PMIMAGE" >> $CONFIG_FILE
 	echo "PRE_FAILURE_COMMAND $TEST_ROOT/redis-nvml/src/redis-server $TEST_ROOT/redis-nvml/redis.conf pmfile $PMIMAGE 8mb" >> $CONFIG_FILE
 	# The post-failure program should self-terminate without any input from the client.
 	echo "POST_FAILURE_COMMAND $TEST_ROOT/redis-nvml/src/redis-server $TEST_ROOT/redis-nvml/redis_post.conf pmfile $PMIMAGE 8mb & (sleep 10 ; ./redis_test_post.sh $i 8) ; wait" >> $CONFIG_FILE
 	# Run realworkload
 	# Start PMRace
 	./build/app/pmrace $CONFIG_FILE >> $TIMING_OUT 2> $DEBUG_OUT &
 	sleep 1
 	pin -t "$TEST_ROOT"/pmrace/pintool/obj-intel64/pintool.so -t 1 -f 1 -- $TEST_ROOT/redis-nvml/src/redis-server $TEST_ROOT/redis-nvml/redis.conf pmfile $PMIMAGE 8mb > /dev/null &
 	sleep 10
 	./redis_test.sh $i 9
 	wait
done
