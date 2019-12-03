/*
 * Copyright 2002-2019 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "pin.H"
#include <iostream>
#include <fstream>
using std::cerr;
using std::string;
using std::endl;

/* ================================================================== */
// Global variables
/* ================================================================== */

std::ostream * out = &cerr;
static BOOL dynamic_rtn_found = FALSE;
static BOOL dynamic_rtn_executed = FALSE;
static BOOL dynamic_ins_found = FALSE;
static BOOL dynamic_ins_executed = FALSE;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool", "o", "", "specify file name for output");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
    cerr << "This tool instruments dynamic functions and queries them for an valid RTN, SEC and IMG." << endl;
    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

VOID RtnAnalysisFunc(CHAR * rtnName, ADDRINT rtnAddress, UINT64 methodId, UINT arg1)
{
    PIN_LockClient();
    
    // Find RTN by address and verify the RTN is valid
    const RTN rtn = RTN_FindByAddress(rtnAddress);
    if (! RTN_Valid(rtn))
    {
        *out << "Failed to get valid RTN from RTN_Address" << endl;
        PIN_ExitApplication(1); // does not return
    }
    
    // Verify RTN is Dynamic
    ASSERTX(RTN_IsDynamic(rtn));
    
    dynamic_rtn_executed = TRUE;
    
    // Verify valid RTN name and compare with name retrieved at instrumentation callback
    if (strcmp(RTN_Name(rtn).c_str(), rtnName) != 0)
    {
        *out << "Mismatch in RTN name: " << RTN_Name(rtn) << " / " << rtnName << endl;
        PIN_ExitApplication(1); // does not return
    }

    // Compare MethodId with ID retrieved at instrumentation callback,
    // and also compare it with function argument 1 (which is also the method id).
    UINT dyn_method_id = RTN_DynamicMethodId(rtn);
    if (dyn_method_id != arg1)
    {
        *out << "Unexpected methodId: expected " << std::dec << arg1 << " , got " << dyn_method_id << endl;
        PIN_ExitApplication(1); // does not return
    }
    if ((UINT64)dyn_method_id != methodId)
    {
        *out << "Mismatch in methodId: " << std::dec << dyn_method_id << " / " << methodId << endl;
        PIN_ExitApplication(1); // does not return
    }

    // Validate a valid SEC, and a dynamic type
    const SEC sec = RTN_Sec(rtn);
    if (! SEC_Valid(sec))
    {
        *out << "Invalid SEC for RTN " << rtnName << endl;
        PIN_ExitApplication(1);
    }
    ASSERTX(SEC_Type(sec) == SEC_TYPE_DYNAMIC);
    
    // Validate a valid IMG, and a dynamic type
    const IMG img = SEC_Img(sec);
    if (! IMG_Valid(img))
    {
        *out << "Invalid IMG for RTN " << rtnName << endl;
        PIN_ExitApplication(1);
    }
    ASSERTX(IMG_Type(img) == IMG_TYPE_DYNAMIC_CODE);
    
    
    *out << "Before " << rtnName << "(" << arg1 << ") ; methodId = " << dyn_method_id << 
        " ; SEC \"" << SEC_Name(sec) << "\" ; IMG \"" << IMG_Name(img) << "\"" << endl;
    
    PIN_UnlockClient();
}

VOID InsAnalysisFunc()
{
    dynamic_ins_executed = TRUE;
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

VOID Instruction(INS ins, VOID *v)
{
    const RTN rtn = INS_Rtn(ins);
    if (RTN_Valid(rtn) && RTN_IsDynamic(rtn))
    {
        dynamic_ins_found = TRUE;
        INS_InsertCall( ins,
                        IPOINT_BEFORE, (AFUNPTR)InsAnalysisFunc, 
                        IARG_END);
    }
}

// Pin calls this function every time a new rtn is executed
VOID Routine(RTN rtn, VOID *v)
{
    if (!RTN_IsDynamic(rtn))
    {
        return;
    }

    *out << "Just discovered " << RTN_Name(rtn) << endl;
    dynamic_rtn_found = TRUE;

    RTN_Open(rtn);

    // Print the code of the jitted function (for debugging purposes)
    *out << RTN_Name(rtn) << "():" << endl;
    for (INS ins = RTN_InsHead(rtn) ; INS_Valid(ins); ins = INS_Next(ins))
    {
        *out << "\t" << std::hex << INS_Address(ins) << " : " << INS_Disassemble(ins) << endl;
    }

    // Insert a call at the entry point of a routine to increment the call count
    RTN_InsertCall( rtn, 
                    IPOINT_BEFORE, (AFUNPTR)RtnAnalysisFunc,
                    IARG_ADDRINT, RTN_Name(rtn).c_str(),
                    IARG_ADDRINT, RTN_Address(rtn),
                    IARG_UINT64, (UINT64)RTN_DynamicMethodId(rtn),
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 1, /* Arg 1 is MethodId from the application */
                    IARG_END);

    RTN_Close(rtn);
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v)
{
    if ((!dynamic_rtn_found) || (!dynamic_rtn_executed) || 
         (!dynamic_ins_found) || (!dynamic_ins_executed))
    {
        *out << "Error: Missing dynamic routines or instructions: Dynamic routines found(" << 
                (dynamic_rtn_found?"YES":"NO") << "), executed(" << 
                (dynamic_rtn_executed?"YES":"NO") << ") ; Dynamic instructions found(" << 
                (dynamic_ins_found?"YES":"NO") << "), executed (" << 
                (dynamic_ins_executed?"YES":"NO") << ")" << std::endl;
        PIN_ExitProcess(1);
    }
    const string fileName = KnobOutputFile.Value();
    if (!fileName.empty())
    {
        delete out;
    }
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments,
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[])
{
    // Initialize symbol processing
    PIN_InitSymbols();

    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid
    if(PIN_Init(argc,argv))
    {
        return Usage();
    }

    const string fileName = KnobOutputFile.Value();

    if (!fileName.empty())
    {
        out = new std::ofstream(fileName.c_str());
    }

    // Register Routine to be called to instrument rtn
    RTN_AddInstrumentFunction(Routine, 0);

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, NULL);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, NULL);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
