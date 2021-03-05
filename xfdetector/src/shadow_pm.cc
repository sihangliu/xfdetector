#include "xfdetector.hh"
#include <sys/time.h>

ShadowPM::ShadowPM()
{
    memset(pre_InternalFunctLevel, 0, sizeof(pre_InternalFunctLevel));
    memset(tx_level, 0, sizeof(tx_level));
    memset(skipDetectionStatus, 0, sizeof(skipDetectionStatus));
}

ShadowPM::ShadowPM(const ShadowPM& in)
{
    //cerr << pm_modify_timestamps << endl;
    pm_status = in.pm_status;
    pm_modify_timestamps = in.pm_modify_timestamps;
    global_timestamp = in.global_timestamp;
    commit_var_set_addr = in.commit_var_set_addr;
    write_addr_IP_mapping = in.write_addr_IP_mapping;
    commit_timestamp = in.commit_timestamp;
    // Init levels to 0
    memset(tx_level, 0, sizeof(tx_level));
    memset(pre_InternalFunctLevel, 0, sizeof(pre_InternalFunctLevel));

    for(int i=0;i<MAX_THREADS;i++){
        //pre_InternalFunctLevel[i] = in.pre_InternalFunctLevel[i];
        tx_added_addr[i] = in.tx_added_addr[i];
        // tx_alloc_addr[i] = in.tx_alloc_addr[i];
        tx_added_addr_IP_mapping[i] = in.tx_added_addr_IP_mapping[i];
        tx_alloc_addr_IP_mapping[i] = in.tx_added_addr_IP_mapping[i];
        tx_non_added_write_addr[i] = in.tx_non_added_write_addr[i];
    }
}


void ShadowPM::add_pm_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    XFD_ASSERT(size && addr);

    // Check if part of the address has already been allocated
    if (MAP_INTERSECT(pm_status, addr, size))
        ERROR(op_ptr, "Allocate on existing PM locations");

    // Add address to PM locations
    MAP_UPDATE(pm_status, addr, size, CLEAN);
}

void ShadowPM::add_pm_addr_post(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    XFD_ASSERT(size && addr);
    // Do nothing
    // Check if part of the address has already been allocated
    /*
    if (MAP_INTERSECT(pm_status, addr, size))
        ERROR(op_ptr, "Allocate on existing PM locations");
    
    // Add address to PM locations
    MAP_UPDATE(pm_status, addr, size, CLEAN);
    */
}

void ShadowPM::remove_pm_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    XFD_ASSERT(size && addr);

    // Check if address has already been allocated
    if (!is_pm_addr(op_ptr, addr, size)) 
        ERROR(op_ptr, "Deallocating unallocated memory");

    // Remove address from PM locations
    MAP_REMOVE(pm_status, addr, size);
}

void ShadowPM::writeback_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    // Check unnecessary writeback
    for(auto &it : MAP_LOOKUP(pm_status, addr, size)) {
        if (it.second == WRITEBACK_PENDING) {
            WARN(op_ptr, "Unnecessary writeback");
            //if (is_in_pre_internal_funct(op_ptr->tid)) {
                cerr << "\033[1;33mUnnecessary Flush\033[0m Addr: " << std::hex << addr 
                    << " Size: " << size << " IP: " << op_ptr->instr_ptr << endl;
            //}
            print_IP_linenumber_mapping(op_ptr->instr_ptr, PRE_FAILURE);
        }
    }
    
    // Update status to WRITEBACK_PENDING
    MAP_UPDATE(pm_status, addr, size, WRITEBACK_PENDING);
}

