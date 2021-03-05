#ifndef PMRACE_PINTOOL_HH
#define PMRACE_PINTOOL_HH

#include "../include/trace.hh"
#include "pin.H"
// #include "atomic.hpp"

using std::string;

// #include "trace.hh"

#define MAX_THREADS 32

PIN_MUTEX print_lock;

#ifdef DEBUG_ENABLE
// Enable output for debugging
#define PinDEBUG(output) PIN_MutexLock(&print_lock); \
                         output; \
                         PIN_MutexUnlock(&print_lock)
#else
// Disable output
#define PinDEBUG(output) {}
#endif


enum pm_op_type {
    PM_ALLOCATION_FUNC,
    PM_WRITEBACK_FUNC,
    PM_LIBRARY_FUNC,
    PM_LOW_LEVEL_FUNC
};

struct pm_func_t {
    // char func_name[80]; // Function name
    pm_op_t enum_name; // Enum of function name
    pm_op_type type; // Function type
    int num_args;  // Number of arguments in function
    int src; // Which argument gets data from PM
    int dst; // Which argument updates PM. Return value encode as (num_args+1) th argument
    int size; // Size of operation
};

/* Map of trackable functions for Pin tool */
std::unordered_map<string, pm_func_t> pm_functions;

std::pair<string, pm_func_t> pm_function_entries[] = {
    // name: type, enum_type, num_args, src, dst, size 
    // allocation functions
    {"pmem_map_file", {PMEM_MAP_FILE, PM_ALLOCATION_FUNC, 6, -1, 6, 4}},
    {"pmem_unmap", {PMEM_UNMAP, PM_ALLOCATION_FUNC, 2, 0, -1, 1}},
    // TX annotation functions
    {"pm_trace_pm_addr_add", {PM_TRACE_PM_ADDR_ADD, PM_LIBRARY_FUNC, 2, -1, 0, 1}},
    {"pm_trace_pm_addr_remove", {PM_TRACE_PM_ADDR_REMOVE, PM_LIBRARY_FUNC, 2, 0, -1, 1}},
    {"pm_trace_tx_begin", {PM_TRACE_TX_BEGIN, PM_LIBRARY_FUNC, 0, -1, -1, -1}},
    {"pm_trace_tx_end", {PM_TRACE_TX_END, PM_LIBRARY_FUNC, 0, -1, -1, -1}},
    {"pm_trace_tx_addr_add", {PM_TRACE_TX_ADDR_ADD, PM_LIBRARY_FUNC, 2, -1, 0, 1}},
    {"pm_trace_tx_alloc", {PM_TRACE_TX_ALLOC, PM_LIBRARY_FUNC, 2, -1, 0, 1}},

    // writeback functions
    {"clwb", {CLWB, PM_WRITEBACK_FUNC, 2, 0, -1, 1}},
    {"flush_clwb_nolog", {CLWB, PM_WRITEBACK_FUNC, 2, 0, -1, 1}},
    {"flush_clflush_nolog", {CLWB, PM_WRITEBACK_FUNC, 2, 0, -1, 1}},
    {"flush_clflushopt_nolog", {CLWB, PM_WRITEBACK_FUNC, 2, 0, -1, 1}},
    {"sfence", {SFENCE, PM_WRITEBACK_FUNC, 0, -1, -1, -1}},
    {"predrain_memory_barrier", {SFENCE, PM_WRITEBACK_FUNC, 0, -1, -1, -1}},
    // Low-level semantics
    {"_add_commit_var", {_ADD_COMMIT_VAR, PM_LOW_LEVEL_FUNC, 2, 0, -1, 1}},
    /*
    // writeback functions
    {"pmem_flush", {PMEM_FLUSH, PM_WRITEBACK_FUNC, 2, 0, -1, 1}},
    {"pmem_persist", {PMEM_PERSIST, PM_WRITEBACK_FUNC, 2, 0, -1, 1}},
    {"pmem_drain", {PMEM_DRAIN, PM_WRITEBACK_FUNC, 0, -1, -1, -1}},
    {"pmem_memmove_persist", {PMEM_MEMMOVE_PERSIST, PM_WRITEBACK_FUNC, 3, 1, 0, 2}},
    {"pmem_memcpy_persist", {PMEM_MEMCPY_PERSIST, PM_WRITEBACK_FUNC, 3, 1, 0, 2}},
    {"pmem_memset_persist", {PMEM_MEMSET_PERSIST, PM_WRITEBACK_FUNC, 3, 1, 0, 2}},
    {"pmem_memmove_nodrain", {PMEM_MEMMOVE_NODRAIN, PM_WRITEBACK_FUNC, 3, 1, 0, 2}},
    {"pmem_memcpy_nodrain", {PMEM_MEMCPY_NODRAIN, PM_WRITEBACK_FUNC, 3, 1, 0, 2}},
    {"pmem_memset_nodrain", {PMEM_MEMSET_NODRAIN, PM_WRITEBACK_FUNC, 3, 1, 0, 2}},
    // library functions
    // TX functions for pmemobj
    
    
    // low-level functions for pmemobj
    {"pmemobj_flush", {PMEMOBJ_FLUSH, PM_WRITEBACK_FUNC, 2, 1, -1, 2}},
    {"pmemobj_persist", {PMEMOBJ_PERSIST, PM_WRITEBACK_FUNC, 2, 1, -1, 2}},
    {"pmemobj_drain", {PMEMOBJ_DRAIN, PM_WRITEBACK_FUNC, 0, -1, -1, -1}},
    {"pmemobj_memcpy_persist", {PMEMOBJ_MEMCPY_PERSIST, PM_WRITEBACK_FUNC, 3, 2, 1, 3}},
    {"pmemobj_memset_persist", {PMEMOBJ_MEMSET_PERSIST, PM_WRITEBACK_FUNC, 3, 2, 1, 3}},
    // internal functions for pmemobj
    {"pmemops_xpersist", {PMEMOPS_XPERSIST, PM_WRITEBACK_FUNC, 4, 1, -1, 2}},
    {"pmemops_xflush", {PMEMOPS_XFLUSH, PM_WRITEBACK_FUNC, 4, 1, 1, 2}},
    {"pmemops_drain", {PMEMOPS_DRAIN, PM_WRITEBACK_FUNC, 1, -1, -1, -1}},
    */
    
};

