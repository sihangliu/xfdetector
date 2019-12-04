all: data_store data_store_hash 

data_store: data_store.c
	gcc -Bstatic -I../../pmdk/src/examples/libpmemobj/map/ -I../../pmdk/src/examples/libpmemobj/hashmap/ -I../../pmdk/src/examples/ -I../../pmdk/src/include/ -I ../../pmrace/include data_store.c ../../pmrace/src/pmrace_interface.c ../../pmdk/src/examples/libpmemobj/map/libmap.a ../../pmdk/src/debug/libpmemobj.a ../../pmdk/src/debug/libpmem.a -lndctl -ldaxctl -lpthread -ldl -o data_store

data_store_hash: data_store_hash.c
	gcc -Bstatic -I../../pmdk/src/examples/libpmemobj/map/ -I../../pmdk/src/examples/libpmemobj/hashmap/ -I../../pmdk/src/examples/ -I../../pmdk/src/include/ -I ../../pmrace/include data_store_hash.c ../../pmrace/src/pmrace_interface.c ../../pmdk/src/examples/libpmemobj/map/libmap.a ../../pmdk/src/debug/libpmemobj.a ../../pmdk/src/debug/libpmem.a -lndctl -ldaxctl -lpthread -ldl -o data_store_hash

clean:
	rm -f data_store data_store_hash













#hashmap_tx: hashmap_driver.c
#	gcc -I../../pmdk/src/examples/libpmemobj/map/ -I../../pmdk/src/include/  hashmap_driver.c ../../pmdk/src/examples/libpmemobj/map/libmap_hashmap_tx.a -o hashmap_tx

#hashmap_atomic: hashmap_driver.c
#	gcc -I../../pmdk/src/examples/libpmemobj/map/ -I../../pmdk/src/include/  hashmap_driver.c ../../pmdk/src/examples/libpmemobj/hashmap/libmap_hashmap_atomic.a -o hashmap_atomic

#clean:
#	rm hashmap_tx hashmap_atomic
