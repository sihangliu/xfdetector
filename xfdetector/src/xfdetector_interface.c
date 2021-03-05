#include <assert.h>

#include "xfdetector_interface.h"
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

/* PMFuzz header */
#ifdef PMFUZZ
#include "pmfuzz.h"
#endif

__thread int trace_status = NOT_TRACING;
__thread int xfdetector_failure_skip_status = NOT_SKIP;
__thread int has_completed = INCOMPLETE;

#ifdef PMRACE_STAT
__thread unsigned additional_failure_point_count = 0;
__thread unsigned atomic_op_count;
#endif

void _roi_pre_begin(void);
void _roi_pre_end(void);
void _roi_post_begin(void);
void _roi_post_end(void);
void _skip_failure_point_begin(void);
void _skip_failure_point_end(void);
void _add_failure_point(void);
void _testing_pre_complete(void);
void _testing_post_complete(void);
void _add_commit_var(const void*, unsigned);

/* Internal functions */
void _roi_pre_begin(void)
{
    assert(trace_status == NOT_TRACING 
        && "Error: XFDetector has already started tracing, canont start RoI again");
}

void _roi_pre_end(void)
{
    assert(trace_status == TRACING 
        && "Error: XFDetector has not started tracing, cannot end RoI");
}

void _roi_post_begin(void)
{
    assert(trace_status == NOT_TRACING 
        && "Error: XFDetector has already started tracing, canont start RoI again");
}

void _roi_post_end(void)
{
    assert(trace_status == TRACING 
        && "Error: XFDetector has not started tracing, cannot end RoI");
}

void _skip_failure_point_begin(void)
{
    xfdetector_failure_skip_status = SKIP;
}

void _skip_failure_point_end(void)
{
    xfdetector_failure_skip_status = NOT_SKIP;
}

void _add_failure_point(void)
{
#ifdef PMRACE_STAT
    additional_failure_point_count++;
#endif
}

void _testing_pre_complete(void)
{
    // has_completed = COMPLETE;
    // assert(0 && "Post-failure execution complete");
}

void _testing_post_complete(void)
{
    // has_completed = COMPLETE;
    // assert(0 && "Post-failure execution complete");
}

void _skipDetectionBegin(void)
{
    return;
}

void _skipDetectionEnd(void)
{
    return;
}

// void _atomic_update_begin(void)
// {
// #ifdef PMRACE_STAT
//     atomic_op_count++;
// #endif
// }

// void _atomic_update_end(void)
// {
//     // No need to do anything.
// }

void _add_commit_var(const void* variable, unsigned size)
{
    assert(variable && "Cannot register a NULL variable");
    assert(size <= 64U && "Cannot commit with a variable larger than one cache line");
    // No need to do anything
}

/* User interface functions */
void XFDetector_RoIBegin(int condition, int stage)
{
    assert(has_completed == INCOMPLETE && "Error: Testing has completed already");
    if (condition) {
        if (stage == PRE_FAILURE) {
            _roi_pre_begin();
        } else if (stage == POST_FAILURE) {
            _roi_post_begin();
        } else if (stage == (PRE_FAILURE | POST_FAILURE)) {
            _roi_pre_begin();
            _roi_post_begin();
        }else {
            assert(0 && "Invalid stage");
        }
    }
    trace_status = TRACING;
}

void XFDetector_RoIEnd(int condition, int stage)
{
    assert(has_completed == INCOMPLETE && "Error: Testing has completed already");
    if (condition) {
        if (stage == PRE_FAILURE) {
            _roi_pre_end();
        } else if (stage == POST_FAILURE) {
            _roi_post_end();
        } else if (stage == (PRE_FAILURE | POST_FAILURE)) {
            _roi_pre_end();
            _roi_post_end();
        } else {
            assert(0 && "Invalid stage");
        }
    }
    trace_status = NOT_TRACING;
}

void XFDetector_addFailurePoint(int condition)
{
    /* PMFuzz failure point annotation */
#ifdef PMFUZZ
    PMFUZZ_FAILURE_HINT
#endif

    assert(has_completed == INCOMPLETE && "Error: Testing has completed already");
    if (condition) {_add_failure_point();}
}

void XFDetector_skipFailureBegin(int condition)
{
    assert(has_completed == INCOMPLETE && "Error: Testing has completed already");
    if (condition) {_skip_failure_point_begin();}
}

void XFDetector_skipFailureEnd(int condition) 
{
    assert(has_completed == INCOMPLETE && "Error: Testing has completed already");
    if (condition) {_skip_failure_point_end();}
}

void XFDetector_complete(int condition, int stage) 
{
    assert(has_completed == INCOMPLETE && "Error: Testing has completed already");
    if (condition) {
        if (stage == PRE_FAILURE) {
            _testing_pre_complete();
        } else if (stage == POST_FAILURE) {
            _testing_post_complete();
        } else if (stage == (PRE_FAILURE | POST_FAILURE)) {
            _testing_pre_complete();
            _testing_post_complete();
        } else {
            assert(0 && "Invalid stage");
        }
        // Get shell variable
        int cur_stage = PRE_FAILURE;
        if (getenv("POST_FAILURE")){
            cur_stage = POST_FAILURE;
        }
        if (cur_stage & stage) {
            has_completed = COMPLETE;
            // kill(getpid(), 9);
            exit(0);
        }
    }
}

void XFDetector_addCommitVar(const void* variable, unsigned size)
{
    _add_commit_var(variable, size);
}


void skipDetectionBegin(int condition, int stage){
    if(!condition){
        return;
    }
    //assert(((stage & POST_FAILURE)==1) && "Skip detection found in post failure");
    int cur_stage = PRE_FAILURE;
    if (getenv("POST_FAILURE")){
        cur_stage = POST_FAILURE;
    }
    if(cur_stage & stage){
        _skipDetectionBegin();
    }
}

void skipDetectionEnd(int condition, int stage){
        if(!condition){
        return;
    }
    //assert(((stage & POST_FAILURE)==1) && "Skip detection found in post failure");
    int cur_stage = PRE_FAILURE;
    if (getenv("POST_FAILURE")){
        cur_stage = POST_FAILURE;
    }
    if(cur_stage & stage){
        _skipDetectionEnd();
    }
}