std::unordered_map<string, bool> failure_point_funcs;

char failure_point_funcs_array[][100] = {
    /*
    "pmem_memmove_persist",
    "pmem_memcpy_persist",
    "pmem_memset_persist",
    "pmem_persist",
    "pmem_drain",
    */
    // Manually added failure point
    // "_add_failure_point",
    // We need to add more failure points for TX.
    // Ideally, the failure point should be right before the ordering points (TX_ADD, TX_END).
    // However, we may not be able to use the same dummy function call that is used to determine
    // the boundary of the trasaction as it is not in the same location as the ordering point.

    // We cannot use pm_trace_tx_addr_add because we did not fix the order of function instrument.
    // "pmemobj_tx_add_common", 
    
    // "pmemobj_persist",
    // "tx_commit_point",
    // "pmemobj_tx_commit"
    // // "pmemobj_tx_process",
    /* PMFuzz annotation */
    "pmfuzz_inject_failure"
};

// PIN tool does not support static initialization
// Needs to manually call this function in PIN tool 
void pm_func_init() 
{
    for (unsigned i = 0; i < sizeof(pm_function_entries)/sizeof(std::pair<string, pm_func_t>); ++i) {
        pm_functions[pm_function_entries[i].first] = pm_function_entries[i].second;
    }
    for (unsigned i = 0; i < sizeof(failure_point_funcs_array)/sizeof(failure_point_funcs_array[0]); ++i) {
        // TODO: Using 1 as the value in map for now
        failure_point_funcs[string(failure_point_funcs_array[i])] = 1;
    }
}

class PINFifo {
public:
    int pinfifo_write(trace_entry_t*);
    void pinfifo_close();
    void init(int);
    PINFifo();
    ~PINFifo();
private:
    string pin_fifo_str;
    int pinfifo_open(const char*);
    // int pmfifo_read(trace_entry_t*);
    // Pintool only writes to FIFO
    int fifo_fd;
    PIN_MUTEX fifo_lock;
};

// int PINFifo::pinfifo_create() 
// {
//     return mkfifo(fifo_name, 0666);
// }

int PINFifo::pinfifo_open(const char* fifo_name)
{
    fifo_fd = open(fifo_name, O_RDWR);
    if (fifo_fd < 0)
        ERR("PINFifo open failed.");
    return fifo_fd;
}

void PINFifo::pinfifo_close() 
{
    close(fifo_fd);
}

