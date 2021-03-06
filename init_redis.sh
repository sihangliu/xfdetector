#!/bin/bash

#   Clone from git and apply patch if not exist, otherwise directly compile
if [[ ! -d "redis-nvml" ]]; then
	git clone https://github.com/pmem/redis.git redis-nvml
	cd redis-nvml
	# Change to PMDK version of Redis (PMDK was named as NVML)
	git checkout 3.2-nvml
	# Apply XFDetector's patch
	git apply ../patch/redis_xfdetector.patch
	# Add PMDK to redis dependency folder
	ln -s ../../pmdk/ ./deps/pmdk
	# Add config for XFDetector post-failure process
	sed -i 's/pmfile ~\/redis.pm 1gb/#pmfile ~\/redis.pm 1gb/' redis.conf
	cp redis.conf redis_post.conf
	sed -i 's/port 6379/port 6380/' redis_post.conf
else
	cd redis-nvml
fi
# Compile with PMDK enabled
make USE_PMDK=yes STD=-std=gnu99
