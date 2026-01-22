
#include <stdlib.h>
#include "../src/defs.h"

void testCrypto(void);
void testCollectionsList(void);

int main(const int argc, const char* const* const argv) {
    assert(argc == 2);

    switch (atoi(argv[1])) {
        case 0: testCrypto(); break;
        case 1: testCollectionsList(); break;
        case 2: break;
        case 3: break;
        default: assert(false);
    }

    checkUnfreedAllocations();
    return 0;
}
