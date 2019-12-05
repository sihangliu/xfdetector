all: pmrace pmdk driver redis memcached

pmrace:
	export PIN_ROOT=$(shell pwd)/pin-3.10 && make -C pmrace

pmdk:
	make -C pmdk EXTRA_CFLAGS="-Wno-error"

driver:
	make -C driver

redis:
	./init_redis.sh

memcached:
	cd memcached-pmem && env LIBS="-levent -L$(shell pwd)/pmrace/build/lib/ -Wl,-rpath=$(shell pwd)/pmrace/build/lib/ -lpmrace_interface" CFLAGS="-I$(shell pwd)/pmrace/include" ./configure --enable-pslab
	make -C memcached-pmem

clean:
	make clean -C pmrace
	make clean -C pmdk
	make clean -C driver
	rm -rf redis-nvml
	make clean -C memcached-pmem

.PHONY: clean pmrace pmdk driver redis memcached
