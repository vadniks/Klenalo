
#include <stdlib.h>
#include "../src/defs.h"

void testCrypto(void);

int main(const int argc, const char* const* const argv) {
    printf("%d %s, %s\n", argc, argv[0], argv[1]);
    assert(argc == 2);

    switch (atoi(argv[1])) {
        case 0: testCrypto(); break;
        case 1: break;
        case 2: break;
        case 3: break;
        default: assert(false);
    }

    return 0;
}
