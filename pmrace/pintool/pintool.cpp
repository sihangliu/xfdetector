// #define SKIP_FUNC_OP

// PIN tool include
#include "pin.H"

// Common include
#include <iostream>
#include <fstream>
// #include <vector>
#include <boost/icl/interval_set.hpp>
#include <boost/icl/interval_map.hpp>
using std::cerr;
using std::cout;
using std::endl;
// using std::vector;
using std::string;
using namespace boost::icl;



/* ================================================================== */
// Global variables 
/* ================================================================== */
// Output fd
std::ostream * out = &cerr;
FILE* func_map_out;

// Read tracking
bool read_enable = false;

// Auto inject failure points
bool failure_enable = false;

// Send trace to FIFO
bool fifo_enable = false;

void* fifo_ptr;

#include "../include/common.hh"
// Stage of testing
int stage = PRE_FAILURE;

// Pintool classes
#include "pmrace_pintool.hh"

// RoI management
RoITracker roi_tracker;

// Skip failure controlled by passing failure skip function
// Not thread safe, but only the selected RoI thread can access it
bool skip_failure = false;


// Interval set of PM addresses
typedef interval_set<addr_t> interval_set_addr;
typedef interval_set_addr::interval_type ival;
interval_set_addr pm_addr_set;

#ifdef SKIP_FUNC_OP

#define CALLED true
#define RETURNED false

// Record function call/return status to avoid tracking operations in PM library functions
struct func_status_t {
    string func_name;
    bool status;
};

// When CALLED is set, future operations are not longer tracked, until it is RETURNED
func_status_t func_status_table[MAX_THREADS];
#endif

ThreadCounter thread_counter;
int failure_point_count;

PINFifo trace_fifo;
SignalFifo signal_fifo;

// PMRace include after global variables
#include "pmrace_pmem.hh"
#include "pmrace_tx.hh"


/* ===================================================================== */
// Command line switches
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "", "specify file name for MyPinTool output");

KNOB<string> KnobEnableRead(KNOB_MODE_WRITEONCE, "pintool",
    "r", "", "enable tracking PM read operations");

KNOB<string> KnobEnableFailure(KNOB_MODE_WRITEONCE, "pintool",
    "f", "", "enable auto failure injection");

KNOB<string> KnobEnableFIFO(KNOB_MODE_WRITEONCE, "pintool",
    "t", "", "enable trace fifo");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
    cerr << "This tool tracks PM opeartions and functions in PM workloads" << endl << endl;

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

void ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    PinDEBUG(cerr << "Thread ID " << tid << " start" << endl);
    thread_counter.increment(tid);
}

void ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 flags, VOID *v)
{
    PinDEBUG(cerr << "Thread ID " << tid << " exit" << endl);
    thread_counter.decrement(tid);
}

