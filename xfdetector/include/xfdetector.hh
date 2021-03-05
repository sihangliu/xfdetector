#ifndef PM_RACE_HH
#define PM_RACE_HH

#include "trace.hh"
#include "common.hh"
#include <bits/stdc++.h> 
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unordered_set>
using std::unordered_set;

// Boost
#include <boost/icl/interval_set.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/filesystem.hpp>

using namespace boost::icl;
using namespace boost::filesystem;

/* Names of PM operations for print out */
static const char* pm_op_name[] = {
    // Uninitialized operation
    "invalid",
    // PM trace flags
    "trace_begin",
    "trace_end",
    "testing_end",
    // Basic PM operations
    "read",
    "write",
    "clwb",
    "sfence",
    // PM allocation functions
    "pmem_map_file",
    "pmem_unmap",
    // PM writeback functions

    // PM library functions
    "pm_trace_pm_addr_add",
    "pm_trace_pm_addr_remove",
    "pm_trace_tx_begin",
    "pm_trace_tx_end",
    "pm_trace_tx_addr_add",
    "pmdk_internal_call",
    "pmdk_internal_ret",

    // Low-level semantics
    "_add_commit_var",
    // "pm_trace_write_ip", //PMFuzz: obsolete
    // Detection skip
    "_skipDetectionBegin",
    "_skipDetectionEnd",
};

/** Constant values **/
const std::string PIN_ROOT_ENV = "PIN_ROOT";

const string HELP_STR = "HELP\n"
    "\n"
    "  USAGE\n"
    "    xfdetector pintool_path pm_image_name [--failure-points=path] -- target_cmd\n"
    "\n"
    "  REQUIRED ARGUMENTS\n"
    "               pintool_path     Path to the pintool\n"
    "              pm_image_name     Name of the PM image to replace in the target_cmd\n"
    "                 target_cmd     Command to run the target program. Pool name should be replaced with __POOL_IMAGE__.\n"
    "\n"
    "  OPTIONAL ARGUMENTS\n"
    "          --failure-points=     Path to the file container failure points.\n"
    "\n"
    "  TARGET COMMAND FORMAT\n"
    "             __POOL_IMAGE__     Name of the pool image, this part will be automatically replaced\n";
    
const string POOL_IMAGE_IDENTIFIER = "__POOL_IMAGE__";

template <typename Type> struct inplace_assign: 
                            public identity_based_inplace_combine<Type>
{
	typedef inplace_assign<Type> type;

	void operator()(Type& A, const Type& B) const
	{ A = B; }
};

#define MAX_BACKTRACE 10

// Track pids
static pid_t pre_failure_pid;
static pid_t post_failure_pid;

static int exec_id;

#define XFD_ASSERT(cond) \
    assert(cond)
    //  if (!(cond)) {kill(pre_failure_pid, 9); kill(post_failure_pid, 9); assert(cond);}

enum PMStatus {
    CLEAN,
    MODIFIED,
    WRITEBACK_PENDING,
    WRITTEN_BACK,
    CONSISTENT
};

// enum PMCorrectCase {
//     CONSISTENT,
//     PERSISTED_AND_COMMITTED,
//     COMMIT_VAR
// };

typedef interval_map<addr_t, PMStatus, partial_enricher, std::less, 
                        inplace_assign> interval_map_addr_status;
typedef interval_map<addr_t, timestamp_t, partial_enricher, std::less, 
                        inplace_assign> interval_map_addr_time;
typedef interval_map<addr_t, addr_t, partial_enricher, std::less, 
                        inplace_assign> interval_map_addr_IP;

typedef interval_set<addr_t> interval_set_addr;
typedef interval_set_addr::interval_type ival;

static std::unordered_set<addr_t> addr2ip;
static std::vector<string> target_cmd;

#define SET_INSERT(set, addr, size) \
        set.add(ival::closed(addr, size+addr-1))

#define SET_REMOVE(set, addr, size) \
        set.erase(ival::closed(addr, size+addr-1))

#define SET_LOOKUP(set, addr, size) \
        within(ival::closed(addr, size+addr-1), set)

#define SET_CLEAR(set) \
        set.clear()

#define MAP_UPDATE(map, addr, size, new_val) \
        (map += (make_pair(ival::closed(addr, size+addr-1), new_val)))

#define MAP_UPDATE_INTERVAL(map, update_interval, new_val) \
        (map += make_pair(update_interval, new_val))

#define MAP_REMOVE(map, addr, size) \
        (map -= (ival::closed(addr, size+addr-1)))

#define MAP_LOOKUP(map, addr, size) \
        (map & ival::closed(addr, size+addr-1))

#define MAP_INTERSECT(map, addr, size) \
        intersects(map, ival::closed(addr, size+addr-1))

