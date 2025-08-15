#include "allocator.h"
#include "mini_uart.h"

void *simple_alloc(size_t size) {
    // 8-byte align
    size = (size + 7) & ~7;
    extern char *__bss_end;   // heap starts from the end of bss section and grows upwards
    static char *heap_ptr = (char *) &__bss_end;
    uart_send_num((unsigned long) heap_ptr, "hex");
    uart_send_string("\n");

    if ((char *)(heap_ptr + size - (char *) &__bss_end) > (char *) MAX_HEAP_SIZE) {
        return nullptr;
    }

    heap_ptr += size;
    return (void *) (heap_ptr - size);
}