void ShadowPM::drain_writeback(trace_entry_t* op_ptr)
{
    unsigned drain_count = 0;
    // Change all WRITEBACK_PENDING to WRITTEN_BACK
    for(auto &it : pm_status) {
        if (it.second == WRITEBACK_PENDING) {
            it.second = WRITTEN_BACK;
            drain_count++;
        }
    }
    // If no PM location has been drained, the SFENCE is unnecessary
    if (!drain_count) {
        // FIXME: remove double fence 
        /*
        WARN(op_ptr, "Unnecessary PM drain");
        cerr << "Unnecessary PM drain IP: " << std::hex << op_ptr->instr_ptr << endl;
        */
    }
}

void ShadowPM::modify_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    XFD_ASSERT(size && addr);

    // Check if the address is on PM
    if (!is_pm_addr(op_ptr, addr, size)) ERROR(op_ptr, "Modify non-PM address");

    // Update status to MODIFIED
    MAP_UPDATE(pm_status, addr, size, MODIFIED);
    // Update to the latest timestamp
    MAP_UPDATE(pm_modify_timestamps, addr, size, global_timestamp);

    DEBUG(fprintf(stderr, "modtimestamp: %d\n", global_timestamp););
}

void ShadowPM::set_consistent_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    XFD_ASSERT(size && addr);

    // Check if the address is on PM
    if (!is_pm_addr(op_ptr, addr, size)) ERROR(op_ptr, "Non-PM address is never consistent");

    MAP_UPDATE(pm_status, addr, size, CONSISTENT);
}

void ShadowPM::increment_global_time()
{
    global_timestamp++;
}

bool ShadowPM::is_consistent(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    XFD_ASSERT(size && addr);

    // Check if the address is on PM
    if (!is_pm_addr(op_ptr, addr, size)) ERROR(op_ptr, "Check non-PM address");

    for (auto &it: MAP_LOOKUP(pm_status, addr, size)) {
        if (it.second != CONSISTENT && it.second != CLEAN) return false;        
    }

    return true;
}

bool ShadowPM::is_writtenback(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    XFD_ASSERT(size && addr);

    // Check if the address is on PM
    if (!is_pm_addr(op_ptr, addr, size)) ERROR(op_ptr, "Check non-PM address");
    
    for (auto &it: MAP_LOOKUP(pm_status, addr, size)) {
        if (it.second != WRITTEN_BACK) return false;
    }

    return true;
}

bool ShadowPM::is_pm_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    // XFD_ASSERT(size && addr);
    // return IN_MAP(pm_status, addr, size);
    return (addr + size < PM_ADDR_SIZE + PM_ADDR_BASE) && (addr >= PM_ADDR_BASE);
}

void ShadowPM::reset_internal_funct_level(int tid){
    // cerr << "Tid: " << tid << " Reset func level" << endl;
    pre_InternalFunctLevel[tid] = 0;
}

void ShadowPM::increment_pre_internal_funct_level(int tid){
    pre_InternalFunctLevel[tid]++;
}

void ShadowPM::decrement_pre_internal_funct_level(int tid){
    XFD_ASSERT((pre_InternalFunctLevel[tid] > 0) && "internal funct level < 0\n");
    pre_InternalFunctLevel[tid]--;
}

bool ShadowPM::is_in_pre_internal_funct(int tid){
    return (pre_InternalFunctLevel[tid] > 0);
}

void ShadowPM::increment_tx_level(int tid, int stage){
    tx_level[tid]++;
    DEBUG(cout << "tx_level[" << tid << "]: " << tx_level << endl;);
}

