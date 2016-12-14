// program to cause a breakpoint trap

#include <inc/lib.h>

    void
umain(int argc, char **argv)
{
    asm volatile("int $3");
    asm volatile("movl $0, %eax");
    asm volatile("addl $1, %eax");
    asm volatile("addl $1, %eax");
    asm volatile("addl $1, %eax");
    asm volatile("addl $1, %eax");
    asm volatile("addl $1, %eax");

}

