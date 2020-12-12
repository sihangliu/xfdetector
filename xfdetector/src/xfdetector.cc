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
        //tx_non_added_write_addr[i] = in.tx_non_added_write_addr[i];
    }
}


void ShadowPM::add_pm_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    DEBUG(assert(size && addr));

    // Check if part of the address has already been allocated
    if (MAP_INTERSECT(pm_status, addr, size))
        ERROR(op_ptr, "Allocate on existing PM locations");

    // Add address to PM locations
    MAP_UPDATE(pm_status, addr, size, CLEAN);
    //MAP_UPDATE(pm_status, addr, size, CONSISTENT);
}

void ShadowPM::add_pm_addr_post(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    DEBUG(assert(size && addr));
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
    DEBUG(assert(size && addr));

    // Check if address has already been allocated
    if (!is_pm_addr(op_ptr, addr, size)) 
        ERROR(op_ptr, "Deallocating unallocated memory");

    // Remove address from PM locations
    MAP_REMOVE(pm_status, addr, size);
}

void ShadowPM::writeback_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    //fprintf(stderr, "writeback addr: %p, size: %p\n", addr, size);
    DEBUG(assert(size && addr));

    // Check if the address is on PM
    if (!is_pm_addr(op_ptr, addr, size)) 
        ERROR(op_ptr, "Writeback non-PM address");

    // Check unncessary writeback
    for(auto &it : MAP_LOOKUP(pm_status, addr, size)) {
        if (it.second == WRITEBACK_PENDING) {
            WARN(op_ptr, "Unnecessary writeback");
            //if (is_in_pre_internal_funct(op_ptr->tid)) {
                cerr << "Unnecessary Flush Addr: " << std::hex << addr 
                    << " Size: " << size << " IP: " << endl; //<< op_ptr->instr_ptr << endl;
            //}
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
    DEBUG(assert(size && addr));

    // Check if the address is on PM
    if (!is_pm_addr(op_ptr, addr, size)) ERROR(op_ptr, "Modify non-PM address");

    // Update status to MODIFIED
    MAP_UPDATE(pm_status, addr, size, MODIFIED);
    // Update to the latest timestamp
    MAP_UPDATE(pm_modify_timestamps, addr, size, global_timestamp);
    // cerr << "Update to " << std::hex << addr << std::dec << " at time " << global_timestamp << endl;
    DEBUG(fprintf(stderr, "modtimestamp: %d\n", global_timestamp););
}

/*
void ShadowPM::modify_addr_internal(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    DEBUG(assert(size && addr));

    // Check if the address is on PM
    if (!is_pm_addr(op_ptr, addr, size)) ERROR(op_ptr, "Modify non-PM address");

    // Update status to MODIFIED
    MAP_UPDATE(pm_status, addr, size, MODIFIED);
    // Update to the latest timestamp
    //cerr << pm_modify_timestamps << endl;
    MAP_UPDATE(pm_modify_timestamps, addr, size, -1);
    //cerr << pm_modify_timestamps << endl;
}
*/

void ShadowPM::set_consistent_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    //cout << "set_consistent: " << addr << endl;
    DEBUG(assert(size && addr));

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
    DEBUG(assert(size && addr));

    // Check if the address is on PM
    if (!is_pm_addr(op_ptr, addr, size)) ERROR(op_ptr, "Check non-PM address");

    for (auto &it: MAP_LOOKUP(pm_status, addr, size)) {
        if (it.second != CONSISTENT && it.second != CLEAN) return false;        
    }

    return true;
}

bool ShadowPM::is_writtenback(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    DEBUG(assert(size && addr));

    // Check if the address is on PM
    if (!is_pm_addr(op_ptr, addr, size)) ERROR(op_ptr, "Check non-PM address");
    
    for (auto &it: MAP_LOOKUP(pm_status, addr, size)) {
        if (it.second != WRITTEN_BACK) return false;
    }

    return true;
}

bool ShadowPM::is_pm_addr(trace_entry_t* op_ptr, addr_t addr, size_t size)
{
    DEBUG(assert(size && addr));
    // return IN_MAP(pm_status, addr, size);
    return (addr + size < PM_ADDR_SIZE + PM_ADDR_BASE) && (addr >= PM_ADDR_BASE);
}

void ShadowPM::increment_pre_internal_funct_level(int tid){
    pre_InternalFunctLevel[tid]++;
}

void ShadowPM::decrement_pre_internal_funct_level(int tid){
    assert((pre_InternalFunctLevel[tid] > 0) && "internal funct level < 0\n");
    pre_InternalFunctLevel[tid]--;
}

