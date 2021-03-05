#ifndef TRACE_HH
#define TRACE_HH

#include "common.hh"
#define MAX_THREADS 32
// enum pm_op_type {
//     TRACE_FLAG = 1,
//     PM_BASIC_OP,
//     PM_ALLOCATION_FUNC,
//     PM_WRITEBACK_FUNC,
//     PM_LIBRARY_FUNC
// };

enum pm_op_t {
    // Uninitialized operation
    INVALID,
    
    // PM trace flags
    TRACE_BEGIN,
    TRACE_END,
    TESTING_END,
    
    // Basic PM operations
    READ,
    WRITE,
    CLWB,
    SFENCE,
    
    // PM allocation functions
    PMEM_MAP_FILE,
    PMEM_UNMAP,
    
    // PM library functions
    PM_TRACE_PM_ADDR_ADD,
    PM_TRACE_PM_ADDR_REMOVE,
    PM_TRACE_TX_BEGIN,
    PM_TRACE_TX_END,
    PM_TRACE_TX_ADDR_ADD,
    PM_TRACE_TX_ALLOC,
    PMDK_INTERNAL_CALL,
    PMDK_INTERNAL_RET,
    
    // Low-level semantics
    _ADD_COMMIT_VAR,
    
    // PM_TRACE_WRITE_IP, //PMFuzz: obsolete
    // Detection skip
    PM_TRACE_DETECTION_SKIP_BEGIN,
    PM_TRACE_DETECTION_SKIP_END,
};

struct trace_entry_t {
    pm_op_t operation = INVALID;
    bool func_ret = false;
    int tid;
    addr_t src_addr = 0;
    addr_t dst_addr = 0;
    size_t size = 0;
    // size_t dst_size = 0;
    addr_t instr_ptr = 0;
    int non_temporal = 0;
    // size_t line_number = 0;
    // char file_name[20];
};

#endif // TRACE_HH