void waitOnSignal(const char* signal)
{
    char buf[MAX_SIGNAL_LEN];
    while (1) {
        int signal_len = signal_fifo.signal_recv(buf, MAX_SIGNAL_LEN);
        // remove the last character in case of \0 and \n
        if (signal_len > 0 && !strncmp(buf, signal, signal_len-1))
            return;
    }
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

void RoIHandler(char* func_name, uint64_t tid)
{
    string name = string(func_name);
    if (stage == PRE_FAILURE) {
        if(name == "_roi_pre_begin") {
            PinDEBUG(cerr << "RoI_Begin @ tid " << tid << endl);
            roi_tracker.trySetRoIThread(tid);
        } else if(name == "_roi_pre_end") {
            PinDEBUG(cerr << "RoI_End @ tid " << tid << endl);
            roi_tracker.unsetRoIThread(tid);
        } else if(name == "_skipDetectionBegin") {
            PinDEBUG(cerr << "skipDetectionBegin @ tid " << tid << endl);
            // Generate trace entry
            trace_entry_t trace_entry;
            trace_entry.tid = tid;
            trace_entry.operation = PM_TRACE_DETECTION_SKIP_BEGIN;
            // Send complete trace to pin_fifo
            trace_fifo.pinfifo_write(&trace_entry);
        } else if(name == "_skipDetectionEnd") {
            PinDEBUG(cerr << "skipDetectionEnd @ tid " << tid << endl);
            // Generate trace entry
            trace_entry_t trace_entry;
            trace_entry.tid = tid;
            trace_entry.operation = PM_TRACE_DETECTION_SKIP_END;
            // Send complete trace to pin_fifo
            trace_fifo.pinfifo_write(&trace_entry);
        } else if(name == "_testing_pre_complete") {
            PinDEBUG(cerr << "Complete @ tid " << tid << endl);
            
            // Generate trace entry
            trace_entry_t trace_entry;
            trace_entry.tid = tid;
            trace_entry.operation = TESTING_END;
            // Send complete trace to pin_fifo
            trace_fifo.pinfifo_write(&trace_entry);
        }
    } else {
        if(name == "_roi_post_begin") {
            PinDEBUG(cerr << "RoI_Begin @ tid " << tid << endl);
            roi_tracker.trySetRoIThread(tid);
        } else if(name == "_roi_post_end") {
            PinDEBUG(cerr << "RoI_End @ tid " << tid << endl);
            roi_tracker.unsetRoIThread(tid);
        } else if(name == "_skipDetectionBegin") {
            PinDEBUG(cerr << "skipDetectionBegin @ tid " << tid << endl);
            // Generate trace entry
            trace_entry_t trace_entry;
            trace_entry.tid = tid;
            trace_entry.operation = PM_TRACE_DETECTION_SKIP_BEGIN;
            // Send complete trace to pin_fifo
            trace_fifo.pinfifo_write(&trace_entry);
        } else if(name == "_skipDetectionEnd") {
            PinDEBUG(cerr << "skipDetectionEnd @ tid " << tid << endl);
            // Generate trace entry
            trace_entry_t trace_entry;
            trace_entry.tid = tid;
            trace_entry.operation = PM_TRACE_DETECTION_SKIP_END;
            // Send complete trace to pin_fifo
            trace_fifo.pinfifo_write(&trace_entry);
        } else if(name == "_testing_post_complete") {
            PinDEBUG(cerr << "Complete @ tid " << tid << endl);
            
            // Generate trace entry
            trace_entry_t trace_entry;
            trace_entry.tid = tid;
            trace_entry.operation = TESTING_END;
            // Send complete trace to pin_fifo
            trace_fifo.pinfifo_write(&trace_entry);

            // Close FIFOs
            //trace_fifo.pinfifo_close();
            //signal_fifo.signalfifo_close();
            // Terminate program
            exit(1);
        }
    }
}


void addFailurePoint(void* writeIP, char* func_name, uint64_t tid)
{
    // Only add failure points if in RoI
    if (!roi_tracker.isInRoI(tid)) return;

    // Skip failure point on demand
    if (string(func_name) == "_skip_failure_point_begin") {
        skip_failure = true;
        assert(roi_tracker.roi_tid == tid);
        return;
    } else if (string(func_name) == "_skip_failure_point_end") {
        skip_failure = false;
        assert(roi_tracker.roi_tid == tid);
        return;
    }

    PinDEBUG(cerr << "Failure point injection for " << func_name << endl;);

    // Stop all therads
    PIN_StopApplicationThreads(tid);
    PinDEBUG(cerr << "Threads stopped by tid=" << tid << endl);
    failure_point_count++;

    // Send TRACE_END flag to trace_fifo
    trace_entry_t trace_entry;
    trace_entry.tid = tid;
    trace_entry.operation = TRACE_END;
    trace_entry.instr_ptr = (addr_t)writeIP;
    trace_fifo.pinfifo_write(&trace_entry);

    // Wait until receives resumption singal
    waitOnSignal(PIN_CONTINUE_SIGNAL);

    // Resume stopped threads
    PIN_ResumeApplicationThreads(tid);
    PinDEBUG(cerr << "Threads resumed by tid=" << tid << endl);
}



// Failure point injection
void FailurePointInst(RTN rtn, void* v) 
{
    RTN_Open(rtn);

    string func_name = RTN_Name(rtn);

    if (failure_point_funcs.find(func_name) != failure_point_funcs.end()) {
        // Inject failure point before and after targeting functions
        // Before
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)addFailurePoint,
            IARG_RETURN_IP,
            IARG_ADDRINT, 
            RTN_Name(rtn).c_str(),
            IARG_THREAD_ID,
            IARG_END);
        // After
        // RTN_InsertCall(
        //     rtn, IPOINT_AFTER,
        //     (AFUNPTR)addFailurePoint,
        //     IARG_ADDRINT, 
        //     RTN_Name(rtn).c_str(),
        //     IARG_THREAD_ID,
        //     IARG_END);
    } else if (func_name == "_skip_failure_point_begin" ||
               func_name == "_skip_failure_point_end") {
        // Skip injection of failure points on demand
        RTN_InsertCall(
            rtn, IPOINT_AFTER,
            (AFUNPTR)addFailurePoint,
            IARG_RETURN_IP,
            IARG_ADDRINT, 
            RTN_Name(rtn).c_str(),
            IARG_THREAD_ID,
            IARG_END);
    }

    RTN_Close(rtn);
}

