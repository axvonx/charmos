#include <console/printf.h>
#include <types/types.h>

/* Only called from syscall.asm */
void syscall_handler(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                     uint64_t arg4, uint64_t arg5);
void syscall_handler(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                     uint64_t arg4, uint64_t arg5) {
    cc_ignore(arg2, arg3, arg4, arg5);
    switch (num) {
    case 1: printf("%s", (char *) arg1); break;
    default: printf("Unknown syscall: %lu\n", num); break;
    }
}
