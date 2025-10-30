
#include <lvgl/src/stdlib/lv_mem.h>
#include "defs.h"

external used void lv_mem_init(void) {}

external used void lv_mem_deinit(void) {}

external used void* nullable lv_malloc_core(const unsigned long size) {
    return xmalloc(size);
}

external used void lv_free_core(void* const memory) {
    xfree(memory);
}

external used lv_result_t lv_mem_test_core(void) {
    return LV_RESULT_OK;
}

external used void* lv_realloc_core(void* const memory, size_t newSize) {
    return xrealloc(memory, newSize);
}

external used void lv_mem_monitor_core(lv_mem_monitor_t* const monitor) {
    xmemset(monitor, 0, sizeof *monitor);
}
