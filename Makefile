all:
	export PIN_ROOT=$(shell pwd)/pin-3.10 && make -C pmrace
	make -C pmdk EXTRA_CFLAGS="-Wno-error"
	make -C driver
	cd memcached-pmem && env LIBS="-levent -L$(shell pwd)/pmrace/build/lib/ -Wl,-rpath=$(shell pwd)/pmrace/build/lib/ -lpmrace_interface" CFLAGS="-I$(shell pwd)/pmrace/include" ./configure --enable-pslab
	make -C memcached-pmem
	make -C redis-nvml USE_PMDK=yes STD=-std=gnu99

clean:
	make clean -C pmrace
	make clean -C pmdk
	make clean -C memcached-pmem
	make clean -C redis-nvml

.PHONY: clean
