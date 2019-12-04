#!/bin/bash
set -x
RED='\033[0;31m'
GRN='\033[0;32m'
NC='\033[0m' # No Color

usage()
{
    echo ""
    echo "  Usage: ./run.sh WORKLOAD TESTSIZE [PATCH]"
    echo ""
    echo "    WORKLOAD:   The workload to test."
    echo "    TESTSIZE:   The size of workload to test. Usually 10 will be sufficient for reproducing all bugs."
    echo "    PATCH:      The name of the patch that reproduces bugs for WORKLOAD. If not specified, then we test the original program without bugs."
    echo ""
}

if [[ $1 == "-h" ]]; then
    usage; exit 1
fi


# Workload
WORKLOAD=$1
# Sizes of the workloads
TESTSIZE=$2
# patchname
PATCH=$3

# PM Image file
PMIMAGE=/mnt/pmem0/${WORKLOAD}
TEST_ROOT=../

# variables to use
PMRACE_EXE=${TEST_ROOT}/pmrace/build/app/pmrace
PINTOOL_SO=${TEST_ROOT}/pmrace/pintool/obj-intel64/pintool.so
DATASTORE_EXE=${TEST_ROOT}/driver/data_store
PIN_EXE=${TEST_ROOT}/pin-3.10/pin

TIMING_OUT=${WORKLOAD}_${TESTSIZE}_time.txt
DEBUG_OUT=${WORKLOAD}_${TESTSIZE}_debug.txt

if ! [[ $TESTSIZE =~ ^[0-9]+$ ]] ; then
   echo -e "${RED}Error:${NC} Invalid workload size ${TESTSIZE}." >&2; usage; exit 1
fi

if [[ ${WORKLOAD} =~ ^(btree|ctree|rbtree)$ ]]; then
    if [[ ${PATCH} != "" ]]; then
        WORKLOAD_LOC=${TEST_ROOT}/pmdk/src/examples/libpmemobj/tree_map/${WORKLOAD}_map.c
        PATCH_LOC=${TEST_ROOT}/patch/${WORKLOAD}_${PATCH}.patch
        echo "Applying bug patch: ${WORKLOAD}_${PATCH}.patch."
        patch ${WORKLOAD_LOC} < ${PATCH_LOC} || exit 1
    fi
elif [[ ${WORKLOAD} =~ ^(hashmap_atomic|hashmap_tx)$ ]]; then
    if [[ ${PATCH} != "" ]]; then
        WORKLOAD_LOC=${TEST_ROOT}/pmdk/src/examples/libpmemobj/hashmap/${WORKLOAD}.c
        PATCH_LOC=${TEST_ROOT}/patch/${WORKLOAD}_${PATCH}.patch
        echo "Applying bug patch: ${WORKLOAD}_${PATCH}.patch."
        patch ${WORKLOAD_LOC} < ${PATCH_LOC} || exit 1
    fi
else
    echo -e "${RED}Error:${NC} Invalid workload name ${WORKLOAD}." >&2; usage; exit 1
fi

echo Running ${WORKLOAD}. Test size = ${TESTSIZE}.

# Generate config file
CONFIG_FILE=${WORKLOAD}_${TESTSIZE}_config.txt
rm -f ${CONFIG_FILE}
echo "PINTOOL_PATH ${PINTOOL_SO}" >> ${CONFIG_FILE}
echo "EXEC_PATH ${DATASTORE_EXE}" >> ${CONFIG_FILE}
echo "PM_IMAGE ${PMIMAGE}" >> ${CONFIG_FILE}
echo "PRE_FAILURE_COMMAND ${DATASTORE_EXE} ${WORKLOAD} ${PMIMAGE} ${TESTSIZE}" >> ${CONFIG_FILE}
echo "POST_FAILURE_COMMAND ${DATASTORE_EXE} ${WORKLOAD} ${PMIMAGE} 2" >> ${CONFIG_FILE}

# Remove old pmimage and fifo files
rm -f /mnt/pmem0/*
rm -f /tmp/*fifo
rm -f /tmp/func_map

echo "Recompiling workload, suppressing make output."
make clean -C ${TEST_ROOT}/pmdk/src/examples -j > /dev/null
make -C ${TEST_ROOT}/pmdk/src/examples EXTRA_CFLAGS="-Wno-error" -j > /dev/null
make clean -C ${TEST_ROOT}/driver > /dev/null
make -C ${TEST_ROOT}/driver > /dev/null

# unapply patch
if [[ $PATCH != "" ]]; then
    echo "Reverting patch: ${WORKLOAD}_${PATCH}.patch."
    patch -R ${WORKLOAD_LOC} < ${PATCH_LOC} || exit 1
fi

MAX_TIMEOUT=2000

# Init the pmImage
# ${DATASTORE_EXE} ${WORKLOAD} ${PMIMAGE} ${TESTSIZE}
${DATASTORE_EXE} ${WORKLOAD} ${PMIMAGE} 1
# Run realworkload
# Start PMRace
echo -e "${GRN}Info:${NC} We kill the post program after running some time, so don't panic if you see a process gets killed."
timeout ${MAX_TIMEOUT} ${PMRACE_EXE} ${CONFIG_FILE} > ${TIMING_OUT} 2> ${DEBUG_OUT} &
sleep 1
timeout ${MAX_TIMEOUT} ${PIN_EXE} -t ${PINTOOL_SO} -o pmrace.out -t 1 -f 1 -- ${DATASTORE_EXE} ${WORKLOAD} ${PMIMAGE} ${TESTSIZE} > /dev/null
wait
