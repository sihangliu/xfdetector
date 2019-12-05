#ifndef PMRACE_TX_HH
#define PMRACE_TX_HH

//#define PMDK_INTERNAL_CALL 0
//#define PMDK_INTERNAL_RET 1

void recordTXBound(char* rtnName, void* return_ip, uint64_t tid){
    assert(tid < MAX_THREADS);
    // Generate trace entry
    trace_entry_t trace_entry;
    trace_entry.operation = pm_functions[string(rtnName)].enum_name;
    trace_entry.func_ret = false;
    trace_entry.tid = tid;
    trace_entry.instr_ptr = (addr_t)return_ip;
    //fprintf(stderr, "%s\n", rtnName);
    assert(0!=trace_entry.operation);
    // Send trace entry to trace_fifo
    trace_fifo.pinfifo_write(&trace_entry);
}


void recordPmemAllocationTX(char* rtnName, void* return_ip, uint64_t tid, ADDRINT addr, ADDRINT size)
{
    // Always track PM allocation functions

    assert(tid < MAX_THREADS);
    //assert(pm_functions[string(rtnName)].type == PM_ALLOCATION_FUNC);

#ifdef SKIP_FUNC_OP
	if (func_status_table[tid].status == CALLED) return;
#endif

    pm_addr_set.insert(ival::closed(addr, size+((uint64_t)addr)-1));
    PinDEBUG(*out << "Funct: " << rtnName 
             << " tid: " << tid << " addr: " << (void*)addr 
             << " size: " << size << endl);

    // Generate trace entry
    trace_entry_t trace_entry;
    trace_entry.operation = pm_functions[string(rtnName)].enum_name;
    trace_entry.func_ret = false;
    trace_entry.tid = tid;
    trace_entry.dst_addr = addr;
    trace_entry.size = size;
    trace_entry.instr_ptr = (addr_t)return_ip;
    //fprintf(stderr, "%s\n", rtnName);
    assert(0!=trace_entry.operation);
    // Send trace entry to trace_fifo
    trace_fifo.pinfifo_write(&trace_entry);

#ifdef SKIP_FUNC_OP
    func_status_table[tid].func_name = string(rtnName);
    func_status_table[tid].status = CALLED;
#endif
}

// Routine instrument for TX
void PMOpTraceInstTX(RTN rtn, void* v)
{
    RTN_Open(rtn);

    string func_name = RTN_Name(rtn);

    if(func_name == "pm_trace_pm_addr_add"){
        // Instrumentation for pm_trace_pm_addr_add()
        // Call
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)recordPmemAllocationTX,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["pm_trace_pm_addr_add"].dst,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["pm_trace_pm_addr_add"].size,
            IARG_END);
        // Return
        RTN_InsertCall(
            rtn, IPOINT_AFTER, 
            (AFUNPTR)recordPmemRetCommon, 
            IARG_ADDRINT, 
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCRET_EXITPOINT_VALUE,
            IARG_END);
    } else if(func_name == "pm_trace_pm_addr_remove"){
        // Instrumentation for pm_trace_pm_addr_add()
        // Call
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)recordPmemDeallocation,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["pm_trace_pm_addr_remove"].src,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["pm_trace_pm_addr_remove"].size,
            IARG_END);
        // Return
        RTN_InsertCall(
            rtn, IPOINT_AFTER, 
            (AFUNPTR)recordPmemRetCommon, 
            IARG_ADDRINT, 
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCRET_EXITPOINT_VALUE,
            IARG_END);
    } else if(func_name == "pm_trace_tx_begin"){
        // Instrumentation for pm_trace_tx_begin()
        // Call
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)recordTXBound,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_END);
        // Return
        RTN_InsertCall(
            rtn, IPOINT_AFTER, 
            (AFUNPTR)recordPmemRetCommon, 
            IARG_ADDRINT, 
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCRET_EXITPOINT_VALUE,
            IARG_END);
    } else if(func_name == "pm_trace_tx_end"){
        // Instrumentation for pm_trace_tx_end()
        // Call
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)recordTXBound,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_END);
        // Return
        RTN_InsertCall(
            rtn, IPOINT_AFTER, 
            (AFUNPTR)recordPmemRetCommon, 
            IARG_ADDRINT, 
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCRET_EXITPOINT_VALUE,
            IARG_END);
    } else if(func_name == "pm_trace_tx_addr_add"){
        // Instrumentation for pm_trace_tx_addr_add()
        // Notice that the instrument function is the same as
        // pm_trace_pm_addr_add. This is intentional.
        // Call
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)recordPmemAllocationTX,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["pm_trace_tx_addr_add"].dst,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["pm_trace_tx_addr_add"].size,
            IARG_END);
        // Return
        RTN_InsertCall(
            rtn, IPOINT_AFTER, 
            (AFUNPTR)recordPmemRetCommon, 
            IARG_ADDRINT, 
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCRET_EXITPOINT_VALUE,
            IARG_END);
    } else {
        // TODO: Atomic operations
        

        // Do not track INVALID operations
    } // func_name

    RTN_Close(rtn);
}

