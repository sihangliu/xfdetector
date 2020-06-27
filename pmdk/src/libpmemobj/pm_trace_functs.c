#include "pm_trace_functs.h"

// PM Address range manipulation
void __attribute__ ((noinline))
pm_trace_pm_addr_add(uint64_t addr, uint64_t size){
    return;
}

void pm_trace_pm_addr_remove(uint64_t addr, uint64_t size){
    return;
}

// Transaction annotation
void __attribute__ ((noinline))
pm_trace_tx_begin(void){
    return;
}

void __attribute__ ((noinline))
pm_trace_tx_end(void){
    return;
}

void __attribute__ ((noinline))
pm_trace_tx_addr_add(uint64_t addr, uint64_t size){
    return;
}

void __attribute__ ((noinline))
tx_commit_point(void){
    return;
}


// not necessary
/*
void pm_trace_pmdk_funct_begin(void){
    return;
}

void pm_trace_pmdk_funct_end(void){
    return;
}
*/
