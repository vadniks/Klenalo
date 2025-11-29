
#include "treeMap.h"
#include "lifecycle.h"

int main(const int argc, const char* const* const argv, const char* const* const envp) {
    USED(argc), USED(argv), USED(envp);
//    lifecycleInit();
//    lifecycleLoop();
//    lifecycleQuit();

    TreeMap* const map = treeMapCreate(false, xfree);

    const int size = 10;
    const int array[size] = {1, 5, 0, 8, 9, 7, 3, 2, 4, 6};

    for (int i = 0; i < size; i++) {
        int* const value = xmalloc(sizeof(int));
        *value = array[i];
        treeMapInsert(map, array[i], value);
    }

    for (int i = 0; i < size; i++)
        assert(*(int*) treeMapSearchKey(map, i) == i);

    treeMapDelete(map, 9);

    TreeMapIterator* iterator;
    treeMapIterateBegin(map, iterator);
    int* value;
    while ((value = treeMapIterate(iterator)))
        printf("%d\n", *value);
    treeMapIterateEnd(iterator);

    treeMapDestroy(map);

    assert(xallocations() == 0);
    return 0;
}