void ShadowPM::decrement_tx_level(int tid, int stage){
    XFD_ASSERT(tx_level[tid]>0);
    tx_level[tid]--;
    //cout << "tx_level[" << tid << "]: " << tx_level << endl;
    if(0==tx_level[tid]){
        // Commit staged changes to shadow PM
        // Need to iterate all members of tx_added_addr[tid]
        DEBUG(cout << "Draining writes" << endl);
        for (auto &i : tx_added_addr[tid]) {
            // Performance bug detection
            if (stage == PRE_FAILURE) {
                size_t size = i.upper() - i.lower() + 1;
                addr_t addr = i.lower();
                bool bug_flag = true;
                for (auto &k : MAP_LOOKUP(pm_status, addr, size)) {
                    if (k.second != CONSISTENT && k.second != CLEAN) {
                        bug_flag = false;
                        break;
                    }
                }
                if (bug_flag) {
                    cerr << "\033[1;33mPerformance Bug:\033[0m\nUnnecessary TX_ADD, added but never modified" << endl;
                    cerr << "Added addr = " << (void*)addr << " size = " << size << endl;
                    for (auto &j : MAP_LOOKUP(tx_added_addr_IP_mapping[tid], addr, size)) {
                        addr_t instr_ptr = j.second;
                        cerr << "Previously added by IP (TX_ADD) = " << (void*)instr_ptr << endl;
                        print_IP_linenumber_mapping(instr_ptr, PRE_FAILURE);
                        break;
                    }
                    for (auto &j : MAP_LOOKUP(tx_alloc_addr_IP_mapping[tid], addr, size)) {
                        addr_t instr_ptr = j.second;
                        cerr << "Previously added by IP (TX_ALLOC) = " << (void*)instr_ptr << endl;
                        print_IP_linenumber_mapping(instr_ptr, PRE_FAILURE);
                        break;
                    }
                }
            }
            DEBUG(cout << std::hex << i << endl;);
            MAP_UPDATE_INTERVAL(pm_status, i, CONSISTENT);
        }

        // Non-ADDed address is updated to shadow PM during the write.
        // Should not need to do anything here.

        // clear staged changes
        SET_CLEAR(tx_added_addr[tid]);
        // SET_CLEAR(tx_alloc_addr[tid]);
        MAP_CLEAR(tx_added_addr_IP_mapping[tid]);
        MAP_CLEAR(tx_alloc_addr_IP_mapping[tid]);
        SET_CLEAR(tx_non_added_write_addr[tid]);
        // increment timestamp
        // increment_global_time();
    }
}
bool ShadowPM::is_in_tx(int tid){
    return (0<tx_level[tid]);
}

bool ShadowPM::is_added_addr(trace_entry_t* op_ptr, addr_t addr, size_t size){
    int tid = op_ptr->tid;
    return SET_LOOKUP(tx_added_addr[tid], addr, size);
}
bool ShadowPM::is_non_added_write_addr(trace_entry_t* op_ptr, addr_t addr, size_t size){
    int tid = op_ptr->tid;
    return SET_LOOKUP(tx_non_added_write_addr[tid], addr, size);
}

bool ShadowPM::print_IP_linenumber_mapping(addr_t ip, int stage)
{
    bool found = false;

    string fname;
    if (stage == PRE_FAILURE)
        fname = "/tmp/backtrace_pre." + std::to_string(exec_id);
    else if (stage == POST_FAILURE)
        fname = "/tmp/backtrace_post." + std::to_string(exec_id);

    std::ifstream ifs(fname.c_str());
    string line;
    int bt_pos = 0;
    while (getline(ifs, line)) {
        bt_pos++;
        int start = line.find(">>");

        if (start == 0 && stoull(line.substr(start+3, 14), NULL, 16) == ip) {
            // Found
            found = true;
            fprintf(stderr, "Position in backtrace File: %d (later=recent)\n", bt_pos);
            // Maximum backtracing
            int count = 0;
            while (count < MAX_BACKTRACE) {
                getline(ifs, line);
                // Stop if current tracetrace ends or cannot get useful output
                if (line.find(">>") == 0 || 
                    line.find(":") == string::npos) {
                    break;
                }
                fprintf(stderr, "[#%d]\t%s\n", count, line.c_str());
                count++;
            }
            break;
        }
    }

    ifs.close();
    return found;
}

// void ShadowPM::add_tx_alloc_addr(trace_entry_t* op_ptr, addr_t addr, size_t size, int stage) {
//     int tid = op_ptr->tid;
//     SET_INSERT(tx_alloc_addr[tid], addr, size);
// }