#define IN_MAP(map, addr, size) \
        within(ival::closed(addr, size+addr-1), map)

#define MAP_CLEAR(map) \
        map.clear()

struct Bug_t {
    trace_entry_t op;
    char description[100];
};

// Trace entries that triggers a warning
static vector<Bug_t> warn_vec;
// Trace entries that triggers a bug
static vector<Bug_t> error_vec;

#define WARN(op_ptr, message) {\
        Bug_t bug; \
        bug.op = *op_ptr; \
        strcpy(bug.description, message); \
        warn_vec.push_back(bug); \
    }

#define ERROR(op_ptr, message) {\
        Bug_t bug; \
        bug.op = *op_ptr; \
        strcpy(bug.description, message); \
        error_vec.push_back(bug); \
    }

// Color output
#define WARN_PRINT "\033[1;33m[WARN]\033[0m\n"
#define ERROR_PRINT "\033[1;31m[ERROR]\033[0m\n"

// Pintool flags
#define PIN_TRACK_READ string("-r 1 ")
#define PIN_ENABLE_FIFO string("-t 1 ")
#define PIN_ENABLE_FAILURE string("-f 1 ")
#define PIN_REDIRECT_OUT string("-o out ")
#define PIN_SET_EXECID(val) (string("-i ") + std::to_string(val))
#define PIN_SET_FAILURE_FILE(val) (string("-l ") + val)

class ShadowPM {
public:
    /* ========Constructor======== */
    ShadowPM();
    ShadowPM(const ShadowPM& in);
    /* ========Update methods======== */
    // Call when allocate new PM 
    void add_pm_addr(trace_entry_t*, addr_t, size_t);
    void add_pm_addr_post(trace_entry_t*, addr_t, size_t);
    // Call when deallocate existing PM
    void remove_pm_addr(trace_entry_t*, addr_t, size_t);
    // Call when CLWB an address
    void writeback_addr(trace_entry_t*, addr_t, size_t); 
    // Call when SFENCE is issued
    void drain_writeback(trace_entry_t*);
    // Call when PM address is consistent according to programming model
    void set_consistent_addr(trace_entry_t*, addr_t, size_t);
    // Call when PM address is modified
    void modify_addr(trace_entry_t*, addr_t, size_t);
    void modify_addr_internal(trace_entry_t*, addr_t, size_t);
    void increment_global_time();

    void reset_internal_funct_level(int tid);
    void increment_pre_internal_funct_level(int tid);
    void decrement_pre_internal_funct_level(int tid);
    bool is_in_pre_internal_funct(int tid);

    void increment_tx_level(int tid, int stage);
    void decrement_tx_level(int tid, int stage);
    bool is_in_tx(int tid);

    bool print_IP_linenumber_mapping(addr_t writeip, int stage);

    /* ========Checking methods======== */
    // Check if addr is guaranteed to be written back
    bool is_writtenback(trace_entry_t*, addr_t, size_t);
    // Check if addr is consistent according to programming model
    bool is_writeback_pending(trace_entry_t*, addr_t, size_t);
    // Check if addr is consistent according to programming model
    bool is_consistent(trace_entry_t*, addr_t, size_t);
    // Check if addr is within allocated PM locations
    bool is_pm_addr(trace_entry_t*, addr_t, size_t);

    bool is_added_addr(trace_entry_t*, addr_t, size_t);
    bool is_non_added_write_addr(trace_entry_t*, addr_t, size_t);
    void add_tx_add_addr(trace_entry_t*, addr_t, size_t, int, bool);
    // void add_tx_alloc_addr(trace_entry_t*, addr_t, size_t, int);
    void add_non_tx_add_addr(trace_entry_t*, addr_t, size_t);
    interval_set_addr get_tx_added_addr(int tid);
    void add_commit_var_addr(trace_entry_t* op_ptr, addr_t addr, size_t size);
    bool is_commit_var_addr(trace_entry_t* op_ptr, addr_t addr, size_t size);
    bool is_recent_commit_update(trace_entry_t* op_ptr, addr_t addr, size_t size);
    void add_write_addr_IP_mapping(trace_entry_t* op_ptr);
    bool print_look_up_write_addr_IP_mapping(trace_entry_t* op_ptr, addr_t addr, size_t size, FILE* file);

