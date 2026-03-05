
#include <stdlib.h>
#include <stdio.h>
#include "../src/defs.h"

void testCrypto(void);
void testCollectionsList(void);
void testCollectionsDeque(void);
void testCollectionsHashtable(void);

int main(const int argc, const char* const* const argv) {
    assert(argc == 2);
    setbuf(stdout, nullptr); // NOLINT(*-unsafe-functions)

    switch (strtol(argv[1], nullptr, 10)) {
        case 0: testCrypto(); break;
        case 1: testCollectionsList(); break;
        case 2: testCollectionsDeque(); break;
        case 3: testCollectionsHashtable(); break;
        default: assert(false);
    }

    checkUnfreedAllocations();
    return 0;
}