void ShadowPM::add_tx_add_addr(trace_entry_t* op_ptr, addr_t addr, size_t size, int stage, bool alloc){
    int tid = op_ptr->tid;
    if(is_non_added_write_addr(op_ptr, addr, size)){
        // if (stage == PRE_FAILURE) {
            cerr << "\033[0;31mConsistency Bug:\033[0m\nTX_ADD after modification" << endl;
            cerr << "Write IP: " << (void*)op_ptr->instr_ptr << " Write Addr: " << (void*)addr << endl;
            print_IP_linenumber_mapping(op_ptr->instr_ptr, PRE_FAILURE);
        // }
    }

    if (is_added_addr(op_ptr, addr, size)) {
        if (stage == PRE_FAILURE) {
            cerr << "\033[1;33mPerformance Bug:\033[0m\nUnnecessary TX_ADD, already added" << endl;
            cerr << "TX_ADD IP = " << (void*)op_ptr->instr_ptr << endl;
            cerr << "Added addr = " << (void*)addr << " size = " << size << endl;
            print_IP_linenumber_mapping(op_ptr->instr_ptr, PRE_FAILURE);
            for (auto &j : MAP_LOOKUP(tx_added_addr_IP_mapping[tid], addr, size)) {
                addr_t instr_ptr = j.second;
                cerr << "Added by IP (TX_ADD) = " << (void*)instr_ptr << endl;
                print_IP_linenumber_mapping(instr_ptr, PRE_FAILURE);
                break;
            }
            for (auto &j : MAP_LOOKUP(tx_alloc_addr_IP_mapping[tid], addr, size)) {
                addr_t instr_ptr = j.second;
                cerr << "Added by IP (TX_ALLOC) = " << (void*)instr_ptr << endl;
                print_IP_linenumber_mapping(instr_ptr, PRE_FAILURE);
                break;
            }
        }
    }

    DEBUG(cerr << "inserting tid: " << tid << " addr: " << addr << " size: " << size <<  endl;);

    assert(addr != 0 && size != 0 && "TX_ADD-ed address/size should not be zero");
    SET_INSERT(tx_added_addr[tid], addr, size);
    if (!alloc) {
        MAP_UPDATE(tx_added_addr_IP_mapping[tid], addr, size, op_ptr->instr_ptr);
    } else {
        MAP_UPDATE(tx_alloc_addr_IP_mapping[tid], addr, size, op_ptr->instr_ptr);
    }
    // cerr << "TX_ADD IP : " << op_ptr->instr_ptr << " " << op_ptr->func_ret << endl;
    //cout << "inserted" << endl;
}

void ShadowPM::add_non_tx_add_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    int tid = op_ptr->tid;
    SET_INSERT(tx_non_added_write_addr[tid], addr, size);
    //cerr << "Added non tx add address" << endl;
}

interval_set_addr ShadowPM::get_tx_added_addr(int tid)
{
    return tx_added_addr[tid];
}

void ShadowPM::add_commit_var_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    // cerr << "inserting commit var addr: " << addr << " size: " << size <<  endl;
    DEBUG(cerr << "inserting commit var addr: " << addr << " size: " << size <<  endl;);
    SET_INSERT(commit_var_set_addr, addr, size);
}

bool ShadowPM::is_commit_var_addr(trace_entry_t* op_ptr, addr_t addr, size_t size){
    
    // if (SET_LOOKUP(commit_var_set_addr, addr, size))
    //     cerr << "Lookup found" << endl;
    
    return SET_LOOKUP(commit_var_set_addr, addr, size);
}

