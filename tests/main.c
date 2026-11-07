
#include <stdlib.h>
#include <stdio.h>
#include "../src/defs.h"

int main(const int argc, const char* const* const argv) {
    assert(argc == 2);
    setbuf(stdout, nullptr); // so printf output can be yielded on test fails, otherwise ctest truncates ignores stdout

    switch (strtol(argv[1], nullptr, 10)) {

        default: assert(false);
    }

    return 0;
}
