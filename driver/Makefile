TEST_ROOT=../

all: data_store data_store_hash 

data_store: data_store.c
	gcc -I${TEST_ROOT}/pmdk/src/examples/libpmemobj/map/ -I${TEST_ROOT}/pmdk/src/examples/libpmemobj/hashmap/ -I${TEST_ROOT}/pmdk/src/examples/ -I${TEST_ROOT}/pmdk/src/include/ -I ${TEST_ROOT}/pmrace/include data_store.c ${TEST_ROOT}/pmrace/src/pmrace_interface.c ${TEST_ROOT}/pmdk/src/examples/libpmemobj/map/libmap.a ${TEST_ROOT}/pmdk/src/debug/libpmemobj.a ${TEST_ROOT}/pmdk/src/debug/libpmem.a -lndctl -ldaxctl -lpthread -ldl -o data_store

data_store_hash: data_store_hash.c
	gcc -I${TEST_ROOT}/pmdk/src/examples/libpmemobj/map/ -I${TEST_ROOT}/pmdk/src/examples/libpmemobj/hashmap/ -I${TEST_ROOT}/pmdk/src/examples/ -I${TEST_ROOT}/pmdk/src/include/ -I ${TEST_ROOT}/pmrace/include data_store_hash.c ${TEST_ROOT}/pmrace/src/pmrace_interface.c ${TEST_ROOT}/pmdk/src/examples/libpmemobj/map/libmap.a ${TEST_ROOT}/pmdk/src/debug/libpmemobj.a ${TEST_ROOT}/pmdk/src/debug/libpmem.a -lndctl -ldaxctl -lpthread -ldl -o data_store_hash

clean:
	rm -f data_store data_store_hash