int PINFifo::pinfifo_write(trace_entry_t* trace) 
{
    // Send trace entry to FIFO only when FIFO is enabled
    if (fifo_enable) {
        //cout << "Trace written" << endl;
        int write_rtn;
        PIN_MutexLock(&fifo_lock);
        write_rtn = write(fifo_fd, trace, sizeof(trace_entry_t));
        PIN_MutexUnlock(&fifo_lock);
        if((unsigned)write_rtn<sizeof(trace_entry_t)){
            cout << "cannot write FIFO" << endl;
            exit(0);
        }
        return write_rtn;
    } else {
        // Write zero byte when FIFO is disabled
        return 0;
    }
}

void PINFifo::init(int stage)
{
    if (stage == PRE_FAILURE) {
        pin_fifo_str = string("/tmp/") + PRE_FAILURE_FIFO + "." + execIDStr;
    } else if (stage == POST_FAILURE) {
        pin_fifo_str = string("/tmp/") + POST_FAILURE_FIFO + "." + execIDStr;
    }
    if (pinfifo_open(pin_fifo_str.c_str()) < 0)
        ERR("PINFifo open failed.");

}

PINFifo::PINFifo()
{
    PIN_MutexInit(&fifo_lock);
}

PINFifo::~PINFifo() 
{
    pinfifo_close();
}

class SignalFifo {
public:
    void init();
    int signal_send(char*, unsigned);
    int signal_recv(char*, unsigned);
    void signalfifo_close();
    SignalFifo();
    ~SignalFifo();
private:
    string signal_fifo_str;
    int signalfifo_open();
    int fifo_fd;
    PIN_MUTEX fifo_lock;
};

void SignalFifo::init()
{
    if (signalfifo_open() < 0) {
        ERR("PINFifo open failed.");
    }
}

int SignalFifo::signal_send(char* message, unsigned len)
{
    PIN_MutexLock(&fifo_lock);
    int num_written = write(fifo_fd, message, len);
    PIN_MutexUnlock(&fifo_lock);

    if (num_written < 0)
        ERR("SignalFifo write failed.");

    return num_written;
}

int SignalFifo::signal_recv(char* buf, unsigned len)
{
    PIN_MutexLock(&fifo_lock);
    int num_read = read(fifo_fd, buf, len);
    PIN_MutexUnlock(&fifo_lock);

    if (num_read < 0)
        ERR("SignalFifo read failed.");

    return num_read;
} 


int SignalFifo::signalfifo_open()
{
    signal_fifo_str = string("/tmp/") + SIGNAL_FIFO + "." + execIDStr;

    fifo_fd = open(signal_fifo_str.c_str(), O_RDWR);
    if (fifo_fd < 0)
        ERR("SignalFifo open failed.");

    return fifo_fd;
}

void SignalFifo::signalfifo_close() 
{
    close(fifo_fd);
}


SignalFifo::SignalFifo()
{
    PIN_MutexInit(&fifo_lock);
    // if (pinfifo_create() < 0) {
    //     ERR("PINFifo create failed.");
    // }
    // if (signalfifo_open() < 0) {
    //     ERR("PINFifo open failed.");
    // }
}

SignalFifo::~SignalFifo() 
{
    signalfifo_close();
}

class ThreadCounter {
public:
    void increment(unsigned);
    void decrement(unsigned);
    int count;
    bool active_thread[MAX_THREADS];
    ThreadCounter();
    // ~TheradCounter();
private:
    PIN_MUTEX counter_lock;
};

void ThreadCounter::increment(unsigned tid)
{
    PIN_MutexLock(&counter_lock);
    count++;
    active_thread[tid] = true;
    PIN_MutexUnlock(&counter_lock);
}

void ThreadCounter::decrement(unsigned tid)
{
    PIN_MutexLock(&counter_lock);
    assert(count > 0);
    count--;
    active_thread[tid] = false;
    PIN_MutexUnlock(&counter_lock);
}

ThreadCounter::ThreadCounter()
{
    PIN_MutexInit(&counter_lock);
    // Initialize thread status
    memset(active_thread, 0, sizeof(MAX_THREADS * sizeof(bool)));
}

class RoITracker {
public:
    bool trySetRoIThread(uint64_t);
    void unsetRoIThread(uint64_t);
    bool isInRoI(uint64_t);
    // bool isRoISet();
    RoITracker();
    // Tracing controlled by passing RoI functions
    // bool trace_enable = false;
    uint64_t roi_tid;

private:
    // Only track one thread for testing
    PIN_MUTEX roi_lock;
    bool roi_tid_set = false;
};

