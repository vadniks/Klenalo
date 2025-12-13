
#include "lvgl/src/stdlib/lv_mem.h"
#include "../defs.h"

export used void lv_mem_init(void) {}

export used void lv_mem_deinit(void) {}

export used void* lv_malloc_core(const unsigned long size) {
    return xmalloc(size);
}

export used void lv_free_core(void* const memory) {
    xfree(memory);
}

export used lv_result_t lv_mem_test_core(void) {
    return LV_RESULT_OK;
}

export used void* lv_realloc_core(void* const memory, size_t newSize) {
    return xrealloc(memory, newSize);
}

export used void lv_mem_monitor_core(lv_mem_monitor_t* const monitor) {
    xmemset(monitor, 0, sizeof *monitor);
}