void PMDKInternalFunctHandler(char* func_name, uint64_t tid, uint32_t type)
{
    if(type==PMDK_INTERNAL_CALL){
        PinDEBUG(cerr << "Calling PMDK Funct: " << func_name 
                << " tid: " << tid << endl;);
    }else{
        PinDEBUG(cerr << "Returning PMDK Funct: " << func_name 
                << " tid: " << tid << endl;);
    }
    // Generate trace entry
    trace_entry_t trace_entry;
    trace_entry.tid = tid;
    trace_entry.operation = ((type==PMDK_INTERNAL_CALL)?PMDK_INTERNAL_CALL:PMDK_INTERNAL_RET);
    assert(0!=trace_entry.operation);
    // Send complete trace to pin_fifo
    trace_fifo.pinfifo_write(&trace_entry);
}


void PMDKInternalFunct(RTN rtn, void* v)
{
    RTN_Open(rtn);

    string func_name = RTN_Name(rtn);
    // These are PMDK's internal function which can be assumed to be safe?
	// From tx.c
    if (func_name == "pmemobj_tx_begin" || 
        func_name == "pmemobj_tx_stage" ||
        func_name == "pmemobj_tx_process" ||
        func_name == "pmemobj_tx_lock" || 
        func_name == "pmemobj_tx_abort" ||
        func_name == "pmemobj_tx_commit" ||
        func_name == "pmemobj_tx_end" || 
        //func_name == "pmemobj_tx_add_common" ||
        func_name == "pmemobj_tx_alloc" ||
        func_name == "pmemobj_tx_add_range_direct" || 
        func_name == "pmemobj_tx_xadd_range_direct" ||
        func_name == "pmemobj_tx_add_range" ||
        func_name == "pmemobj_tx_xadd_range" ||
        func_name == "pmemobj_tx_alloc" ||
        func_name == "pmemobj_tx_zalloc" ||
        func_name == "pmemobj_tx_xalloc" ||
        func_name == "pmemobj_tx_realloc" ||
        func_name == "pmemobj_tx_zrealloc" ||
        func_name == "pmemobj_tx_strdup" ||
        func_name == "pmemobj_tx_wcsdup" ||
        func_name == "pmemobj_tx_free" ||
        func_name == "pmemobj_tx_publish" ||
	// From obj.c
        func_name == "pmemobj_create" ||
        func_name == "pmemobj_open" ||
        func_name == "pmemobj_close" ||
        func_name == "pmemobj_check" ||
        func_name == "pmemobj_alloc" ||
        func_name == "pmemobj_xalloc" ||
        func_name == "pmemobj_zalloc" ||
        func_name == "pmemobj_realloc" ||
        func_name == "pmemobj_zrealloc" ||
        func_name == "pmemobj_strdup" ||
        func_name == "pmemobj_wcsdup" ||
        func_name == "pmemobj_free" ||
        func_name == "pmemobj_root_construct" ||
        func_name == "pmemobj_root" ||
        func_name == "pmemobj_reserve" ||
        func_name == "pmemobj_xreserve" ||
        func_name == "pmemobj_publish" ||
        func_name == "pmemobj_cancel" ||
        func_name == "pmemobj_list_insert" ||
        func_name == "pmemobj_list_insert_new" ||
        func_name == "pmemobj_list_remove" ||
        func_name == "pmemobj_list_move" ||
        func_name == "pmemobj_ctl_set" ||
        func_name == "pmemobj_ctl_exec"
        // TODO, add pmemobj version of pmem functions
        ) {
        // Start or stop tracing after RoI functions
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)PMDKInternalFunctHandler,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_THREAD_ID,
            IARG_UINT32,
            PMDK_INTERNAL_CALL,
            IARG_END);
        RTN_InsertCall(
            rtn, IPOINT_AFTER,
            (AFUNPTR)PMDKInternalFunctHandler,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_THREAD_ID,
            IARG_UINT32,
            PMDK_INTERNAL_RET,
            IARG_END);
    }
    RTN_Close(rtn);
}

#endif