bool RoITracker::trySetRoIThread(uint64_t tid)
{
    // assert(!roi_tid_set && "RoI has been pinned to one thread");
    PIN_MutexLock(&roi_lock);
    if (!roi_tid_set) {
        roi_tid_set = true;
        roi_tid = tid;
        // trace_enable = true;
        PIN_MutexUnlock(&roi_lock);
        return true;
    } else {
        PIN_MutexUnlock(&roi_lock);
        return false;
    }
}

void RoITracker::unsetRoIThread(uint64_t tid)
{
    PIN_MutexLock(&roi_lock);
    if (roi_tid == tid) {
        assert(roi_tid_set && "RoI has not been pinned to one thread");
        roi_tid_set = false;
        // trace_enable = false;
    }
    PIN_MutexUnlock(&roi_lock);
}

bool RoITracker::isInRoI(uint64_t tid)
{
    return (tid == roi_tid && roi_tid_set);
}

RoITracker::RoITracker()
{
    PIN_MutexInit(&roi_lock);
}

/* PMFuzz: Do not need image dump any more */
#if 1
VOID ImageLoad(IMG img, VOID *v)
{   
    if (IMG_IsMainExecutable(img)) {
        int line;
        string path;

        for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
        { 
            for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
            {
                RTN_Open(rtn);

                // if (!INS_Valid(RTN_InsHead(rtn)))
                // {
                //     RTN_Close(rtn);
                //     continue;
                // }
                // PIN_GetSourceLocation(RTN_Address(rtn), NULL, &line, &path);
                // if (line)
                //     fprintf(out, "%p %d %s\n", (void*)RTN_Address(rtn), line, path.c_str());

                for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
                {
                    if (INS_Valid(ins)) {
                        PIN_GetSourceLocation(INS_Address(ins), NULL, &line, &path);
                        if (line){
                            fprintf(func_map_out, "%p %s : %d\n", (void*)INS_Address(ins), path.c_str(), line);
                            // Generate trace entry
                            /*
                            trace_entry_t trace_entry;
                            trace_entry.operation = PM_TRACE_WRITE_IP;
                            trace_entry.func_ret = false;
                            trace_entry.instr_ptr = (addr_t)INS_Address(ins);
                            //fprintf(stderr, "%s\n", rtnName);
                            assert(0!=trace_entry.operation);
                            // Send trace entry to trace_fifo
                            if(stage==PRE_FAILURE){
                                ((PINFifo*)fifo_ptr)->pinfifo_write(&trace_entry);
                            }
                            */
                            //  else if(stage==POST_FAILURE && path.find("x86_64/flush.h") != std::string::npos){
                            //     cerr << RTN_Name(rtn).c_str() << endl;
                            // }
                        }
                    }
                }
                RTN_Close(rtn);
            }
        }
    }
    
}
#endif


#include <tool_macros.h>
#include <execinfo.h>

// RoI management
RoITracker roi_tracker;
// Stage of testing
int stage = 0;

/* PMFuzz: new methods */
VOID recordWriteInstBacktrace(const CONTEXT * ctxt, void* ip, void* addr, uint64_t size, uint64_t tid)
{
    // if (!roi_tracker.isInRoI(tid)) return;

    if(isPmemAddr(addr, size)){
        void* buf[128];
        char **bt;

        PIN_LockClient();
        int nptrs = PIN_Backtrace(ctxt, buf, sizeof(buf)/sizeof(buf[0]));  
        ASSERTX(nptrs > 0);
        bt = backtrace_symbols(buf, nptrs);
        PIN_UnlockClient();

        ASSERTX(NULL != bt);

        // Dump backtrace to file
        fprintf(backtrace_out, ">> %p\n", ip);
        for (int i = 0; i < nptrs; i++) {
            string str = string(bt[i]);
            int start = str.find("(");
            int end = str.find(")");
            if (start >= 0 && end >= 0) {
                fprintf(backtrace_out, "%s\n", str.substr(start+1, end-start-1).c_str());
            }
        }
        fflush(backtrace_out);
        free(bt);
    }
}