bool ShadowPM::is_in_pre_internal_funct(int tid){
    return (pre_InternalFunctLevel[tid] > 0);
}

void ShadowPM::increment_tx_level(int tid){
    tx_level[tid]++;
    DEBUG(cout << "tx_level[" << tid << "]: " << tx_level << endl;);
}
void ShadowPM::decrement_tx_level(int tid){
    assert(tx_level[tid]>0);
    tx_level[tid]--;
    //cout << "tx_level[" << tid << "]: " << tx_level << endl;
    if(0==tx_level[tid]){
        // Commit staged changes to shadow PM
        // Need to iterate all members of tx_added_addr[tid]
        DEBUG(cout << "Draining writes" << endl);
        for (auto &i : tx_added_addr[tid]) {
            DEBUG(cout << std::hex << i << endl;);
            MAP_UPDATE_INTERVAL(pm_status, i, CONSISTENT);
        }
        //cout << pm_status << endl;

        // Non-ADDed address is updated to shadow PM during the write.
        // Should not need to do anything here.

        // clear staged changes
        SET_CLEAR(tx_added_addr[tid]);
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
    //TODO: Need check
    return SET_LOOKUP(tx_added_addr[tid], addr, size);
}
bool ShadowPM::is_non_added_write_addr(trace_entry_t* op_ptr, addr_t addr, size_t size){
    int tid = op_ptr->tid;
    //TODO: Need check
    return SET_LOOKUP(tx_non_added_write_addr[tid], addr, size);
}

void print_IP_linenumber_mapping(addr_t writeip)
{
    ifstream ifs;
    ifs.open("/tmp/func_map", std::ios::in | std::ios::binary);
    string ip;
    int linenum;
    string filename;
    
    while (ifs >> ip >> linenum >> filename) {
        if (std::stoul(ip, nullptr, 16) == writeip) {
            fprintf(stderr, "  Filename: %s, line: %d.\n", filename.c_str(), linenum);
            break;
        }
    }
    ifs.close();
}

void ShadowPM::add_tx_add_addr(trace_entry_t* op_ptr, addr_t addr, size_t size, int stage){
    int tid = op_ptr->tid;
    // TODO: check for duplicate TX_ADD addresses
    if(is_non_added_write_addr(op_ptr, addr, size)){
        if (stage == PRE_FAILURE) {
            cerr << "\033[0;31mConsistency Bug:\033[0m TX_ADD after modification. Write IP: " << std::hex << op_ptr->instr_ptr << " Write Addr: " << addr << endl;
            print_IP_linenumber_mapping(op_ptr->instr_ptr);
        }
    }

    if (is_added_addr(op_ptr, addr, size)) {
        if (stage == PRE_FAILURE) {
            cerr << "\033[1;33mPerformance Bug:\033[0m Unnecessary TX_ADD" << endl;
            print_IP_linenumber_mapping(op_ptr->instr_ptr);
        }
    }

    DEBUG(cerr << "inserting tid: " << tid << " addr: " << addr << " size: " << size <<  endl;);
    //cout << "preinserted" << endl;
    SET_INSERT(tx_added_addr[tid], addr, size);
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
    assert(op_ptr->operation == WRITE);
    addr_t addr = op_ptr->dst_addr;
    size_t size = op_ptr->size;
    addr_t instr_ptr = op_ptr->instr_ptr;
    assert(addr && size);
    MAP_UPDATE(write_addr_IP_mapping, addr, size, instr_ptr);
}

bool ShadowPM::print_look_up_write_addr_IP_mapping(trace_entry_t* op_ptr, addr_t addr, size_t size, FILE* file)
{
    bool isWriteAddrFound = false;
    assert(op_ptr->operation == READ);
    assert(addr && size);
    //cerr << write_addr_IP_mapping << endl;
    
    for(auto &it : MAP_LOOKUP(write_addr_IP_mapping, addr, size)){
        fprintf(file, "\033[0;31mConsistency Bug:\033[0m Associate Write IP: %p.\n", (void*)it.second);
        print_IP_linenumber_mapping(it.second);
        isWriteAddrFound = true;
    }
    return isWriteAddrFound;
}

bool ShadowPM::lookup_checked_addr(addr_t addr, size_t size)
{
    assert(addr && size);
    assert(size<(1<<16));
    uint64_t lookup_key = (addr<<16) | size;
    return (checked_addr_set.find(lookup_key) != checked_addr_set.end());
}

void ShadowPM::insert_checked_addr(addr_t addr, size_t size)
{
    assert(addr && size);
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

void ExeCtrl::init(char* path)
{
    // If user specifies config path, change the path
    if (path) {
        config_file = string(path);
    } else {
        char path_buffer[1024];
        if (!getcwd(path_buffer, sizeof(path_buffer)))
            ERR("Cannot get current directory");
        config_file = string(path_buffer) + "/config.txt";
    }
    // Parse commands according to config file    
    parse_exec_command();
}

// void ExeCtrl::execute_pre_failure()
// {
//     if (system(genPinCommand(PRE_FAILURE, pm_image_name).c_str()) < 0)
//         ERR("Execution of pre-failure command failed.");
// }

void ExeCtrl::execute_post_failure()
{   
    string image_copy_name = copy_pm_image();

    // Execute recovery code on the PM image copy
    // string image_copy_name = copy_name_queue.front();
    string post_failure_command = genPinCommand(POST_FAILURE, image_copy_name) + string(" 2>> post.out");
    
    if (system(post_failure_command.c_str()) < 0) {
        std::cerr << "Error: " << strerror(errno) << '\n';
        // cerr << getExeName()<< endl;
        // int rtn = system(("killall -9 " + getExeName()).c_str());
        // exit(1);
        // ERR("Execution of post-failure command failed.");
    }
    // Set post-failure execution to completed
    // race_detector.post_testing_complete = true;

    // Remove copied image
    remove(image_copy_name.c_str());
}

string ExeCtrl::copy_pm_image()
{
    // Use tid to name copy image
    string copy_name = pm_image_name + "_xfdetector_" + std::to_string(pthread_self());
    // copy_file(path(pm_image_name), path(copy_name), copy_option::overwrite_if_exists);
    string copy_command = "cp " + pm_image_name + " " + copy_name;
    // cerr << copy_command << endl;
    if (system(copy_command.c_str()) < 0)
        ERR("Cannot copy image: " + pm_image_name);
    return copy_name;
}

void ExeCtrl::parse_exec_command()
{
    // Config file usage
    string usage = "Config file follows this format: \n\
                    PINTOOL_PATH </path/to/xfdetector/pintool> \n\
                    PM_IMAGE </path/to/pm/image> \n\
                    EXEC_PATH </path/to/executable/under/testing> \n\
                    PRE_FAILURE_COMMAND <command for pre-failure execution> \n\
                    POST_FAILURE_COMMAND <command for post-failure execution>";


    // Open file
    std::ifstream infile(config_file);
    string line;
    //fprintf(stderr ,"djklfjksdlfjsljfskl\n");
    int line_count = 0;
    while (std::getline(infile, line)) {
        string name = line.substr(0, line.find(" "));
        string content = line.substr(line.find(" ") + 1, line.length());
        if (name == "PINTOOL_PATH") {
            pintool_path = content;
        } else if (name == "PM_IMAGE") {
            pm_image_name = content;
        } else if (name == "EXEC_PATH") {
            executable_path = content;
        } else if (name == "PRE_FAILURE_COMMAND") {
            pre_failure_exec_command = content;
        } else if (name == "POST_FAILURE_COMMAND") {
            assert(pm_image_name != "" && "Specify PM_IMAGE name before POST_FAILURE_COMMAND");
            post_failure_exec_command_part1
                = content.substr(0, content.find(pm_image_name));
            post_failure_exec_command_part2
                = content.substr(content.find(pm_image_name) + 
                    pm_image_name.length(), content.length());
        } else {
            cout << usage << endl;
            assert(0 && "Incorrect option name");
        }
        line_count++;
    }

    if (line_count != NUM_OPTIONS) {
        cout << usage << endl;
        assert(0 && "Insufficient options in config file");
    }
}

string ExeCtrl::getExeName() 
{
    for (int i = executable_path.length(); i >= 0; --i) {
        if(executable_path[i] == '/') {
            return executable_path.substr(i+1, executable_path.length()-1);
        }
    }
}


void ExeCtrl::kill_proc(unsigned pid)
{
    if (kill(pid, 9) < 0) ERR("Cannot kill process: " + std::to_string(pid));
}

string ExeCtrl::genPinCommand(int stage, string pm_image_name)
{
    const char *pin_root = std::getenv("PIN_ROOT");
    if (strcmp(pin_root, "") != 0) {
        if (stage == PRE_FAILURE) {
            return std::string(pin_root) + "/pin -t " + pintool_path + " " + pin_pre_failure_option 
                    + " -- " +  pre_failure_exec_command;
        } else if (stage == POST_FAILURE) {
            // cerr << "PIN POST COMMAND: " 
            //     << ("export POST_FAILURE=1; " + std::string(pin_root) + "/pin -t " + pintool_path + " " + pin_post_failure_option 
            //         + " -- " + post_failure_exec_command_part1 + pm_image_name 
            //         + post_failure_exec_command_part2) << endl;
            return "export POST_FAILURE=1; " + std::string(pin_root) + "/pin -t " + pintool_path + " " + pin_post_failure_option 
                    + " -- " + post_failure_exec_command_part1 + pm_image_name 
                    + post_failure_exec_command_part2;
        }
    }
    else {
        assert(0 && "Environment PIN_ROOT not set.");
    }
}

void XFDetectorFIFO::fifo_create()
{
    // remove old FIFOs
    remove(PRE_FAILURE_FIFO);
    remove(POST_FAILURE_FIFO);
    remove(SIGNAL_FIFO);

    // Create pre-failure FIFO
    if (mkfifo(PRE_FAILURE_FIFO, 0666) < 0) {
        ERR("Pre-failure FIFO create failed.");
    }
    // Creast post-failure FIFO
    if (mkfifo(POST_FAILURE_FIFO, 0666) < 0) {
        ERR("Post-failure FIFO create failed.");
    }
    // Create Signal FIFO
    if (mkfifo(SIGNAL_FIFO, 0666) < 0) {
        ERR("Signal FIFO create failed.");
    }
}

void XFDetectorFIFO::fifo_open(const char* name)
{
    if (!strcmp(name, PRE_FAILURE_FIFO)) {
        pre_fifo_fd = open(PRE_FAILURE_FIFO, O_RDONLY);
        if (pre_fifo_fd < 0) ERR("Pre-failure FIFO open failed.");
    } else if (!strcmp(name, POST_FAILURE_FIFO)) {
        post_fifo_fd = open(POST_FAILURE_FIFO, O_RDONLY);
        if (post_fifo_fd < 0) ERR("Post-failure FIFO open failed.");
    } else if (!strcmp(name, SIGNAL_FIFO)) {
        signal_fifo_fd = open(SIGNAL_FIFO, O_RDWR);
        if (signal_fifo_fd < 0) ERR("Signal FIFO open failed.");
    } else {
        ERR("Open unknown FIFO");
    }
}

void XFDetectorFIFO::fifo_close(const char* name)
{
    if (!strcmp(name, PRE_FAILURE_FIFO)) {
        close(pre_fifo_fd);
    } else if (!strcmp(name, POST_FAILURE_FIFO)) {
        close(post_fifo_fd);
    } else if (!strcmp(name, SIGNAL_FIFO)) {
        close(signal_fifo_fd);
    } else {
        ERR("Close unknown FIFO");
    }
}

int XFDetectorFIFO::pre_fifo_read()
{
    return read(pre_fifo_fd, pre_fifo_buf, PIN_FIFO_BUF_SIZE);
}

int XFDetectorFIFO::post_fifo_read()
{
    return read(post_fifo_fd, post_fifo_buf, PIN_FIFO_BUF_SIZE);
}

int XFDetectorFIFO::signal_send(char* message, unsigned len)
{
    return write(signal_fifo_fd, message, len);
}

int XFDetectorFIFO::signal_recv()
{
    return read(signal_fifo_fd, signal_buf, MAX_SIGNAL_LEN);
}

void XFDetectorFIFO::pin_continue_send()
{
    char message[] = PIN_CONTINUE_SIGNAL;
    // cerr << strlen(message) << endl;
    int send_size = signal_send(message, strlen(message)+1);
    if (send_size < 0) {
        ERR("PIN_CONTINUE_SIGNAL send failure.");
    }        //MAP_UPDATE_INTERVAL(pm_status, tx_added_addr[tid], CONSISTENT);
}

trace_entry_t* XFDetectorFIFO::get_trace(int stage, unsigned index)
{
    if (stage == PRE_FAILURE) {
        return pre_fifo_buf + index;
    } else if (stage == POST_FAILURE) {
        return post_fifo_buf + index;
    }
    // Should never reach here
    return NULL;
}

XFDetectorFIFO::XFDetectorFIFO()
{
    // Initialize FIFOs
    fifo_create();
    fifo_open(PRE_FAILURE_FIFO);
    fifo_open(SIGNAL_FIFO);

    // Allocate FIFO buffers
    pre_fifo_buf = (trace_entry_t*) malloc(PIN_FIFO_BUF_SIZE);
    post_fifo_buf = (trace_entry_t*) malloc(PIN_FIFO_BUF_SIZE);
    signal_buf = (char*) malloc(MAX_SIGNAL_LEN);
}

XFDetectorFIFO::~XFDetectorFIFO()
{
    // Close FIFOs
    fifo_close(PRE_FAILURE_FIFO);
    fifo_close(SIGNAL_FIFO);
    // Deallocate FIFO buffers
    free(pre_fifo_buf);
    free(post_fifo_buf);
    free(signal_buf);
}

void XFDetectorDetector::check_pm_status()
{
    // TODO:
}

void XFDetectorDetector::locate_bug(trace_entry_t* bug_trace, string executable)
{
    assert(bug_trace && bug_trace->operation != INVALID && "Invalid trace operation");
    
    string command = "addr2line -e " + executable + " " 
                        + std::to_string(bug_trace->instr_ptr);
    
    if (system(command.c_str()) < 0) {
        ERR("Locate bug fail");
    }
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
        cerr << "Read Addr: " << std::hex << cur_trace->src_addr << " size: " << size << " ";
        fprintf(stderr, "Read Instr_ptr: %p\n", (void*)instr_ptr);
        // cerr << "Found in added addr? " << is_added_addr(cur_trace, src_addr, size) << endl;
        // DEBUG(fprintf(stderr, "------Added address------\n"););
        // DEBUG(cerr << this->get_tx_added_addr(tid););
        // DEBUG(fprintf(stderr, "-------------------------\n"););
    }
    return isAddrFound;
}

void XFDetectorDetector::update_pm_status(int stage, ShadowPM* shadow_mem, trace_entry_t* cur_trace)
{
    pm_op_t operation = cur_trace->operation;
    bool func_ret = cur_trace->func_ret;
    int tid = cur_trace->tid;
    addr_t src_addr = cur_trace->src_addr;
    addr_t dst_addr = cur_trace->dst_addr;
    size_t size = cur_trace->size;
    addr_t instr_ptr = cur_trace->instr_ptr;
    int non_temporal = cur_trace->non_temporal;
    // print trace for debugging
    DEBUG(cout << "OP: " << pm_op_name[operation]
            << " TID: " << tid
            << " SRC_ADDR: " << (void*)src_addr
            << " DST_ADDR: " << (void*)dst_addr
            << " SIZE: " << size
            << " INSTR_PTR: " << (void*)instr_ptr << endl);
    // if(operation==WRITE){
    //     if (stage == POST_FAILURE)
    //         cerr << std::hex << "WRITE: " << dst_addr << " size: " << size << " WRITE IP: " << instr_ptr << endl;
    // }
    // Update shadow memory for each operation in trace

    // We need to check if the trace is from function call or return.
    // Only PMEM_MAP_FILE's return trace is useful.
    if (true==func_ret){
        switch(operation){
        case PMEM_MAP_FILE:
            //shadow_mem->add_pm_addr(cur_trace, dst_addr, size);
            if(stage==PRE_FAILURE){
                        shadow_mem->add_pm_addr(cur_trace, dst_addr, size);
                    }else{
                        shadow_mem->add_pm_addr_post(cur_trace, dst_addr, size);
                    }
            break;
        
        }
    }else{
        // TODO: We also need to handle update inside the transaction.
        //       We ignore the update to the TX_ADD-ed addresses until the
        //       transaction commits. After that, those addresses
        //       becomes consistent.
        // TODO: need to handle atomic region with multiple cases
        switch (cur_trace->operation) {
            case TRACE_END:
                fprintf(stderr, "Failure point IP: %p\n", (void*)instr_ptr);
                if (stage == PRE_FAILURE) pre_failure_point_complete = COMPLETE;
                // else if (stage == POST_FAILURE) post_failure_point_complete = COMPLETE;
                break;
            case TRACE_BEGIN:
                // TODO: used as assertion on trace start
                break;
            case TESTING_END:
                if (stage == PRE_FAILURE) pre_testing_complete = COMPLETE;
                else if (stage == POST_FAILURE) post_testing_complete = COMPLETE;
                break;
            case READ:
                // TODO: We also need to check if the read address is in PM address range.
                // TODO: check the consistency of the address we want to read in the shadow memory.
                if(stage == POST_FAILURE){
                    // TODO: check for commit variable
                    // Suppress external writer
                    bool isWriteOriginFound = true;
                    /*
                    for(auto &it : MAP_LOOKUP(shadow_mem->write_addr_IP_mapping, src_addr, size)){
                       if(addr2ip.find(it.second)!=addr2ip.end()){
                           isWriteOriginFound = true;
                       }
                    }
                    */
                    if(shadow_mem->is_detection_disabled(tid)==0){
                        if(isWriteOriginFound){
                            if(shadow_mem->lookup_checked_addr(src_addr, size)){
                                //fprintf(stderr, "Bypassing checks\n");
                                // Do nothing
                                //cerr << "Read Addr: " << std::hex << cur_trace->src_addr << " size: " << size << " ";
                                        //fprintf(stderr, "Consistent Read\n");
                            }else{
                                bool isCorrect = false;
                                // PMCorrectCase correct_case = 0;
                                if(shadow_mem->is_consistent(cur_trace, src_addr, size)){
                                    isCorrect = true;
                                    // correct_case = CONSISTENT;
                                }
                                if((shadow_mem->is_writtenback(cur_trace, src_addr, size)) && 
                                        (shadow_mem->is_recent_commit_update(cur_trace, src_addr, size))){
                                    // cerr << "Recent commit: " << std::hex << src_addr << std::dec << "\n";
                                    isCorrect = true;
                                    // correct_case = PERSISTED_AND_COMMITTED;
                                }
                                if(shadow_mem->is_commit_var_addr(cur_trace, src_addr, size)){
                                    isCorrect = true;
                                    //cerr << "Reading commit var" << endl;
                                    // correct_case = COMMIT_VAR;
                                }
                                // FIXME: ignore tx_added locations for now
                                if (shadow_mem->is_added_addr(cur_trace, src_addr, size)) {
                                    isCorrect = true;
                                }

                                if(!isCorrect){
                                    bool addrFound = shadow_mem->printInconsistentReadDebug(cur_trace);
                                    // Skip writes from internal functions
                                    if (addrFound) {
                                        if (!shadow_mem->is_writtenback(cur_trace, src_addr, size)) {
                                            cerr << "Not persisted before failure" << endl;
                                        }
                                        if (!shadow_mem->is_recent_commit_update(cur_trace, src_addr, size)) {
                                            cerr << "Not persisted before commit var" << endl;
                                            //assert(shadow_mem->commit_var_set_addr.size()==0 && "No commit variable registered");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            case WRITE:
                if(stage==PRE_FAILURE && shadow_mem->is_commit_var_addr(cur_trace, dst_addr, size)){
                    // Update commit timestamp
                    // cerr << "updating TS" << endl;
                    shadow_mem->update_commitVar_timestamp();
                    shadow_mem->add_write_addr_IP_mapping(cur_trace);
                }
                // cerr << "Write addr=" << std::hex << dst_addr << endl;
                if((stage==POST_FAILURE)||(shadow_mem->is_detection_disabled(tid)==1)){
                    //cerr << "Tracking disabled" << endl;
                    shadow_mem->modify_addr(cur_trace, dst_addr, size);
                    // shadow_mem->writeback_addr(cur_trace, dst_addr, size);
                    // shadow_mem->drain_writeback(cur_trace);
                    shadow_mem->set_consistent_addr(cur_trace, dst_addr, size);
                    shadow_mem->add_write_addr_IP_mapping(cur_trace);
                    //cerr << std::hex << "WRITE: " << dst_addr << endl;
                }else{
                    // if(shadow_mem->is_commit_var_addr(cur_trace, dst_addr, size)){
                    //     // Update commit timestamp
                    //     // cerr << "updating TS" << endl;
                    //     shadow_mem->update_commitVar_timestamp();
                    // }
                    if(shadow_mem->is_in_pre_internal_funct(tid)){
                        shadow_mem->modify_addr(cur_trace, dst_addr, size);
                        shadow_mem->set_consistent_addr(cur_trace, dst_addr, size);
                        //cerr << "Write in PMDK" << endl;
                    }else{
                        // We need to update shadow mem in both cases because the recovery program 
                        // should not read from the original location if the Tx has not been commited.
                        // The pending writes will be handled when TX commits.
                        // fprintf(stderr, "Write: %p addr: %p\n", (void *)cur_trace->instr_ptr, dst_addr);
                        if(shadow_mem->is_in_tx(tid)){
                            // fprintf(stderr, "Write TX addr: %p\n", dst_addr);
                            if(!shadow_mem->is_added_addr(cur_trace, dst_addr, size)){
                                shadow_mem->add_non_tx_add_addr(cur_trace, dst_addr, size);
                            }
                        }
                        shadow_mem->modify_addr(cur_trace, dst_addr, size);
                        if(non_temporal){
                            shadow_mem->writeback_addr(cur_trace, dst_addr, size);
                        }
                        shadow_mem->add_write_addr_IP_mapping(cur_trace);
                        //cerr << "WRITE: " << dst_addr << endl;
                    }

                }
                break;
            case CLWB:
                if (stage == PRE_FAILURE) {
                    shadow_mem->writeback_addr(cur_trace, src_addr, size);
                }
                break;
            case SFENCE:
                shadow_mem->drain_writeback(cur_trace);
                if (stage == PRE_FAILURE) {
                    shadow_mem->increment_global_time();
                    // cerr << "Increment global time to " << shadow_mem->global_timestamp << endl;
                }
                break;
            case PMEM_MAP_FILE:
                // pmem_map_file() provides the address in return
                break;
            case PM_TRACE_PM_ADDR_ADD:
                // pm_trace_pm_addr_add() provides address and size as parameters.
                //shadow_mem->add_pm_addr(cur_trace, dst_addr, size);
                if(stage==PRE_FAILURE){
                    shadow_mem->add_pm_addr(cur_trace, dst_addr, size);
                }else{
                    shadow_mem->add_pm_addr_post(cur_trace, dst_addr, size);
                }
                break;
            case PMEM_UNMAP:
            case PM_TRACE_PM_ADDR_REMOVE:   // TODO, combine log entry types?
                shadow_mem->remove_pm_addr(cur_trace, src_addr, size);
                break;
            case PM_TRACE_TX_ADDR_ADD:
                // if(stage == POST_FAILURE){
                    // shadow_mem->set_consistent_addr(cur_trace, dst_addr, size);
                // }
                // if (stage == PRE_FAILURE) {
                shadow_mem->add_tx_add_addr(cur_trace, dst_addr, size, stage);
                // }
                break;
            case PM_TRACE_TX_BEGIN:
                shadow_mem->increment_tx_level(tid);
                break;
            case PM_TRACE_TX_END:
                shadow_mem->decrement_tx_level(tid);
                break;
            case PMDK_INTERNAL_CALL:
                shadow_mem->increment_pre_internal_funct_level(tid);
                break;
            case PMDK_INTERNAL_RET:
                shadow_mem->decrement_pre_internal_funct_level(tid);
                break;
            case _ADD_COMMIT_VAR:
                // cerr << std::hex << "Commit addr=" << src_addr << endl;
                if (stage==PRE_FAILURE) {
                    shadow_mem->add_commit_var_addr(cur_trace, src_addr, size);
                }
                break;
            case PM_TRACE_WRITE_IP:
                addr2ip.insert(instr_ptr);
                break;
            case PM_TRACE_DETECTION_SKIP_BEGIN:
                assert((shadow_mem->is_detection_disabled(tid)==0)&&"Trace skip already enabled.\n");
                shadow_mem->disable_detection(tid);
                break;

            case PM_TRACE_DETECTION_SKIP_END:
                assert((shadow_mem->is_detection_disabled(tid)==1)&&"Trace skip is not enabled.\n");
                shadow_mem->enable_detection(tid);
                break;


            case INVALID:
                cout << "Invalid ops" << endl;
                break;
            default:
                ERR("Unknown operation in trace");
                break;
        }
    
    }
}

void XFDetectorDetector::print_pm_trace(int stage, trace_entry_t* cur_trace)
{
    if (!cur_trace->func_ret) {
        DEBUG(cout << "OP: " << pm_op_name[cur_trace->operation]);
    } else {
        DEBUG(cout << "OP: " << pm_op_name[cur_trace->operation] << " (Return)");
    } 
    DEBUG(cout << " TID: " << cur_trace->tid
        << " SRC_ADDR: " << (void*)cur_trace->src_addr
        << " DST_ADDR: " << (void*)cur_trace->dst_addr
        << " SIZE: " << cur_trace->size
        << " INSTR_PTR: " << (void*)cur_trace->instr_ptr << endl);

    if (stage == PRE_FAILURE) {
        if (cur_trace->operation == TRACE_END) {
            pre_failure_point_complete = COMPLETE;
        } else if(cur_trace->operation == TESTING_END) {
            pre_testing_complete = COMPLETE;
        }
    } else if (stage == POST_FAILURE) {
        assert(cur_trace->operation != TRACE_END);
        // if (cur_trace->operation == TRACE_END) {
            // post_failure_point_complete = COMPLETE;
        if(cur_trace->operation == TESTING_END) {
            post_testing_complete = COMPLETE;
        }    
    }
}

ShadowPM shadow_mem;
XFDetectorDetector race_detector;
ExeCtrl execution_controller;
XFDetectorFIFO fifo;

void print_all_bugs()
{
    // Print out warnings
    for (auto &it: warn_vec) {
        cout << WARN_PRINT << it.description << ":" << endl;
        race_detector.locate_bug(&it.op, execution_controller.get_executable_path());
    }
    // Print out errors
    for (auto &it: warn_vec) {
        cout << ERROR_PRINT << it.description << ":" << endl;
        race_detector.locate_bug(&it.op, execution_controller.get_executable_path());
    }
}

void* spwan_post_failure_process(void* a)
{
    execution_controller.execute_post_failure();
}

int main(int argc, char* argv[])
{
    // Use the default config name unless specified
    if (argc > 1) execution_controller.init(argv[1]);
    else execution_controller.init(NULL);
    // Set testing_complete flag as incomplete
    race_detector.pre_testing_complete = INCOMPLETE;

    struct timeval total_start;
    struct timeval total_end;
    gettimeofday(&total_start, NULL);

    // For each failure point in the RoI
    while (race_detector.pre_testing_complete != COMPLETE) {
        cerr << "--------Switching to Pre failure--------" << endl;

        // Reset failure_point_complete flag to incomplete
        race_detector.pre_failure_point_complete = INCOMPLETE;
        // For each operation before failure point
        while (race_detector.pre_failure_point_complete != COMPLETE && 
                race_detector.pre_testing_complete != COMPLETE) {
            int read_size = fifo.pre_fifo_read();
            // Iterate through operations in FIFO buffer
            for (int i = 0; i < read_size / sizeof(trace_entry_t); ++i) {
                trace_entry_t* cur_trace = fifo.get_trace(PRE_FAILURE, i);
                race_detector.update_pm_status(PRE_FAILURE, &shadow_mem, cur_trace);
                
                //race_detector.print_pm_trace(PRE_FAILURE, cur_trace);
                // if (!cur_trace->func_ret) {
                //     cerr << "OP: " << pm_op_name[cur_trace->operation];
                // } else {
                //     cerr << "OP: " << pm_op_name[cur_trace->operation] << " (Return)";
                // } 
                // cerr << " TID: " << cur_trace->tid
                // << " SRC_ADDR: " << (void*)cur_trace->src_addr
                // << " DST_ADDR: " << (void*)cur_trace->dst_addr
                // << " SIZE: " << cur_trace->size
                // << " INSTR_PTR: " << (void*)cur_trace->instr_ptr << endl;
            }
            // Clear pre-failure FIFO buffer
            fifo.clear_pre_fifo_buf();
        }
        // TODO: Copy shadow memory.
        ShadowPM post_shadow_mem(shadow_mem);
        // Execute post-failure program
        struct timeval post_start;
        struct timeval post_end;
        gettimeofday(&post_start, NULL);
        pthread_t post_failure_thread;
        pthread_create(&post_failure_thread, NULL, spwan_post_failure_process, NULL);
        cerr << "--------Switching to post failure--------" << endl;
        // For timeout
        // time_t start, end;
        // time(&start);

        fifo.fifo_open(POST_FAILURE_FIFO);
        while (race_detector.post_testing_complete != COMPLETE) {
            int read_size = fifo.post_fifo_read();
            //cout << "readsize: " << read_size << endl;
            for (int i = 0; i < read_size / sizeof(trace_entry_t); ++i) {
                // cout << "Trace read" << endl;
                trace_entry_t* cur_trace = fifo.get_trace(POST_FAILURE, i);
                //race_detector.print_pm_trace(POST_FAILURE, cur_trace);
                
                // TODO: update shadow PM
                race_detector.update_pm_status(POST_FAILURE, &post_shadow_mem, cur_trace);
            }
            fifo.clear_post_fifo_buf();
            // time(&end);
            // Kill post-failure process when timeout
            // if (end - start > POST_FAILURE_EXEC_TIMEOUT){
            //     pthread_kill(post_failure_thread, 9);
            //     cerr << "Timeout : killing post failure process" << endl;
            //     cout << "Timeout" << endl;
            // }
            // start=end;
        }
        gettimeofday(&post_end, NULL);
        long long post_time = ((post_end.tv_sec*1000000L)+post_end.tv_usec) - ((post_start.tv_sec*1000000L)+post_start.tv_usec);
        cout << "Post-failure time: " << post_time/1000 << "ms" << endl;
        // cout << "testing completed" << endl;
        fifo.fifo_close(POST_FAILURE_FIFO);
        // Reset complete flag
        race_detector.post_testing_complete = INCOMPLETE;
        // Resume next failure point
        fifo.pin_continue_send();
    }
    gettimeofday(&total_end, NULL);
    int64_t total_time = ((total_end.tv_sec*1000000L)+total_end.tv_usec) - ((total_start.tv_sec*1000000L)+total_start.tv_usec);
        cout << "Total-failure time: " << total_time/1000 << "ms" << endl;
    return 0;
}
