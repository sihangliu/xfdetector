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

#include <stdio.h>

#if defined (TARGET_WINDOWS)
#define EXPORT_SYM __declspec( dllexport )
#else
#define EXPORT_SYM extern
#endif

EXPORT_SYM void first ( unsigned long arg );
EXPORT_SYM void second( unsigned long arg );
EXPORT_SYM void third ( unsigned long arg );
EXPORT_SYM void fourth( unsigned long arg );


int main()
{
    first(0xc0decafe);

    return 0;
}


EXPORT_SYM void first( unsigned long arg )
{
    printf( "first: 0x%lx\n", arg);
    
    second( 0xdeadc0de );
}


EXPORT_SYM void second( unsigned long arg )
{
    printf( "second: 0x%lx\n", arg );
    
    third( 0xc0ffee );
}


EXPORT_SYM void third( unsigned long arg )
{
    printf( "third: 0x%lx\n", arg );
    
    fourth( 0xbeeffeed );
}


EXPORT_SYM void fourth( unsigned long arg )
{
    printf( "fourth: 0x%lx\n", arg );
}



