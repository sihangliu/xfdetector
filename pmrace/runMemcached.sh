#!/bin/bash
set -x

# Workload
WORKLOAD=memcached
TESTSIZE=$1

# PM Image file
PMIMAGE=/mnt/pmem0/${WORKLOAD}
TEST_ROOT=../

# variables to use
PMRACE_EXE=${TEST_ROOT}/pmrace/build/app/pmrace
PINTOOL_SO=${TEST_ROOT}/pmrace/pintool/obj-intel64/pintool.so
MEMCACHED_EXE=${TEST_ROOT}/memcached-pmem/memcached
MEMCACHED_TEST=${TEST_ROOT}/script/memcached_test.sh
PIN_EXE=${TEST_ROOT}/pin-3.10/pin

if ! [[ $TESTSIZE =~ ^[0-9]+$ ]] ; then
   echo -e "${RED}Error:${NC} Invalid workload size ${TESTSIZE}." >&2; usage; exit 1
fi

echo Running ${WORKLOAD}. Test size = ${TESTSIZE}.

# Remove old pmimage and fifo files
rm -f /mnt/pmem0/*
rm /tmp/*fifo
rm -f /tmp/func_map

TIMING_OUT=${WORKLOAD}_${TESTSIZE}_time.txt
DEBUG_OUT=${WORKLOAD}_${TESTSIZE}_debug.txt

# Generate config file
CONFIG_FILE=${WORKLOAD}_${TESTSIZE}_config.txt
rm ${CONFIG_FILE}
echo "PINTOOL_PATH ${PINTOOL_SO}" >> ${CONFIG_FILE}
echo "EXEC_PATH ${MEMCACHED_EXE}" >> ${CONFIG_FILE}
echo "PM_IMAGE ${PMIMAGE}" >> ${CONFIG_FILE}
echo "PRE_FAILURE_COMMAND ${MEMCACHED_EXE} -A -m 0 -o pslab_file=${PMIMAGE},pslab_force,pslab_recover" >> ${CONFIG_FILE}
# The post-failure program should self-terminate without any input from the client.
echo "POST_FAILURE_COMMAND ${MEMCACHED_EXE} -A -p 11212 -m 0 -o pslab_file=${PMIMAGE},pslab_force,pslab_recover" >> ${CONFIG_FILE}

# Init the pmImage
${MEMCACHED_EXE} -A -p 11211 -m 0 -o pslab_file=${PMIMAGE},pslab_force & ( sleep 1 ; ${MEMCACHED_TEST} 1)
wait

# Run realworkload
# Start PMRace
${PMRACE_EXE} ${CONFIG_FILE} > ${TIMING_OUT} 2> ${DEBUG_OUT} &
sleep 1
${PIN_EXE} -t ${PINTOOL_SO} -t 1 -f 1 -- ${MEMCACHED_EXE} -A -m0 -o pslab_file="${PMIMAGE}",pslab_force,pslab_recover > /dev/null &
sleep 10
${MEMCACHED_TEST} ${TESTSIZE}
wait
