## Cross-Failure Bug Detection in Persistent Memory Programs
Sihang Liu, Korakit Seemakhupt, Yizhou Wei, Thomas Wenisch, Aasheesh Kolli, and Samira Khan <br/>
The International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS), 2020

## Table of Contents
* [Introduction to XFDetector](#introduction-to-xfdetector)
* [Prerequisites](#prerequisites)
  * [Hardware Dependencies](#hardware-dependencies)
  * [Software Dependencies](#software-dependencies)
* [Installation](#installation)
  * [Build XFDetector](#build-xfdetector)
  * [Build Test Workloads](#build-test-workloads)
* [Testing and Reproducing Bugs](#testing-and-reproducing-bugs)
  * [PMDK Examples](#pmdk-examples)
  * [Redis](#redis)
  * [Memcached](#memcached)
  * [Testing Other Workloads](#testing-other-workloads)
  

## Introduction to XFDetector
Persistent memory (PM) technologies, such as Intel's Optane memory, deliver high performance, byte-addressability and persistence, allowing program to directly manipulate persistent data in memory without OS overhead. 
An important requirement of these programs is that persistent data must remain consistent across a failure, which we refer to as the crash consistency guarantee.

However, maintaining crash consistency is not trivial --- consistency depends critically on the order of persistent memory access in both *pre-failure* and *post-failure* execution. 
Because hardware may reorder persistent memory accesses both before and after failure, validation of crash-consistent programs requires holistic analysis of both execution stages.
We categorize the underlying causes behind inconsistent recovery due to incorrect interaction between the pre-failure and post-failure execution. First, a program is not crash consistent 
if the post-failure stage reads from locations that are not guaranteed to have persisted in all possible access interleavings during the pre-failure stage --- an error that we refer to as a *cross-failure race*.
Second, a program is not crash consistent 
if the post-failure stage reads persistent data that  has been left semantically inconsistent during the pre-failure stage, such as a stale log or uncommitted data, which we call a *cross-failure semantic bug*.
Together, we call these *cross-failure bugs*. 
In this work, we propose XFDetector, a tool that detects cross-failure bugs by considering failures injected at all ordering points in pre-failure execution and checking for cross-failure races and cross-failure semantic bugs in the post-failure continuation.
XFDetector has detected four new bugs in three pieces of PM software: one of PMDK examples, a PM-optimized Redis database and a  PMDK library function.

## Prerequisites

### Hardware Dependencies
XFDetector targets workloads that directly manage the persistent data on PM through a DAX file system. To support these workloads, the hardware needs to provide a persistent memory device, either by using real PM (Intel Optane DC Persistent Memory) or emulating PM with DRAM. To enable efficient writeback of persistent data, the processor also requires the CLWB instruction in both real and emulated platforms. 

The following is a list of requirements:
#### Real PM system 
* CPU: Intel 2nd Generation Xeon Scalable Processor (Gold or Platinum)
* Memory: Intel Optane DC Persistent Memory (at least 1x 128GB DIMM) and DDR4 RDIMM (at least 1x 16GB DMM).

Please refer to Intel's [guide](https://software.intel.com/en-us/articles/quick-start-guide-configure-intel-optane-dc-persistent-memory-on-linux) to initialize DC persistent memory in App Direct mode. In the rest of this documentation, we assume the PM device is mounted on `/mnt/pmem0`.

#### Emulated PM system
* CPU: Intel 1st/2nd Generation Xeon Scalable Processor
* Memory: at least DDR4 32GB (16GB of which will be emulated as PM)

Steps for PM emulation can be found at [PMDK's website](https://pmem.io/2016/02/22/pm-emulation.html).
We have tested XFDetector on a system with real PM. There can be unexpected behavior in emulated systems. 

### Software Dependencies
The following is a list of software dependencies for XFDetector and test workloads (the listed versions have been tested, other versions might work but not guaranteed):   
* OS: Ubuntu 18.04 (kernel 4.15)   
* Compiler: g++/gcc-7
* Libraries: libboost-1.65 (`libboost-all-dev`), pkg-config (`pkg-config`), ndctl-61.2 (`libndctl-dev`), daxctl-61.2 (`libdaxctl-dev`), autoconf (`autoconf`), libevent (`libevent-dev`).

Other dependent libraries for the workloads are contained in this repository. 

## Installation
This repository is organized as the following structure:
* `xfdetector/`: The source code of our tool.
* `driver/`: The modified driver function of PMDK examples.
* `pmdk/`: Intel's [PMDK](https://pmem.io/) library, including its example PM programs.
* `redis-nvml/`: A Redis implementation (from [Intel](https://github.com/pmem/redis/tree/3.2-nvml)) based on PMDK (PMDK was previously named as NVML).
* `memcached-pmem/`: A Memcached implementation (from [Lenovo](https://github.com/lenovo/memcached-pmem)) based on Intel's PMDK library. 
* `patch/`: Patches for reproducing bugs and trying our tool

This repository provides a Makefile that compiles both XFDetector and test workloads under the root folder. Simply execute:
```
$ export PIN_ROOT=<XFDetector Root>/pin-3.10
$ export PATH=$PATH:$PIN_ROOT
$ export PMEM_MMAP_HINT=0x10000000000
$ make
```
The tests would be runnable when `make` is done. 
`PMEM_MMAP_HINT` is a debugging functionality from PMDK that maps PM to a predefined virtual address. 
Proper exeuction of XFDetector requires that `PMEM_MMAP_HINT`, `PIN_ROOT` and `PATH` are set up.

The followings are the detailed instructions to build XFDetector and workloads separately. 
**If compile all at once, skip the rest steps in Installation.**

### Build XFDetector
```
$ export PIN_ROOT=<XFDetector Root>/pin-3.10
$ export PATH=$PATH:$PIN_ROOT
$ cd <XFDetector Root>/
$ make
```
Note that if you have any missing dependencies of PMDK, you will need to install PMDK by 

```
$ cd <XFDetector Root>/pmdk/
$ sudo make install
```
And then continue `make` in the root directory of XFDetector.

### Build Driver Functions for PMDK Examples
```
$ cd driver/
$ make
```


### PMDK
Compile our annotated PMDK (based on PMDK-1.6) using the following command. You need to specify `EXTRA_CFLAGS="-Wno-error"` for the ease of compilation.
```
$ cd pmdk/
$ make EXTRA_CFLAGS="-Wno-error"
```

Install the compiled PMDK (require sudo privilege):
```
# make install
```

### Redis
Compile Redis using the following script. You can refer to the `README.md` under `redis-nvml/` for more details on compilation.
```
$ ./init-redis.sh
```

### Memcached
Configure the Memcached using the following command. The variable `LIBS` and `CFLAGS` include the XFDetector's interface and the option `--enable-pslab` enables PM as persistent storage. 
```
$ cd memcached-pmem/
$ env LIBS='-levent -L../xfdetector/build/lib/ -Wl,-rpath=../xfdetector/build/lib/ -lxfdetector_interface' CFLAGS='-I../xfdetector/include' ./configure --enable-pslab
```
Compile our annotated Memcached:
```
$ make
```

## Testing and Reproducing Bugs
After compiling the XFDetector suite, we can start testing and reproducing the bugs.

Tests for the following programs are available in XFDetector:
* PMDK program examples:
	* btree
	* ctree
	* rbtree
	* hashmap_tx
	* hashmap_atomic
* Redis
* Memcached

We provide patches for reproducing some of the synthetic bugs that we created and reported in the paper. The scripts for applying the patches and running the buggy programs are under `xfdetector/` folder.

Before running any program, please execute the following commands under the root directory of XFDetector:
```
$ export PIN_ROOT=<XFDetector Root>/pin-3.10
$ export PATH=$PATH:$PIN_ROOT
$ export PMEM_MMAP_HINT=0x10000000000
```

The detailed steps for testing and reproducing bugs on those programs are as follows.

### PMDK Examples
Use script `xfdetector/run.sh` to insert bugs and run all those examples. The usage is shown as follows. The user can also run `./run.sh -h` to check this information.
```
Usage: ./run.sh WORKLOAD INITSIZE TESTSIZE [PATCH]
    WORKLOAD:   The workload to test.
    INITSIZE:   The number of data insertions when initializing the image. This is for fast-forwarding the initialization.
    TESTSIZE:   The number of additional data insertions when reproducing bugs with XFDetector.
    PATCH:      The name of the patch that reproduces bugs for WORKLOAD. If not specified, then we test the original program without bugs.
```
For example, to reproduce the `race1` bug in `btree`, we need to insert 5 elements on both initialization and testing, so we can run the following command:
```
$ ./run.sh btree 5 5 race1
```

For a complete list of all available tests and corresponding parameters that can be used to reproduce the bugs, check `runallPMDK.sh`. You can also directly run the script to run all available tests. (Not recommended, since this will take a long time)

### Redis
Use script `xfdetector/runRedis.sh` to run the Redis example with the initialization bug. The usage is shown as follows.
```
Usage: ./runRedis.sh TESTSIZE
	TESTSIZE:   The size of workload to test.
```
Use script `xfdetector/runRedisNoBug.sh` to run the Redis example without bug. The usage is shown as follows.
```
Usage: ./runRedisNoBug.sh TESTSIZE
	TESTSIZE:   The size of workload to test.
```

### Memcached
Use script `xfdetector/runMemcached.sh` to run Memcached examples. The usage is shown as follows.
```
Usage: ./runMemcached.sh TESTSIZE
	TESTSIZE:   The size of workload to test.
```

### Testing Other Workloads
The interface from XFDetector for annotation is defined in `include/xfdetector_interface.h`.
In these functions, 
field `condition` is a boolean option that enables/disables this function and 
field `stage` can be `PRE_FAILURE`, `POST_FAILURE` or `PRE_FAILURE|POST_FAILURE` (both). 


* Select region-of-interest for testing:
```
void XFDetector_RoIBegin(int condition, int stage);
void XFDetector_RoIEnd(int condition, int stage);
```

* Terminate testing (kill the process):
```
void XFDetector_complete(int condition, int stage);
```

* Add commit variable to detect cross-failure semantic bugs:   
Field `var` is the pointer to the selected commit variable, and field `size` is the size of the commit variable.
```
void XFDetector_addCommitVar(const void* var, unsigned size);
```
<!-- 
* Annotate your own PM libraries:
A pair of the following functions select a code region that skips detection.
```
void skipDetectionBegin(int condition, int stage);
void skipDetectionEnd(int condition, int stage);
``` -->
