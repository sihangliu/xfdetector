git clone https://github.com/pmem/redis.git redis-nvml
cd redis-nvml
# Change to PMDK version of Redis (PMDK was named as NVML)
git checkout 3.2-nvml
# Apply XFDetector's patch
git apply ../redis_xfdetector.patch
# Add PMDK to redis dependency folder
ln -s ../../pmdk/ ./deps/pmdk
# Compile with PMDK enabled
make USE_PMDK=yes STD=-std=gnu99
# Add config for XFDetector post-failure process
cp redis.conf redis_post.conf
sed -i 's/port 6379/port 6380/' redis_post.conf
