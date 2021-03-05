#include "xfdetector.hh"
#include <sys/time.h>

void XFDetectorFIFO::fifo_create(int exec_id)
{
    if (exec_id >= 0) {
        sprintf(pre_failure_fifo_str, "/tmp/%s.%d", PRE_FAILURE_FIFO, exec_id);
        sprintf(post_failure_fifo_str, "/tmp/%s.%d", POST_FAILURE_FIFO, exec_id);
        sprintf(signal_fifo_str, "/tmp/%s.%d", SIGNAL_FIFO, exec_id);
    } else {
        sprintf(pre_failure_fifo_str, "/tmp/%s", PRE_FAILURE_FIFO);
        sprintf(post_failure_fifo_str, "/tmp/%s", POST_FAILURE_FIFO);
        sprintf(signal_fifo_str, "/tmp/%s", SIGNAL_FIFO);
    }

    // remove old FIFOs, if exist
    remove(pre_failure_fifo_str);
    remove(post_failure_fifo_str);
    remove(signal_fifo_str);

    // Create pre-failure FIFO
    if (mkfifo(pre_failure_fifo_str, 0666) < 0) {
        ERR("Pre-failure FIFO create failed.");
    }
    // Creast post-failure FIFO
    if (mkfifo(post_failure_fifo_str, 0666) < 0) {
        ERR("Post-failure FIFO create failed.");
    }
    // Create Signal FIFO
    if (mkfifo(signal_fifo_str, 0666) < 0) {
        ERR("Signal FIFO create failed.");
    }
}

