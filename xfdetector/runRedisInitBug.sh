#!/bin/bash
# set -x

if [[ ${PIN_ROOT} == "" ]]; then
    echo  -e "${RED}Error:${NC} Environment variable PIN_ROOT not set. Please specify the full path." >&2; exit 1
fi

# Workload
WORKLOAD=redis
TESTSIZE=$1

# PM Image file
PMIMAGE=/mnt/pmem0/${WORKLOAD}
TEST_ROOT="$PWD"/..

# variables to use
PMRACE_EXE=${TEST_ROOT}/xfdetector/build/app/xfdetector
PINTOOL_SO=${TEST_ROOT}/xfdetector/pintool/obj-intel64/pintool.so
REDIS_SERVER=${TEST_ROOT}/redis-nvml/src/redis-server
REDIS_TEST=${TEST_ROOT}/script/redis_test.sh
REDIS_TEST_POST=${TEST_ROOT}/script/redis_test_post.sh
PIN_EXE=${TEST_ROOT}/pin-3.10/pin

if ! [[ $TESTSIZE =~ ^[0-9]+$ ]] ; then
   echo -e "${RED}Error:${NC} Invalid workload size ${TESTSIZE}." >&2; usage; exit 1
fi

echo Running ${WORKLOAD}. Test size = ${TESTSIZE}.

# Remove old pmimage and fifo files
rm -f /mnt/pmem0/*
rm -f /tmp/*fifo
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
echo "POST_FAILURE_COMMAND ${REDIS_SERVER} ${TEST_ROOT}/redis-nvml/redis_post.conf pmfile ${PMIMAGE} 8mb & (sleep 10 ; ${REDIS_TEST_POST} ${TESTSIZE} \${RANDOM}) ; wait" >> ${CONFIG_FILE}

export PMEM_MMAP_HINT=0x10000000000

ORIGINAL_PATCH=${TEST_ROOT}/patch/redis_xfdetector.patch
BUGGY_PATCH=${TEST_ROOT}/patch/redis_init_bug.patch

# unapply the correct patch, apply the buggy patch and recompile
echo "Applying bug patch: redis_init_bug.patch."
cd ${TEST_ROOT}/redis-nvml
git apply -R ${ORIGINAL_PATCH} || exit 1
git apply ${BUGGY_PATCH} || exit 1
cd -

echo "Recompiling workload, suppressing make output."
make -C ${TEST_ROOT}/redis-nvml USE_PMDK=yes STD=-std=gnu99 -j > /dev/null

echo "Reverting patch: redis_init_bug.patch."
cd ${TEST_ROOT}/redis-nvml
git apply -R ${BUGGY_PATCH} || exit 1
git apply ${ORIGINAL_PATCH} || exit 1
cd -

MAX_TIMEOUT=120

# Run realworkload
# Start XFDetector
(timeout ${MAX_TIMEOUT} ${PMRACE_EXE} ${CONFIG_FILE} | tee ${TIMING_OUT}) 3>&1 1>&2 2>&3 | tee ${DEBUG_OUT} &
sleep 1
timeout ${MAX_TIMEOUT} ${PIN_EXE} -t ${PINTOOL_SO} -t 1 -f 1 -- ${REDIS_SERVER} ${TEST_ROOT}/redis-nvml/redis.conf pmfile ${PMIMAGE} 8mb &
sleep 10
${REDIS_TEST} ${TESTSIZE} ${RANDOM}
wait

echo "Recompiling workload, suppressing make output."
make -C ${TEST_ROOT}/redis-nvml USE_PMDK=yes STD=-std=gnu99 -j > /dev/null
