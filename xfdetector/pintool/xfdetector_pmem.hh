#ifndef PMRACE_PMEM_HH
#define PMRACE_PMEM_HH


// Print a memory read record
void recordPmemRead(void* ip, void* addr, uint64_t size, uint64_t tid)
{
    // Only record trace in RoI
    if (!roi_tracker.isInRoI(tid)) return;

    assert(tid < MAX_THREADS);

#ifdef SKIP_FUNC_OP
	if (func_status_table[tid].status == CALLED) return;
#endif

	if(isPmemAddr(addr, size)){
        // Trace output for debugging
		PinDEBUG(*out << "R: " << addr 
                    << " size: " << size
                    << " tid: " << tid << endl);

        // Generate trace entry
        trace_entry_t trace_entry;
        trace_entry.tid = tid;
        trace_entry.operation = READ;
        trace_entry.func_ret = false;
        trace_entry.src_addr = (addr_t)addr;
        trace_entry.size = size;
        trace_entry.instr_ptr = (addr_t)ip;
        assert(0!=trace_entry.operation);
        // Send trace entry to trace_fifo
        trace_fifo.pinfifo_write(&trace_entry);
	}
}

// Print a memory write record
void recordPmemWrite(void* ip, void* addr, uint64_t size, uint64_t tid, bool is_non_temporal_write)
{
    assert(tid < MAX_THREADS);

#ifdef SKIP_FUNC_OP
	if (func_status_table[tid].status == CALLED) return;
#endif

	if(isPmemAddr(addr, size)){
        // Trace output for debugging
		PinDEBUG(*out << "W: " << addr 
                    << " size: " << size 
                    << " tid: " << tid << endl);

        // Generate trace entry
        trace_entry_t trace_entry;
        trace_entry.tid = tid;
        trace_entry.operation = WRITE;
        trace_entry.func_ret = false;
        trace_entry.dst_addr = (addr_t)addr;
        trace_entry.size = size;
        trace_entry.instr_ptr = (addr_t)ip;
        trace_entry.non_temporal = is_non_temporal_write;

        assert(0!=trace_entry.operation);
        assert(0!=trace_entry.dst_addr && "Cannot write address=0");
        // Send trace entry to trace_fifo
        trace_fifo.pinfifo_write(&trace_entry);
	}
}


void recordPmemWriteback(char* rtnName, void* return_ip, uint64_t tid, 
                            ADDRINT arg1, ADDRINT arg2)
{
    if (arg2 == 0) return;

    assert(tid < MAX_THREADS);
    PinDEBUG(*out << "Funct: CLWB, Src: " << std::hex 
                    <<  arg1 << " Size: " << arg2 << endl);

    // Declare trace_entry before assignment
    trace_entry_t trace_entry;

    // Generate trace entry
    trace_entry.operation = CLWB;
    trace_entry.func_ret = false;
    trace_entry.tid = tid;
    trace_entry.src_addr = (addr_t)arg1;
    trace_entry.size = arg2;
    trace_entry.instr_ptr = (addr_t)return_ip;

    assert(0!=trace_entry.operation);
    assert(0!=trace_entry.src_addr && "Cannot flush address=0");
    // Send trace entry to trace_fifo
    trace_fifo.pinfifo_write(&trace_entry);

}

void recordPmemFence(char* rtnName, void* return_ip, uint64_t tid)
{
    // Only record trace in RoI
    // if (!roi_tracker.isInRoI(tid)) return;
    // This assertion is no longer true.
    if (strcmp(rtnName, "predrain_memory_barrier")){
        cerr << "Wrong sfence function. Funct name: " << rtnName <<endl;
    }
    assert(tid < MAX_THREADS);
    PinDEBUG(*out << "Funct: SFENCE" << endl);
    
    // Declare trace_entry before assignment
    trace_entry_t trace_entry;

    // Generate trace entry
    trace_entry.operation = SFENCE;
    trace_entry.func_ret = false;
    trace_entry.tid = tid;
    trace_entry.instr_ptr = (addr_t)return_ip;

    assert(0!=trace_entry.operation);
    // Send trace entry to trace_fifo
    trace_fifo.pinfifo_write(&trace_entry);

}

uint64_t* pm_alloc_size_ptr = NULL;