bool ShadowPM::is_recent_commit_update(trace_entry_t* op_ptr, addr_t addr, size_t size){
    // Find max time stamp of the intervals
    int maxTimeStamp = -2;
    for(auto &it : MAP_LOOKUP(pm_modify_timestamps, addr, size)) {
        maxTimeStamp = (maxTimeStamp > it.second) ? maxTimeStamp : it.second;
        DEBUG(fprintf(stderr, "timeStamp: %d\n", it.second););
    }
    DEBUG(fprintf(stderr ,"global time stamp: %d, maxTimeStamp: %d", global_timestamp, maxTimeStamp););
    if(commit_timestamp < 0){
        return true;
    }
    // if (commit_timestamp <= maxTimeStamp) {
    //     cerr << "Addr: " << std::hex << addr << std::dec << " TS: " << maxTimeStamp << endl;
    // }
    return commit_timestamp > maxTimeStamp;
}

void ShadowPM::update_commitVar_timestamp(){
    cerr << "Updating commit Var to timestamp " << std::dec << global_timestamp << endl;
    commit_timestamp = global_timestamp;
}

void ShadowPM::add_write_addr_IP_mapping(trace_entry_t* op_ptr){
    XFD_ASSERT(op_ptr->operation == WRITE);
    addr_t addr = op_ptr->dst_addr;
    size_t size = op_ptr->size;
    addr_t instr_ptr = op_ptr->instr_ptr;
    XFD_ASSERT(addr && size);
    MAP_UPDATE(write_addr_IP_mapping, addr, size, instr_ptr);
}

bool ShadowPM::print_look_up_write_addr_IP_mapping(trace_entry_t* op_ptr, addr_t addr, size_t size, FILE* file)
{
    bool isWriteAddrFound = false;
    XFD_ASSERT(op_ptr->operation == READ);
    XFD_ASSERT(addr && size);
    //cerr << write_addr_IP_mapping << endl;
    fprintf(file, "\033[0;31mConsistency Bug:\033[0m\n");
    for(auto &it : MAP_LOOKUP(write_addr_IP_mapping, addr, size)){
        fprintf(file, "Write IP: %p\n", (void*)it.second);
        print_IP_linenumber_mapping(it.second, PRE_FAILURE);
        isWriteAddrFound = true;
    }
    return isWriteAddrFound;
}

bool ShadowPM::lookup_checked_addr(addr_t addr, size_t size)
{
    XFD_ASSERT(addr && size);
    XFD_ASSERT(size<(1<<16));
    uint64_t lookup_key = (addr<<16) | size;
    return (checked_addr_set.find(lookup_key) != checked_addr_set.end());
}

void ShadowPM::insert_checked_addr(addr_t addr, size_t size)
{
    XFD_ASSERT(addr && size);
    uint64_t insert_key = (addr<<16) | size;
    checked_addr_set.insert(insert_key);
}

void ShadowPM::disable_detection(int tid)
{
    skipDetectionStatus[tid]=1;
}

void ShadowPM::enable_detection(int tid)
{
    skipDetectionStatus[tid]=0;
}

int ShadowPM::is_detection_disabled(int tid)
{
    return skipDetectionStatus[tid]==1;
}

bool ShadowPM::printInconsistentReadDebug(trace_entry_t* cur_trace){
    pm_op_t operation = cur_trace->operation;
    bool func_ret = cur_trace->func_ret;
    int tid = cur_trace->tid;
    addr_t src_addr = cur_trace->src_addr;
    addr_t dst_addr = cur_trace->dst_addr;
    size_t size = cur_trace->size;
    addr_t instr_ptr = cur_trace->instr_ptr;
    bool isAddrFound = this->print_look_up_write_addr_IP_mapping(cur_trace, src_addr, size, stderr);
    if(isAddrFound){
        // Suppress non-user code report
        cerr << "Addr: " << (void*)cur_trace->src_addr << ", Size: " << size << endl;
        cerr << "Read IP: " << (void*)instr_ptr << endl;;
        print_IP_linenumber_mapping(instr_ptr, POST_FAILURE);
    }
    return isAddrFound;
}