VOID recordReadInstBacktrace(const CONTEXT * ctxt, void* ip, void* addr, uint64_t size, uint64_t tid)
{
    // if (!roi_tracker.isInRoI(tid) || stage != POST_FAILURE) return;

    if(isPmemAddr(addr, size)){
        void* buf[128];
        char **bt;

        PIN_LockClient();
        int nptrs = PIN_Backtrace(ctxt, buf, sizeof(buf)/sizeof(buf[0]));  
        ASSERTX(nptrs > 0);
        bt = backtrace_symbols(buf, nptrs);
        PIN_UnlockClient();

        ASSERTX(NULL != bt);

        // Dump backtrace to file
        fprintf(backtrace_out, ">> %p\n", ip);
        for (int i = 0; i < nptrs; i++) {
            string str = string(bt[i]);
            int start = str.find("(");
            int end = str.find(")");
            if (start >= 0 && end >= 0) {
                fprintf(backtrace_out, "%s\n", str.substr(start+1, end-start-1).c_str());
            }
        }
        fflush(backtrace_out);
        free(bt);
    }
}


VOID recordFuncBacktrace(const CONTEXT * ctxt, void* ip)
{
    void* buf[128];
    char **bt;

    PIN_LockClient();
    int nptrs = PIN_Backtrace(ctxt, buf, sizeof(buf)/sizeof(buf[0]));  
    ASSERTX(nptrs > 0);
    bt = backtrace_symbols(buf, nptrs);
    PIN_UnlockClient();

    ASSERTX(NULL != bt);

    // Dump backtrace to file
    fprintf(backtrace_out, ">> %p\n", ip);
    for (int i = 0; i < nptrs; i++) {
        string str = string(bt[i]);
        int start = str.find("(");
        int end = str.find(")");
        if (start >= 0 && end >= 0) {
            fprintf(backtrace_out, "%s\n", str.substr(start+1, end-start-1).c_str());
        }
    }
    fflush(backtrace_out);
    free(bt);
}


VOID Backtrace(IMG img, VOID *v) 
{
    // if (IMG_IsMainExecutable(img)) {

        // Backtrace for library functions: during pre-failure
        if (stage == PRE_FAILURE) {
            for (auto name = pm_functions.begin(); 
                            name != pm_functions.end(); ++name) {
                RTN rtn = RTN_FindByName(img, C_MANGLE(name->first.c_str()));
                if (RTN_Valid(rtn)) {
                    RTN_Open(rtn);
                    RTN_InsertCall(rtn, IPOINT_BEFORE, 
                                (AFUNPTR)recordFuncBacktrace, 
                                IARG_CONST_CONTEXT, 
                                IARG_RETURN_IP, 
                                IARG_END);
                    RTN_Close(rtn);
                }
            }
        }

        // Backtrace for read/write instructions
        for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)) { 
            for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)) {
                RTN_Open(rtn);
                for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
                    if (INS_Valid(ins)) {
                        // Discard CLWB read trace
                        UINT32 memOperands = INS_MemoryOperandCount(ins);

                        // Iterate over each memory operand of the instruction.
                        for (UINT32 memOp = 0; memOp < memOperands; memOp++) {

                            // Write: during pre-failure
                            if (stage == PRE_FAILURE && INS_MemoryOperandIsWritten(ins, memOp)) {
                                
                                if(INS_hasKnownMemorySize(ins)) {
                                    INS_InsertCall(ins, IPOINT_BEFORE, 
                                                (AFUNPTR)recordWriteInstBacktrace, 
                                                IARG_CONST_CONTEXT, 
                                                IARG_INST_PTR, 
                                                IARG_MEMORYOP_EA, memOp,
                                                IARG_MEMORYWRITE_SIZE,
                                                IARG_THREAD_ID,
                                                IARG_END);
                                } else {
                                    INS_InsertCall(ins, IPOINT_BEFORE, 
                                                (AFUNPTR)recordWriteInstBacktrace, 
                                                IARG_CONST_CONTEXT, 
                                                IARG_INST_PTR, 
                                                IARG_MEMORYOP_EA, memOp,
                                                IARG_MULTI_MEMORYACCESS_EA,
                                                IARG_THREAD_ID,
                                                IARG_END);
                                }
                            }

                            // Read: during post-failure
                            if (stage == POST_FAILURE && INS_MemoryOperandIsRead(ins, memOp)) {
                                INS_InsertCall(ins, IPOINT_BEFORE, 
                                            (AFUNPTR)recordReadInstBacktrace, 
                                            IARG_CONST_CONTEXT, 
                                            IARG_INST_PTR, 
                                            IARG_MEMORYOP_EA, memOp,
                                            IARG_MEMORYREAD_SIZE,
                                            IARG_THREAD_ID,
                                            IARG_END);
                            }
                        }
                    }
                }
                RTN_Close(rtn);
            }
        }
    // }
}


#endif // PM_FUNC_HH