void recordPmemAllocation(char* rtnName, void* return_ip, uint64_t tid, ADDRINT size_ptr)
{
    // Always track PM allocation functions
    assert(pm_functions[string(rtnName)].type == PM_ALLOCATION_FUNC);
    assert(tid < MAX_THREADS);

#ifdef SKIP_FUNC_OP
	if (func_status_table[tid].status == CALLED) return;
#endif

    pm_alloc_size_ptr = (uint64_t*)size_ptr;

    PinDEBUG(*out << "Funct: " << rtnName
            << " tid: " << tid << "size_ptr=" << pm_alloc_size_ptr << endl);

    // Generate trace entry
    trace_entry_t trace_entry;
    trace_entry.operation = pm_functions[string(rtnName)].enum_name;
    trace_entry.func_ret = false;
    trace_entry.tid = tid;
    trace_entry.instr_ptr = (addr_t)return_ip;
    assert(0!=trace_entry.operation);
    // Send trace entry to trace_fifo
    trace_fifo.pinfifo_write(&trace_entry);

#ifdef SKIP_FUNC_OP
    func_status_table[tid].func_name = string(rtnName);
    func_status_table[tid].status = CALLED;
#endif
}


void recordPmemRetAllocation(char* rtnName, void* return_ip, uint64_t tid, ADDRINT addr)
{
    // Always track PM allocation functions

    assert(tid < MAX_THREADS);

#ifdef SKIP_FUNC_OP
    if (func_status_table[tid].status == CALLED && func_status_table[tid].func_name == string(rtnName)) {
        // reset function status after return
        func_status_table[tid].func_name = "";
        func_status_table[tid].status = RETURNED;
    } else if (func_status_table[tid].status == CALLED) {
        return;
    }
#endif

    assert(pm_alloc_size_ptr && "size pointer is invalid");

#ifdef SKIP_FUNC_OP
 	if (func_status_table[tid].status == CALLED) return;
#endif

    // Dereference the size pointer after function returns
 	uint64_t interval_size = *pm_alloc_size_ptr;
    pm_alloc_size_ptr = NULL;
    // Update interval set
    pm_addr_set.insert(ival::closed(addr, (uint64_t)interval_size+((uint64_t)addr)-1));
   
    PinDEBUG(*out << "FunctRet: " << rtnName 
                << " tid: " << tid 
                << " addr: " << (void*)addr 
                << " size: " << interval_size << endl);

    // Generate trace entry
    trace_entry_t trace_entry;
    trace_entry.operation = pm_functions[string(rtnName)].enum_name;
    trace_entry.func_ret = true;
    trace_entry.tid = tid;
    trace_entry.dst_addr = addr;
    trace_entry.size = interval_size;
    trace_entry.instr_ptr = (addr_t)return_ip;
    assert(0!=trace_entry.operation);
    // Send trace entry to trace_fifo
    trace_fifo.pinfifo_write(&trace_entry);
}

void recordPmemDeallocation(char* rtnName, void* return_ip, uint64_t tid, ADDRINT addr, ADDRINT size)
{
    // Always track PM deallocation functions
    PinDEBUG(*out << "Funct: " << rtnName 
             << " tid: " << tid << " addr: " << (void*)addr 
             << " size: " << size << endl);
    assert(tid < MAX_THREADS);

#ifdef SKIP_FUNC_OP
	if (func_status_table[tid].status == CALLED) return;
#endif

    pm_addr_set.erase(ival::closed(addr, size+((uint64_t)addr)-1));
    PinDEBUG(*out << "Funct: " << rtnName 
             << " tid: " << tid << " addr: " << (void*)addr 
             << " size: " << size << endl);

    // Generate trace entry
    trace_entry_t trace_entry;
    trace_entry.operation = pm_functions[string(rtnName)].enum_name;
    trace_entry.func_ret = false;
    trace_entry.tid = tid;
    trace_entry.src_addr = addr;
    trace_entry.size = size;
    trace_entry.instr_ptr = (addr_t)return_ip;
    assert(0!=trace_entry.operation);
    // Send trace entry to trace_fifo
    trace_fifo.pinfifo_write(&trace_entry);

#ifdef SKIP_FUNC_OP
    func_status_table[tid].func_name = string(rtnName);
    func_status_table[tid].status = CALLED;
#endif
}