// Failure point injection
void RoISelection(RTN rtn, void* v) 
{
    RTN_Open(rtn);

    string func_name = RTN_Name(rtn);

    if (func_name == "_roi_pre_begin" || 
        func_name == "_roi_pre_end" ||
        func_name == "_roi_post_begin" ||
        func_name == "_roi_post_end" ||
        func_name == "_testing_pre_complete" ||
        func_name == "_testing_post_complete") {
        // Start or stop tracing after RoI functions
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)RoIHandler,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_THREAD_ID,
            IARG_END);
    }
    RTN_Close(rtn);
}






int main(int argc, char *argv[])
{
    // Initialize unordered_map of PM functions
    pm_func_init();
    // Initialize lock for print
    PIN_MutexInit(&print_lock);
#ifdef SKIP_FUNC_OP
    // Initialized func_status table to RETURNED
    for (int i = 0; i < MAX_THREADS; ++i) {
        func_status_table[i].func_name = "";
        func_status_table[i].status = RETURNED;
    }
#endif

    fifo_ptr = &trace_fifo;

    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid 
    PIN_InitSymbols();
    if(PIN_Init(argc,argv)) { return Usage();}
    
    // Parse Knob options
    string fileName = KnobOutputFile.Value();
    string readOption = KnobEnableRead.Value();
    string failureOption = KnobEnableFailure.Value();
    string fifoOption = KnobEnableFIFO.Value();

    if (!fileName.empty()) { out = new std::ofstream(fileName.c_str());}
    if (!readOption.empty()) { read_enable = true;}
    if (!failureOption.empty()) { failure_enable = true;}
    if (!fifoOption.empty()) {fifo_enable = true;}
    
    if(read_enable && !failure_enable){
        stage = POST_FAILURE;
    }else{
        stage = PRE_FAILURE;
    }
    trace_fifo.init(stage);

    // Image instrument
    func_map_out = fopen("/tmp/func_map", "ab");
    IMG_AddInstrumentFunction(ImageLoad, 0);

    if (failure_enable) { RTN_AddInstrumentFunction(FailurePointInst, 0);}
    RTN_AddInstrumentFunction(PMDKInternalFunct, 0);
    RTN_AddInstrumentFunction(PMOpTraceInstPmem, 0);
    RTN_AddInstrumentFunction(PMOpTraceInstTX, 0);
    RTN_AddInstrumentFunction(RoISelection, 0);
    INS_AddInstrumentFunction(PMReadAndWriteInst, 0);

    // Track thread liveliness
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    // Pint tool description
    cerr <<  "===============================================" << endl;
    cerr <<  "This application is instrumented by PMRacePinTool" << endl;
    
    // Output option
    if (!KnobOutputFile.Value().empty()) 
    {
        cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
    }
    // Tracking option
    if (!KnobEnableRead.Value().empty()) 
    {
        cerr << "Read tracking enabled" << endl;
    }
    // Failure option
    if (!KnobEnableRead.Value().empty()) 
    {
        cerr << "Auto failure injection enabled" << endl;
    }
    // Failure option
    if (!KnobEnableFIFO.Value().empty()) 
    {
        cerr << "Trace FIFO enabled" << endl;
    }

    cerr <<  "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
