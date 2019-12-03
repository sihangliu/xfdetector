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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <memory.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>

#include "jitprofiling.h"

#include <iostream>
using namespace std;

extern "C" int foo(int n, int method_id); // This function body is in an assembly file

typedef int (*FOO_FUNC)(int, int);
typedef int (*PinNotifyEventFunc)(iJIT_JVM_EVENT event_type, void* EventSpecificData);

unsigned int iJIT_GetNewMethodID(void)
{
    static unsigned int id = 0;
    return ++id;
}

size_t get_page_size()
{
    return sysconf(_SC_PAGE_SIZE);
}

size_t get_size_in_full_pages(const size_t code_size)
{
    const size_t page_size = get_page_size();
    if ((code_size % page_size) == 0) 
    {
        return code_size; // already aligned
    }
    else
    {
        return page_size* ((code_size/page_size) + 1);
    }
}

void create_function_duplicate_and_notify(PinNotifyEventFunc f_notify_event, void* src_func_addr, size_t src_func_size, void* dst_func_address, const char* dst_func_name, unsigned int& method_id)
{
    memcpy(dst_func_address, src_func_addr, src_func_size);
    
    // Notify method load
    iJIT_Method_Load jit_func_load_event   = { 0 };
    method_id = iJIT_GetNewMethodID();
    jit_func_load_event.method_id           = method_id;
    jit_func_load_event.method_name         = strdup(dst_func_name);
    jit_func_load_event.method_load_address = dst_func_address  ;
    jit_func_load_event.method_size         = src_func_size   + 1;
    jit_func_load_event.line_number_size    = 0;
     (*f_notify_event)(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, (void*)&jit_func_load_event  );
    free(jit_func_load_event.method_name);
}

int main(int argc, char *argv[])
{
    //
    // parse argv
    //
    if (argc != 2)
    {
        printf("Error: Expecting 1 argument - foo size \n");
        return 1;
    }

    // The only argument is the size of foo()
    const long int foo_size = strtol(argv[1], NULL, 10);
    if ((foo_size == 0) || (foo_size == LONG_MAX) || (foo_size == (LONG_MIN)))
    {
        printf("Error: Failed to convert parameter \"%s\" to a number \n", argv[1]);
        return 1;
    }

    //
    // Load libpinjitprofiling.so and find NotifyEvent function address
    //
    void* handle = NULL;
    PinNotifyEventFunc iJIT_NotifyEvent;

    handle = dlopen("libpinjitprofiling.so", RTLD_LAZY);
    if (handle) {
        dlerror();
         *(void **)(&iJIT_NotifyEvent) = dlsym(handle, "NotifyEvent");
        if (iJIT_NotifyEvent == NULL)
        {
            printf("Error: dlsym failed with error %s\n", dlerror());
            return 1;
        }
     }
    else
    {
        printf("Error: dlopen failed with error %s\n", dlerror());
        return 1;
    }

    //
    // Create duplicates of foo()
    //
    const size_t foo_size_in_pages = get_size_in_full_pages((size_t)foo_size);
    const size_t allocated_size = (foo_size_in_pages * 3) + get_page_size();
    void* allocated_addr = mmap( NULL,                               /*addr*/
                                 allocated_size,                    /*length*/
                                 PROT_READ | PROT_WRITE | PROT_EXEC, /*prot*/
                                 MAP_PRIVATE | MAP_ANONYMOUS,        /*flags*/
                                 -1                                  /*fd*/, 
                                 0                                   /*offset*/
                                 );

    if (allocated_addr == MAP_FAILED)
    {
        printf("Error: mmap failed with errno %d\n", errno);
        return 1;
    }

    // Create 3 duplicated:
    // 1 - at the base address of the allocated buffer
    // 2 - at the base address of the adjacent page to duplicate 1
    // 3 - leave a blank page between duplicate 2 and duplicate 3
    unsigned int method_id_1, method_id_2, method_id_3;
    void* addr_1 = allocated_addr;
    void* addr_2 = (void*)((uint8_t*)allocated_addr + foo_size_in_pages);
    void* addr_3 = (void*)((uint8_t*)allocated_addr + (2*foo_size_in_pages) + get_page_size());
    create_function_duplicate_and_notify(iJIT_NotifyEvent, (void*)foo, (size_t)foo_size, addr_1, "jit_foo_1", method_id_1);
    create_function_duplicate_and_notify(iJIT_NotifyEvent, (void*)foo, (size_t)foo_size, addr_2, "jit_foo_2", method_id_2);
    create_function_duplicate_and_notify(iJIT_NotifyEvent, (void*)foo, (size_t)foo_size, addr_3, "jit_foo_3", method_id_3);
    const FOO_FUNC jit_foo_1 = (FOO_FUNC)addr_1;
    const FOO_FUNC jit_foo_2 = (FOO_FUNC)addr_2;
    const FOO_FUNC jit_foo_3 = (FOO_FUNC)addr_3;

    // Execute jit functions
    const int i = 8;
    assert((jit_foo_1)(i, (int)method_id_1) == (i*i)); // just to make sure the functionality is ok
    assert((jit_foo_2)(i, (int)method_id_2) == (i*i));
    assert((jit_foo_3)(i, (int)method_id_3) == (i*i));

    // Notify shutdown
    (*iJIT_NotifyEvent)(iJVM_EVENT_TYPE_SHUTDOWN, NULL);

    // Shutdown:
    // Unmap memory
    // Unoad libpinjitprofiling.so

    if (munmap(allocated_addr, allocated_size) == -1)
    {
        printf("Error: munmap failed with errno %d\n", errno);
        return 1;
    }

    if (handle)
    {
        if (dlclose(handle) != 0)
        {
            printf("Error: dlclose failed with error %s\n", dlerror());
            return 1;
        }
     }

    return 0;
}