void XFDetectorFIFO::fifo_open(const char* name)
{
    if (!strcmp(name, PRE_FAILURE_FIFO)) {
        pre_fifo_fd = open(pre_failure_fifo_str, O_RDONLY);
        if (pre_fifo_fd < 0) ERR("Pre-failure FIFO open failed.");
    } else if (!strcmp(name, POST_FAILURE_FIFO)) {
        post_fifo_fd = open(post_failure_fifo_str, O_RDONLY);
        if (post_fifo_fd < 0) ERR("Post-failure FIFO open failed.");
    } else if (!strcmp(name, SIGNAL_FIFO)) {
        signal_fifo_fd = open(signal_fifo_str, O_RDWR);
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

XFDetectorFIFO::XFDetectorFIFO(int exec_id)
{
    // Initialize FIFOs
    fifo_create(exec_id);

    // Allocate FIFO buffers
    pre_fifo_buf = (trace_entry_t*) malloc(PIN_FIFO_BUF_SIZE);
    post_fifo_buf = (trace_entry_t*) malloc(PIN_FIFO_BUF_SIZE);
    signal_buf = (char*) malloc(MAX_SIGNAL_LEN);
}

XFDetectorFIFO::~XFDetectorFIFO()
{
    // Close FIFOs
    fifo_close(PRE_FAILURE_FIFO);
    fifo_close(POST_FAILURE_FIFO);
    // Deallocate FIFO buffers
    free(pre_fifo_buf);
    free(post_fifo_buf);
    free(signal_buf);
    // Remove fifo files
    remove(pre_failure_fifo_str);
    remove(post_failure_fifo_str);
    remove(signal_fifo_str);
}

void XFDetectorDetector::locate_bug(trace_entry_t* bug_trace, string executable)
{
    XFD_ASSERT(bug_trace && bug_trace->operation != INVALID && "Invalid trace operation");
    
    string command = "addr2line -e " + executable + " " 
                        + std::to_string(bug_trace->instr_ptr);
    
    if (system(command.c_str()) < 0) {
        ERR("Locate bug fail");
    }
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
                // shadow_mem->reset_internal_funct_level(tid);
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
                if(stage == POST_FAILURE){
                    // Suppress external writer
                    // bool isWriteOriginFound = true; // PMFuzz Obsolete
                    /*
                    for(auto &it : MAP_LOOKUP(shadow_mem->write_addr_IP_mapping, src_addr, size)){
                       if(addr2ip.find(it.second)!=addr2ip.end()){
                           isWriteOriginFound = true;
                       }
                    }
                    */
                    if(shadow_mem->is_detection_disabled(tid)==0){
                        // cerr << "@XFD Read: " << std::hex << cur_trace->src_addr << " " << size << endl;
                        // if(isWriteOriginFound){ // PMFuzz Obsolete
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
                                if (shadow_mem->is_added_addr(cur_trace, src_addr, size)) {
                                    isCorrect = true;
                                }

                                if(!isCorrect){
                                    bool addrFound = shadow_mem->printInconsistentReadDebug(cur_trace);
                                    // Skip writes from internal functions
                                    if (addrFound) {
                                        if (!shadow_mem->is_writtenback(cur_trace, src_addr, size)) {
                                            cerr << "Not persisted before failure" << endl;
                                        } else if (!shadow_mem->is_recent_commit_update(cur_trace, src_addr, size)) {
                                            cerr << "Not persisted before commit var" << endl;
                                            //XFD_ASSERT(shadow_mem->commit_var_set_addr.size()==0 && "No commit variable registered");
                                        }
                                        else {
                                            cerr << "Other" << endl;
                                        }
                                    }
                                }
                            }
                        // }
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
                    if(shadow_mem->is_in_pre_internal_funct(tid)){
                        shadow_mem->modify_addr(cur_trace, dst_addr, size);
                        shadow_mem->set_consistent_addr(cur_trace, dst_addr, size);
                        // cerr << "Write in PMDK" << endl;
                        // cerr << "@XFD INTERNAL WRITE: " << std::hex << dst_addr << " " << size << endl;
                    }else{
                        // We need to update shadow mem in both cases because the recovery program 
                        // should not read from the original location if the Tx has not been commited.
                        // The pending writes will be handled when TX commits.
                        // fprintf(stderr, "Write: %p addr: %p\n", (void *)cur_trace->instr_ptr, dst_addr);
                        if(shadow_mem->is_in_tx(tid)){
                            if(!shadow_mem->is_added_addr(cur_trace, dst_addr, size)){
                                // cerr << "@XFD WRITE TX Addr: " << std::hex << dst_addr << endl;
                                shadow_mem->add_non_tx_add_addr(cur_trace, dst_addr, size);

                                cerr << "\033[0;31mConsistency Bug:\033[0m\nModify before TX_ADD.\nWrite IP: " 
                                    << std::hex << cur_trace->instr_ptr << " Write Addr: " << dst_addr << endl;
                                shadow_mem->print_IP_linenumber_mapping(cur_trace->instr_ptr, PRE_FAILURE);
                            }
                        }
                        shadow_mem->modify_addr(cur_trace, dst_addr, size);
                        if(non_temporal){
                            shadow_mem->writeback_addr(cur_trace, dst_addr, size);
                        }
                        shadow_mem->add_write_addr_IP_mapping(cur_trace);
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
                shadow_mem->add_tx_add_addr(cur_trace, dst_addr, size, stage, false);
                // }
                break;
            case PM_TRACE_TX_ALLOC:
                // if(stage == POST_FAILURE){
                    // shadow_mem->set_consistent_addr(cur_trace, dst_addr, size);
                // }
                // if (stage == PRE_FAILURE) {
                shadow_mem->add_tx_add_addr(cur_trace, dst_addr, size, stage, true);
                // shadow_mem->add_tx_alloc_addr(cur_trace, dst_addr, size, stage);
                // }
                break;
            case PM_TRACE_TX_BEGIN:
                // if (stage == PRE_FAILURE)
                    // shadow_mem->reset_internal_funct_level(tid);
                    shadow_mem->increment_tx_level(tid, stage);
                break;
            case PM_TRACE_TX_END:
                // if (stage == PRE_FAILURE)
                    shadow_mem->decrement_tx_level(tid, stage);
                    // shadow_mem->reset_internal_funct_level(tid);
                break;
            case PMDK_INTERNAL_CALL:
                // if (stage == PRE_FAILURE)
                    shadow_mem->increment_pre_internal_funct_level(tid);
                break;
            case PMDK_INTERNAL_RET:
                // if (stage == PRE_FAILURE)
                    shadow_mem->decrement_pre_internal_funct_level(tid);
                break;
            case _ADD_COMMIT_VAR:
                // cerr << std::hex << "Commit addr=" << src_addr << endl;
                if (stage==PRE_FAILURE) {
                    shadow_mem->add_commit_var_addr(cur_trace, src_addr, size);
                }
                break;
            // case PM_TRACE_WRITE_IP:
            //     addr2ip.insert(instr_ptr);
            //     break;
            case PM_TRACE_DETECTION_SKIP_BEGIN:
                XFD_ASSERT((shadow_mem->is_detection_disabled(tid)==0)&&"Trace skip already enabled.\n");
                shadow_mem->disable_detection(tid);
                break;

            case PM_TRACE_DETECTION_SKIP_END:
                XFD_ASSERT((shadow_mem->is_detection_disabled(tid)==1)&&"Trace skip is not enabled.\n");
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
        cout << "OP: " << pm_op_name[cur_trace->operation];
    } else {
        cout << "OP: " << pm_op_name[cur_trace->operation] << " (Return)";
    } 
    cout << " TID: " << cur_trace->tid
        << " SRC_ADDR: " << (void*)cur_trace->src_addr
        << " DST_ADDR: " << (void*)cur_trace->dst_addr
        << " SIZE: " << cur_trace->size
        << " INSTR_PTR: " << (void*)cur_trace->instr_ptr << endl;

    if (stage == PRE_FAILURE) {
        if (cur_trace->operation == TRACE_END) {
            pre_failure_point_complete = COMPLETE;
        } else if(cur_trace->operation == TESTING_END) {
            pre_testing_complete = COMPLETE;
        }
    } else if (stage == POST_FAILURE) {
        XFD_ASSERT(cur_trace->operation != TRACE_END);
        if(cur_trace->operation == TESTING_END) {
            post_testing_complete = COMPLETE;
        }    
    }
}

ShadowPM shadow_mem;
XFDetectorDetector race_detector;
ExeCtrl execution_controller;
XFDetectorFIFO *fifo;

int main(int argc, char* argv[])
{
    std::vector<string> args(argv, argv+argc);

    // Use the default config name unless specified
    if (argc > 3) {
        execution_controller.init(atoi(argv[2]), args);
    } else if (argc > 2) {
        execution_controller.init(atoi(argv[2]), args);
    } else if (argc > 1) {
        execution_controller.init(-1, args);
    } else {
        execution_controller.init(-1, args);
    }
    
    fifo = new XFDetectorFIFO(atoi(argv[2]));

    // Set testing_complete flag as incomplete
    race_detector.pre_testing_complete = INCOMPLETE;

    // Execute pre-failure (with pintool)
    execution_controller.execute_pre_failure();

    fifo->fifo_open(PRE_FAILURE_FIFO);
    fifo->fifo_open(SIGNAL_FIFO);

    struct timeval total_start;
    struct timeval total_end;
    gettimeofday(&total_start, NULL);

    // For each failure point in the RoI
    while (race_detector.pre_testing_complete != COMPLETE) {
        cerr << "--------Switching to Pre failure--------" << endl;

        // Reset failure_point_complete flag to incomplete
        race_detector.pre_failure_point_complete = INCOMPLETE;

        // Pre-failure FIFO timeout
        struct timeval fifo_read_start;
        struct timeval fifo_read_end;
        gettimeofday(&fifo_read_start, NULL);

        // For each operation before failure point
        while (race_detector.pre_failure_point_complete != COMPLETE && 
                race_detector.pre_testing_complete != COMPLETE) {

            int read_size = fifo->pre_fifo_read();
            if (read_size != 0) {
                // Reset time if read_size not zero
                gettimeofday(&fifo_read_start, NULL);
            }
            // Check timeout
            gettimeofday(&fifo_read_end, NULL);
            if (PRE_FAILURE_FIFO_TIMEOUT > 0 && 
                    fifo_read_end.tv_sec - fifo_read_start.tv_sec > PRE_FAILURE_FIFO_TIMEOUT) {
                ERR("Pre-failure FIFO timeout");
            }

            // Iterate through operations in FIFO buffer
            for (unsigned i = 0; i < read_size / sizeof(trace_entry_t); ++i) {
                trace_entry_t* cur_trace = fifo->get_trace(PRE_FAILURE, i);
                race_detector.update_pm_status(PRE_FAILURE, &shadow_mem, cur_trace);
                // race_detector.print_pm_trace(PRE_FAILURE, cur_trace);
            }
            // Clear pre-failure FIFO buffer
            fifo->clear_pre_fifo_buf();
        }
        ShadowPM post_shadow_mem(shadow_mem);
        // Execute post-failure program
        struct timeval post_start;
        struct timeval post_end;
        gettimeofday(&post_start, NULL);
        string image_copy_name = execution_controller.execute_post_failure();

        cerr << "--------Switching to post failure--------" << endl;
        
        bool timeout = false;
        fifo->fifo_open(POST_FAILURE_FIFO);
        while (race_detector.post_testing_complete != COMPLETE) {
            int read_size = fifo->post_fifo_read();
            for (unsigned i = 0; i < read_size / sizeof(trace_entry_t); ++i) {
                trace_entry_t* cur_trace = fifo->get_trace(POST_FAILURE, i);

                race_detector.update_pm_status(POST_FAILURE, &post_shadow_mem, cur_trace);
            }
            fifo->clear_post_fifo_buf();
            gettimeofday(&post_end, NULL);
            // Kill post-failure process when timeout
            // Timeout disabled if threshold < 0
            if (POST_FAILURE_EXEC_TIMEOUT > 0 
                    && post_end.tv_sec - post_start.tv_sec > POST_FAILURE_EXEC_TIMEOUT) {
                execution_controller.term_post_failure();
                timeout = true;
                cerr << "Timeout: killing post failure pid " << post_failure_pid << endl;
                break;
            }
        }
        gettimeofday(&post_end, NULL);
        long long post_time = ((post_end.tv_sec*1000000L)+post_end.tv_usec) 
                                - ((post_start.tv_sec*1000000L)+post_start.tv_usec);
        cout << "Post-failure time: " << post_time/1000 << "ms" << endl;
        // Remove copied image
        remove(image_copy_name.c_str());
        // Close post-failure FIFO
        fifo->fifo_close(POST_FAILURE_FIFO);
        // Reset complete flag
        race_detector.post_testing_complete = INCOMPLETE;
        // Resume next failure point
        fifo->pin_continue_send();

        // Check the return status of post-failure process
        if (execution_controller.post_failure_status() < 0 && !timeout) {
            cerr << "Kill pre failure due to post-failure error" << endl;
            execution_controller.term_pre_failure();
            return 1;
        }
    }
    gettimeofday(&total_end, NULL);
    int64_t total_time = ((total_end.tv_sec*1000000L)+total_end.tv_usec) 
                            - ((total_start.tv_sec*1000000L)+total_start.tv_usec);
    cout << "Total time: " << total_time/1000 << "ms" << endl;

    // clean up
    delete fifo;
    remove((string("/tmp/backtrace_pre.") + std::to_string(exec_id)).c_str());
    remove((string("/tmp/backtrace_post.") + std::to_string(exec_id)).c_str());

    return 0;
}
