all: xfdetector pmdk driver redis memcached

xfdetector:
	export PIN_ROOT=$(shell pwd)/pin-3.10 && make -C xfdetector

pmdk:
	make -C pmdk EXTRA_CFLAGS="-Wno-error"

driver:
	make -C driver

redis:
	./init_redis.sh

memcached:
	cd memcached-pmem && env LIBS="-levent -L$(shell pwd)/xfdetector/build/lib/ -Wl,-rpath=$(shell pwd)/xfdetector/build/lib/ -lxfdetector_interface" CFLAGS="-I$(shell pwd)/xfdetector/include" ./configure --enable-pslab
	make -C memcached-pmem

clean:
	make clean -C xfdetector
	make clean -C pmdk
	make clean -C driver
	rm -rf redis-nvml
	make clean -C memcached-pmem

.PHONY: clean xfdetector pmdk driver redis memcached