void recordPmemRetCommon(char* rtnName, void* return_ip, uint64_t tid, ADDRINT addr)
{
    // Only record trace in RoI
    if (!roi_tracker.isInRoI(tid)) return;

    assert(tid < MAX_THREADS);

#ifdef SKIP_FUNC_OP
    if (func_status_table[tid].status == CALLED && func_status_table[tid].func_name == string(rtnName)) {
        // reset function status after return
        func_status_table[tid].func_name = "";
        func_status_table[tid].status = RETURNED;
    } else if (func_status_table[tid].status == CALLED) {
        return;
    }
#endif

    PinDEBUG(*out << "FunctRet: " << rtnName 
            << " tid: " << tid << " addr: " << (void*)addr << endl);

    // Generate trace entry
    trace_entry_t trace_entry;
    trace_entry.operation = pm_functions[string(rtnName)].enum_name;
    trace_entry.func_ret = true;
    trace_entry.tid = tid;
    trace_entry.dst_addr = addr;
    trace_entry.instr_ptr = (addr_t)return_ip;
    assert(0!=trace_entry.operation);
    // Send trace entry to trace_fifo
    trace_fifo.pinfifo_write(&trace_entry);
}

// Instruction instrument for read and write
void PMReadAndWriteInst(INS ins, void *v)
{
    //get the ins
    // Disassemble the instruction. Compare with CLWB
    std::string decodedInstruction = INS_Disassemble(ins).c_str();
    if(decodedInstruction.find("clwb")!=std::string::npos){
        PinDEBUG(cerr << "Found CLWB" << std::hex << " IP:" << INS_Address(ins) << endl;);
        return;
    }
    // Discard CLWB read trace
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        // Read -- only track PM read when read_enable is set
        if (read_enable && INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)recordPmemRead,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_MEMORYREAD_SIZE,
                IARG_THREAD_ID,
                IARG_END);
        }

        // Write -- always track PM write
        if (INS_MemoryOperandIsWritten(ins, memOp)){
            bool is_non_temporal_write = false;
            if(decodedInstruction.find("movnt")!=std::string::npos){
                is_non_temporal_write = true;
            }
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)recordPmemWrite,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_MEMORYWRITE_SIZE,
                IARG_THREAD_ID,
                IARG_BOOL,
                is_non_temporal_write,
                IARG_END);
        }
    }
}

void recordCommitVar(char* rtnName, void* return_ip, uint64_t tid, ADDRINT addr, ADDRINT size)
{
    // Always track PM deallocation functions
    PinDEBUG(*out << "Funct: " << rtnName 
             << " tid: " << tid << " addr: " << addr 
             << "size: " << size << endl);
    assert(tid < MAX_THREADS);

    // Generate trace entry
    trace_entry_t trace_entry;
    trace_entry.operation = pm_functions[string(rtnName)].enum_name;
    trace_entry.func_ret = false;
    trace_entry.tid = tid;
    trace_entry.src_addr = addr;
    trace_entry.size = size;
    trace_entry.instr_ptr = (addr_t)return_ip;
    assert(0!=trace_entry.operation);
    // Send trace entry to trace_fifo
    trace_fifo.pinfifo_write(&trace_entry);
}


// Routine instrument for Pmem
void PMOpTraceInstPmem(RTN rtn, void* v)
{
    RTN_Open(rtn);

    string func_name = RTN_Name(rtn);

    // Instrument routines based on function name
    if (func_name == "pmem_map_file") {
        // Instrumentation for pmem_map_file()
        // Call
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)recordPmemAllocation,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["pmem_map_file"].size,
            IARG_END);
        // Return
        RTN_InsertCall(
            rtn, IPOINT_AFTER, 
            (AFUNPTR)recordPmemRetAllocation, 
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCRET_EXITPOINT_VALUE,
            IARG_END);
    } else if (func_name == "pmem_unmap") {
        // Instrumentation for pmem_unmap()
        // Call
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)recordPmemDeallocation,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["pmem_unmap"].src,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["pmem_unmap"].size,
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
    } else if (func_name == "predrain_memory_barrier") {
        // Call
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)recordPmemFence,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_END);
    } else if (func_name == "flush_clwb_nolog" || 
               func_name == "flush_clflushopt_nolog" || 
               func_name == "flush_clflush_nolog") {
            //    func_name == "pmem_flush" || 
            //    func_name == "pmem_deep_flush") {
        // Call
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)recordPmemWriteback,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["clwb"].src,
            IARG_FUNCARG_ENTRYPOINT_VALUE,
            pm_functions["clwb"].size,
            IARG_END);
    } else if (func_name == "_add_commit_var") {
        // Track commit variable
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR)recordCommitVar,
            IARG_ADDRINT,
            RTN_Name(rtn).c_str(),
            IARG_RETURN_IP,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 
            pm_functions["_add_commit_var"].src,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 
            pm_functions["_add_commit_var"].size,
            IARG_END);
    } else {
        // Do not track INVALID operations
    } // func_name

    RTN_Close(rtn);
}


#endif
