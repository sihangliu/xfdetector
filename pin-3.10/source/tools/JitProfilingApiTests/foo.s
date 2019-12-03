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

#include <asm_macros.h>

.data
.text

# int foo(int i1, int i2);
# This function returns i1*i1

DECLARE_FUNCTION(foo)
NAME(foo):
    BEGIN_STACK_FRAME
    mov PARAM1,GAX_REG
    imul GAX_REG,GAX_REG
    END_STACK_FRAME
    ret
END_FUNCTION(foo)

