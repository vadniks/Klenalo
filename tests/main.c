
#include "../src/defs.h"

int main() {
    setbuf(stdout, nullptr); // so printf output can be yielded on test fails, otherwise ctest truncates ignores stdout



    return 0;
}
