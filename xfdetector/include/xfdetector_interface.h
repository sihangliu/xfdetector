#ifndef PMRACE_INTERFACE_H
#define PMRACE_INTERFACE_H

#define PMRACE_STAT
#define TRACING 1
#define NOT_TRACING 0
#define SKIP 1
#define NOT_SKIP 0
#define COMPLETE 1
#define INCOMPLETE 0

#define PRE_FAILURE 1 
#define POST_FAILURE 2

/* User interface functions */

// Select Region of Interest (RoI) for pm-race detection
void XFDetector_RoIBegin(int condition, int stage);
void XFDetector_RoIEnd(int condition, int stage);

// Insert additional failure points
void XFDetector_addFailurePoint(int condition);

// Skip all the added and auto-generated failure points to speedup testing
void XFDetector_skipFailureBegin(int condition);
void XFDetector_skipFailureEnd(int condition);

// End of all testing
void XFDetector_complete(int condition, int stage);
// Updates within the atomic region are regarded either persist or not persist to PM
// XFDetector checks if the update exceeds the writeback granularity (one cache line)
// #define XFDetector_AtomicUpdate(code) __atomic_update_begin(); code; _atomic_update_end();

// Register a variable that commits updates
void XFDetector_addCommitVar(const void*, unsigned);

void skipDetectionBegin(int condition, int stage);
void skipDetectionEnd(int condition, int stage);

#endif // PMRACE_INTERFACE