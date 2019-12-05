#!/bin/bash
set -x

# Workload
WORKLOAD=redis
TESTSIZE=$1

# PM Image file
PMIMAGE=/mnt/pmem0/${WORKLOAD}
TEST_ROOT="$PWD"/..

# variables to use
PMRACE_EXE=${TEST_ROOT}/pmrace/build/app/pmrace
PINTOOL_SO=${TEST_ROOT}/pmrace/pintool/obj-intel64/pintool.so
REDIS_SERVER=${TEST_ROOT}/redis-nvml/src/redis-server
REDIS_TEST=${TEST_ROOT}/script/redis_test.sh
REDIS_TEST_POST=${TEST_ROOT}/script/redis_test_post.sh
PIN_EXE=${TEST_ROOT}/pin-3.10/pin

if ! [[ $TESTSIZE =~ ^[0-9]+$ ]] ; then
   echo -e "${RED}Error:${NC} Invalid workload size ${TESTSIZE}." >&2; usage; exit 1
fi

echo Running ${WORKLOAD}. Test size = ${TESTSIZE}.

# Remove old pmimage and fifo files
#rm -f /mnt/pmem0/*
rm /tmp/*fifo
rm -f /tmp/func_map

TIMING_OUT=${WORKLOAD}_${TESTSIZE}_time.txt
DEBUG_OUT=${WORKLOAD}_${TESTSIZE}_debug.txt

# Generate config file
CONFIG_FILE=${WORKLOAD}_${TESTSIZE}_config.txt
rm ${CONFIG_FILE}
echo "PINTOOL_PATH ${PINTOOL_SO}" >> ${CONFIG_FILE}
echo "EXEC_PATH ${REDIS_SERVER}" >> ${CONFIG_FILE}
echo "PM_IMAGE ${PMIMAGE}" >> ${CONFIG_FILE}
echo "PRE_FAILURE_COMMAND ${REDIS_SERVER} ${TEST_ROOT}/redis-nvml/redis.conf pmfile ${PMIMAGE} 8mb" >> ${CONFIG_FILE}
# The post-failure program should self-terminate without any input from the client.
echo "POST_FAILURE_COMMAND ${REDIS_SERVER} ${TEST_ROOT}/redis-nvml/redis_post.conf pmfile ${PMIMAGE} 8mb & (sleep 10 ; ${REDIS_TEST_POST} ${TESTSIZE} 7) ; wait" >> ${CONFIG_FILE}

# Init the pmImage
${REDIS_SERVER} ${TEST_ROOT}/redis-nvml/redis.conf pmfile ${PMIMAGE} 8mb & (sleep 5 ; ${REDIS_TEST} ${TESTSIZE} 8)
wait

# Run realworkload
# Start PMRace
${PMRACE_EXE} ${CONFIG_FILE} > ${TIMING_OUT} 2> ${DEBUG_OUT} &
sleep 1
${PIN_EXE} -t ${PINTOOL_SO} -t 1 -f 1 -- ${REDIS_SERVER} ${TEST_ROOT}/redis-nvml/redis.conf pmfile ${PMIMAGE} 8mb  &
sleep 10
${REDIS_TEST} ${TESTSIZE} 9
wait

# print the output
cat ${DEBUG_OUT}
cat ${TIMING_OUT} | grep "Total-failure time"
