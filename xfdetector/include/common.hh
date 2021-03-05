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
#include <stdarg.h>

using std::cout;
using std::cerr;
using std::endl; 
using std::string;
using std::unordered_map;
using std::vector;
using std::queue;

// FIFO names
#define PRE_FAILURE_FIFO "pre_fifo"
#define POST_FAILURE_FIFO "post_fifo"
#define SIGNAL_FIFO "signal_fifo"

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

/* Timeout values in seconds */
#define POST_FAILURE_EXEC_TIMEOUT 15
#define PRE_FAILURE_FIFO_TIMEOUT 15

typedef uint64_t addr_t;
typedef uint64_t size_t;
typedef int timestamp_t;

#define ERR(message) {cerr << message << " (Error code: " << errno << ")" << endl; abort();}

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

inline char* alloc_print(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char* out = (char*)malloc(sizeof(char) * 1024);
    vsprintf(out, format, args);
    return out;
}

static inline char** str2cmd(string str)
{
    char** cmd = (char**)malloc(sizeof(char*) * 1024);
    string tmp;
    int cnt = 0;
    for (unsigned i = 0; i < str.length(); ++i) {
        if (str[i] == ' ') {
            cmd[cnt++] = alloc_print(tmp.c_str());
            tmp.clear();
            continue;
        }
        tmp+=str[i];
    }
    cmd[cnt++] = alloc_print(tmp.c_str());
    cmd[cnt++] = NULL;
    return cmd; 
}

static inline string cmd2str(char** cmd) {
    string out;
    int cnt = 0;
    while(cmd[cnt]) {
        out += string(cmd[cnt++]);
        out += string(" ");
    }
    return out;
}

// Check whether input address is on PM
#define isPmemAddr(addr, size) \
    (((uint64_t)addr + size < PM_ADDR_BASE + PM_ADDR_SIZE) \
        && ((uint64_t)addr >= PM_ADDR_BASE))

#endif
