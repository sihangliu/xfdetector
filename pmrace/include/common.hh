#ifndef COMMON_HH
#define COMMON_HH

//#define DEBUG_ENABLE

#include <cstdint>
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <string.h>
#include <assert.h>
#include <string>
#include <fstream>

using std::cout;
using std::cerr;
using std::endl; 
using std::string;
using std::unordered_map;
using std::vector;
using std::queue;

// FIFO names
#define PRE_FAILURE_FIFO "/tmp/pre_fifo"
#define POST_FAILURE_FIFO "/tmp/post_fifo"
#define SIGNAL_FIFO "/tmp/signal_fifo"

// Number of buffer entries
#define PIN_FIFO_BUF_SIZE (1024 * sizeof(trace_entry_t))

// Signals for inter-process communication
#define MAX_SIGNAL_LEN 100
#define PIN_CONTINUE_SIGNAL "PIN_CONTINUE"
// #define PIN_SUSPEND_SIGNAL "PIN_SUSPEND"
// #define PIN_TERMINATE_SIGNAL "PIN_TERMINATE"

// #define MAX_WORKER 8

#define PM_ADDR_BASE 0x10000000000
#define PM_ADDR_SIZE 0x10000000000

#define COMPLETE 1
#define INCOMPLETE 0

#define PRE_FAILURE 1
#define POST_FAILURE 2

#define MAX_COMMAND_LEN 200
#define MAX_FILE_NAME_LEN 100

#define POST_FAILURE_EXEC_TIMEOUT 20

typedef uint64_t addr_t;
typedef uint64_t size_t;
typedef int timestamp_t;

#define ERR(message) {cerr << message << " (Error code: " << errno << ")" << endl; assert(0);}

#ifdef DEBUG_ENABLE
// Enable output for debugging
#define DEBUG(output) output
#else
// Disable output
#define DEBUG(output) {}
#endif

// #define FILE_NAME_LEN 20

// void cstr_copy(char* dst, char* src)
// {
//     strncpy(dst, src, FILE_NAME_LEN-1);
//     dst[FILE_NAME_LEN-1] = '\0';
// }

#endif