    bool lookup_checked_addr(addr_t addr, size_t size);
    void insert_checked_addr(addr_t addr, size_t size);
    // Read debug output
    bool printInconsistentReadDebug(trace_entry_t* cur_trace);
    // Set for commit variable
    interval_set_addr commit_var_set_addr;
    // Map for Write address -> IP mapping
    interval_map_addr_IP write_addr_IP_mapping;
    // ShadowPM();
    // ~ShadowPM();
    void disable_detection(int tid);
    void enable_detection(int tid);
    int is_detection_disabled(int tid);
    void update_commitVar_timestamp();
    timestamp_t global_timestamp = 0;

private:
    // PM address to memory status mapping
    interval_map_addr_status pm_status;
    // PM address to modification timestamp mapping
    interval_map_addr_time pm_modify_timestamps;
    // Timestamp based on ordering points
    // Keep track of library function calls.
    int pre_InternalFunctLevel[MAX_THREADS];
    // Address set for tracking TX_ADD-ed addresses inside the transaction.
    interval_set_addr tx_added_addr[MAX_THREADS];
    interval_map_addr_IP tx_alloc_addr_IP_mapping[MAX_THREADS];
    interval_map_addr_IP tx_added_addr_IP_mapping[MAX_THREADS];
    // Address set for tracking non TX_ADD-ed write inside the transaction.
    // We use this to detect inconsistency caused by having TX_ADD after write.
    interval_set_addr tx_non_added_write_addr[MAX_THREADS];
    // Counter for nested transaction.
    int tx_level[MAX_THREADS];
    // Filter out checked addresses
    unordered_set<uint64_t> checked_addr_set;
    int skipDetectionStatus[MAX_THREADS];
    // Commit variable timestamp
    timestamp_t commit_timestamp = -1;
    // TODO: Extension for multiple commit variables
    // This is not necessary for our test cases now.
    //unordered_map<addr_t, timestamp_t> commitAddr_to_timestamp_map;

};

#define NUM_OPTIONS 5

class ExeCtrl {
public:
    void init(int, std::vector<string>);
    void execute_pre_failure();
    string execute_post_failure();
    string get_executable_path() {return executable_path; }
    // void kill_proc(unsigned);
    void term_pre_failure();
    void term_post_failure();
    int post_failure_status();
    // int exec_id = -1; // Change to global
private:
    string copy_pm_image();
    char *change_env(char *kv);
    char** genPinCommand(int, string);
    void parse_exec_command(std::vector<string>);
    string rename_pool_img(string);
    string getExeName();
    string config_file;
    string failure_point_file;
    string pintool_path;
    string executable_path;
    string pm_image_name;
    string pre_failure_exec_command;
    // need to cut post-failure command into two parts 
    // part1<pm_recovery_image>part2
    string post_failure_exec_command_part1;
    string post_failure_exec_command_part2;
    // Track read in post-failure execution
    string pin_pre_failure_option 
        = PIN_ENABLE_FAILURE + PIN_ENABLE_FIFO; // + PIN_SET_EXECID(exec_id);
    string pin_post_failure_option 
        = PIN_TRACK_READ + PIN_ENABLE_FIFO + PIN_REDIRECT_OUT; // + PIN_SET_EXECID(exec_id);
};

class XFDetectorFIFO {
public:
    // Read from pre-failure FIFO
    int pre_fifo_read();
    // Read from post-failure FIFO
    int post_fifo_read();
    // Send control signals
    void pin_continue_send();
    trace_entry_t* get_trace(int, unsigned);
    void clear_pre_fifo_buf() {memset(pre_fifo_buf, 0, PIN_FIFO_BUF_SIZE);}
    void clear_post_fifo_buf() {memset(post_fifo_buf, 0, PIN_FIFO_BUF_SIZE);}

    XFDetectorFIFO(int);
    ~XFDetectorFIFO();

    void fifo_open(const char*);
    void fifo_close(const char*);

private:
    char pre_failure_fifo_str[1024];
    char post_failure_fifo_str[1024];
    char signal_fifo_str[1024];

    // Read/write through Signal FIFO
    int signal_send(char*, unsigned);
    int signal_recv();

    // Create all FIFOs
    void fifo_create(int exec_id);

    // FIFO buffer for pre-failure trace
    trace_entry_t* pre_fifo_buf;
    // FIFO buffer for post-failure trace
    trace_entry_t* post_fifo_buf;
    char* signal_buf;

    // FIFO for PIN tool
    int pre_fifo_fd;
    int post_fifo_fd;
    // FIFO for sending control signals
    int signal_fifo_fd;
};

class XFDetectorDetector {
public:
    // void update_pre_failure_status(ShadowPM*, trace_entry_t*);
    // void update_post_failure_status(ShadowPM*, trace_entry_t*);
    void update_pm_status(int, ShadowPM*, trace_entry_t*);
    void check_pm_status();
    void locate_bug(trace_entry_t*, string);
    // Trace printer for debugging
    void print_pm_trace(int, trace_entry_t*);
    // XFDetectorDetector();
    // ~XFDetectorDetector();
    bool pre_testing_complete = INCOMPLETE;
    bool pre_failure_point_complete = INCOMPLETE;
    bool post_testing_complete = INCOMPLETE;
private:
};

// Get existing envs
extern char **environ;

#endif
