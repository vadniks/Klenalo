
#include "lifecycle.h"

#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>
#include <SDL3/SDL.h>

[[gnu::noinline]]
static bool xSDL_Init(SDL_InitFlags flags) {
    debug("aaa")
    return false;
}

static void patch_function(void* target_function, void* new_function) {
    unsigned long page_size = sysconf(_SC_PAGESIZE);
    void* page_start = (void*) ((unsigned long) target_function & ~(page_size - 1));

    // Change memory permissions to RWX
    assert(!mprotect(page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC));
    debugArgs("%p %p", new_function, target_function)
    debug("a")
    // Overwrite with JMP to our new function
    byte jmp_code[15] = { 0x48, 0xb8 }; // x86_64 JMP opcode
//    const long address = 0x5555556cd9c8ul; //(long) 0xffff8000085bcff0ul; //(long) new_function - (long) target_function - 5 - 0x800000000000; //(intptr_t)new_function - (intptr_t)target_function - 5; //(unsigned) ((unsigned long) xSDL_Init & ~((int) 0)); //(unsigned) ((unsigned long) new_function - (unsigned long) target_function);
//    debugArgs("%lx %ld %u %ld %x", address, address, (unsigned) address, address & 0xffffffff, (int) address)
    *(unsigned long*) &jmp_code[2] = (unsigned long) new_function;
    *(unsigned int*) &jmp_code[10] = 0xe0ff;
    printMemory(jmp_code, sizeof jmp_code, PRINT_MEMORY_MODE_HEX);

    memcpy(target_function, jmp_code, sizeof(jmp_code));

    // Restore memory protection
    mprotect(page_start, page_size, PROT_READ | PROT_EXEC);
}

int main(void) {
    debugArgs("%lx %lx %lx %lx", SDL_Init, xSDL_Init, (void*) SDL_Init - (void*) xSDL_Init, (void*) xSDL_Init - (void*) SDL_Init)
    debugArgs("%lx", max((unsigned long) SDL_Init, (unsigned long) xSDL_Init) - min((unsigned long) SDL_Init, (unsigned long) xSDL_Init))
    debugArgs("%lu %lu %lu %lu", SDL_Init, xSDL_Init, SDL_Init - xSDL_Init, xSDL_Init - SDL_Init)
    debugArgs("%lx %lx %lx %lx", main, xSDL_Init, (void*) main - (void*) xSDL_Init, (void*) xSDL_Init - (void*) main)
    debugArgs("%u %x", (unsigned) ((unsigned long) xSDL_Init - (unsigned long) main), (unsigned) ((unsigned long) xSDL_Init & ~((int) 0)))
//    assert(0);
    void* real_func = dlsym(RTLD_NEXT, "SDL_Init"); // Find real function
    printMemory(SDL_Init, 10, PRINT_MEMORY_MODE_HEX);
    debugArgs("%lx %p %p", swapBytes((long) SDL_Init), SDL_Init, real_func)
    if (real_func)
        patch_function(real_func, (void*) xSDL_Init);
    printMemory(SDL_Init, 10, PRINT_MEMORY_MODE_HEX);
    debug("b")

    debugArgs("%c", SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) ? 't' : 'f')
//    assert(0);
    return 0;

    const unsigned long allocations = xallocations();
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    assert(xallocations() == allocations);
    return 0;
}
