
#include "rwMutex.h"
#include "list.h"

struct _List {
    void** nullable values;
    int size;
    RWMutex* nullable rwMutex;
    const ListDeallocator nullable deallocator;
};

static const int MAX_SIZE = 0x7fffffff;

List* listCreate(const bool synchronized, const ListDeallocator nullable deallocator) {
    List* const list = xmalloc(sizeof *list);
    assert(list);
    list->values = nullptr;
    list->size = 0;
    list->rwMutex = synchronized ? rwMutexCreate() : nullptr;
    *(ListDeallocator*) &list->deallocator = deallocator;
    return list;
}

bool listSynchronized(const List* const list) {
    return (bool) list->rwMutex;
}

void listSetSynchronized(List* const list, const bool synchronized) {
    assert(synchronized != !!list->rwMutex);
    if (synchronized)
        list->rwMutex = rwMutexCreate();
    else {
        rwMutexDestroy(list->rwMutex);
        list->rwMutex = nullptr;
    }
}

static inline void deallocateValue(const List* const list, void* const value) {
    if (list->deallocator) list->deallocator(value);
}

static inline void listRwMutexCommand(List* const list, const RWMutexCommand command) {
    if (list->rwMutex) rwMutexCommand(list->rwMutex, command);
}

List* nullable listCopy(List* const old, const bool synchronized, const ListItemDuplicator nullable itemDuplicator) {
    listRwMutexCommand(old, RW_MUTEX_COMMAND_READ_LOCK);

    if (!old->size) {
        listRwMutexCommand(old, RW_MUTEX_COMMAND_READ_UNLOCK);
        return nullptr;
    } else
        assert(old->values);

    List* const new = listCreate(synchronized, old->deallocator);

    assert(new->values = xmalloc(old->size * sizeof(void*)));
    new->size = old->size;
    for (int i = 0; i < old->size; new->values[i] = itemDuplicator ? itemDuplicator(old->values[i]) : old->values[i], i++);

    listRwMutexCommand(old, RW_MUTEX_COMMAND_READ_UNLOCK);
    return new;
}

void listAddBack(List* const list, void* const value) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(list->size < MAX_SIZE);

    assert(list->values = xrealloc(list->values, ++list->size * sizeof(void*)));
    list->values[list->size - 1] = value;

    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void listAddFront(List* const list, void* const value) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(list->size < MAX_SIZE);

    void** const temp = xmalloc(++list->size * sizeof(void*));
    assert(temp);
    temp[0] = value;
    for (int i = 1; i < list->size; temp[i] = list->values[i - 1], i++);

    xfree(list->values);
    list->values = temp;

    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void* nullable listGet(List* const list, const int index) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_READ_LOCK);
    assert(!!list->size == !!list->values);
    void* const value = index >= 0 && index < list->size && list->values ? list->values[index] : nullptr;
    listRwMutexCommand(list, RW_MUTEX_COMMAND_READ_UNLOCK);
    return value;
}

void* nullable listPopFirst(List* const list) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);

    if (!list->size || !list->values) {
        listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return nullptr;
    }

    void* const value = list->values[0];
    list->size--;

    if (!list->size) {
        xfree(list->values);
        list->values = nullptr;

        listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return value;
    }

    xmemmove(list->values, (void*) list->values + sizeof(void*), list->size * sizeof(void*));
    assert(list->values = xrealloc(list->values, list->size * sizeof(void*)));

    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
    return value;
}

void* nullable listPopLast(List* const list) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);

    if (!list->size || !list->values) {
        listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return nullptr;
    }

    list->size--;
    void* const value = list->values[list->size];

    if (list->size)
        assert(list->values = xrealloc(list->values, list->size * sizeof(void*)));
    else {
        xfree(list->values);
        list->values = nullptr;
    }

    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
    return value;
}

void listRemove(List* const list, const int index) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(list->size && list->values && index >= 0 && index < list->size);

    deallocateValue(list, list->values[index]);
    for (int i = index; i < list->size; list->values[i] = list->values[i + 1], i++);

    if (--list->size)
        assert(list->values = xrealloc(list->values, list->size * sizeof(void*)));
    else {
        xfree(list->values);
        list->values = nullptr;
    }

    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void* nullable listPeekFirst(List* const list) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_READ_LOCK);

    assert(!list->size && !list->values || list->size && list->values);
    void* const value = list->size ? list->values[0] : nullptr;

    listRwMutexCommand(list, RW_MUTEX_COMMAND_READ_UNLOCK);
    return value;
}

void* nullable listPeekLast(List* const list) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_READ_LOCK);

    assert(!list->size && !list->values || list->size && list->values);
    void* const value = list->size ? list->values[list->size - 1] : nullptr;

    listRwMutexCommand(list, RW_MUTEX_COMMAND_READ_UNLOCK);
    return value;
}

int listSize(List* const list) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_READ_LOCK);
    assert(list->size && list->values || !list->size && !list->values);
    const int size = list->size;
    listRwMutexCommand(list, RW_MUTEX_COMMAND_READ_UNLOCK);
    return size;
}

void* nullable listBinarySearch(List* const list, const void* const key, const ListComparator comparator) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_READ_LOCK);
    assert(list->size && list->values);

    void* SDL_bsearch(
        const void* const, const void* const, const unsigned long, const unsigned long,
        int (* const)(const void* const, const void* const)
    );

    const unsigned long index =
        (void**) SDL_bsearch(key, list->values, list->size, sizeof(void*), comparator) - list->values;
    void* const value = (int) index >= list->size ? nullptr : list->values[index];

    listRwMutexCommand(list, RW_MUTEX_COMMAND_READ_UNLOCK);
    return value;
}

void listQSort(List* const list, const ListComparator comparator) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);

    void SDL_qsort(void* const, const unsigned long, const unsigned long, int (* const)(const void* const, const void* const));

    assert(list->size && list->values);
    SDL_qsort(list->values, list->size, sizeof(void*), comparator);

    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

static void destroyValuesIfNotEmpty(List* const list) {
    if (!list->deallocator) return;
    assert(list->size && list->values || !list->size && !list->values);
    for (int i = 0; i < list->size; list->deallocator(list->values[i++]));
}

void listClear(List* const list) {
    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);

    destroyValuesIfNotEmpty(list);
    list->size = 0;

    xfree(list->values);
    list->values = nullptr;

    listRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void listDestroy(List* const list) {
    if (list->rwMutex) rwMutexDestroy(list->rwMutex);
    destroyValuesIfNotEmpty(list);
    xfree(list->values);
    xfree(list);
}

#if TESTING
static void stubDeallocator(void* const) {}

static void deallocator(void* const value) {
    SDL_free(value);
}

static int comparator(const void* const a, const void* const b) {
    const void* const aa = *(const void* const*) a;
    const void* const bb = *(const void* const*) b;
    return (aa > bb) - (aa < bb);
}

static void* duplicator(const void* const value) {
    int* const copy = SDL_malloc(sizeof(int));
    *copy = *(int*) value;
    return copy;
}

void listRunTests(void) {
    const int allocations = SDL_GetNumAllocations();

    {
        List* const list = listCreate(stubDeallocator);
        assert(list);

        listAddBack(list, (void*) 1);
        listAddBack(list, (void*) 2);
        listAddBack(list, (void*) 3);
        listAddBack(list, (void*) 4);
        listAddBack(list, (void*) 5);

        assert(listGet(list, 0) == (void*) 1);
        assert(listGet(list, 1) == (void*) 2);
        assert(listGet(list, 2) == (void*) 3);
        assert(listGet(list, 3) == (void*) 4);
        assert(listGet(list, 4) == (void*) 5);

        assert(listPeekFirst(list) == (void*) 1);
        assert(listPeekLast(list) == (void*) 5);

        listRemove(list, 0);
        listRemove(list, 1);
        listRemove(list, 2);

        assert(listGet(list, 0) == (void*) 2);
        assert(listGet(list, 1) == (void*) 4);

        listDestroy(list);
    }

    {
        List* const list = listCreate(nullptr);
        assert(list);

        for (int i = 0; i < 5; i ++) {
            int* const j = SDL_malloc(sizeof(int));
            *j = i;
            listAddFront(list, j);
        }

        assert(*(int*) listGet(list, 0) == 4);
        assert(*(int*) listGet(list, 1) == 3);
        assert(*(int*) listGet(list, 2) == 2);
        assert(*(int*) listGet(list, 3) == 1);
        assert(*(int*) listGet(list, 4) == 0);

        for (int i = 0, j = 4; i < 5; i++, j--) {
            int* const k = listPopFirst(list);
            assert(*k == j);
            SDL_free(k);
        }

        listDestroy(list);
    }

    {
        List* const list = listCreate(deallocator);
        assert(list);

        for (int i = 0; i < 5; i ++)
            listAddBack(list, SDL_malloc(1));

        listDestroy(list);
    }

    {
        List* const list = listCreate(nullptr);
        assert(list);

        listAddBack(list, (void*) 1);
        listAddBack(list, (void*) 4);
        listAddBack(list, (void*) 2);
        listAddBack(list, (void*) 0);
        listAddBack(list, (void*) 3);

        listQSort(list, comparator);

        assert(listGet(list, 0) == (void*) 0);
        assert(listGet(list, 1) == (void*) 1);
        assert(listGet(list, 2) == (void*) 2);
        assert(listGet(list, 3) == (void*) 3);
        assert(listGet(list, 4) == (void*) 4);

        assert(listBinarySearch(list, (void*[1]) {(void*) 3}, comparator) == (void*) 3);

        listDestroy(list);
    }

    {
        List* const list = listCreate(nullptr);
        assert(list);

        listAddBack(list, (void*) 0);
        listAddBack(list, (void*) 1);
        listAddBack(list, (void*) 2);
        listAddBack(list, (void*) 3);
        listAddBack(list, (void*) 4);

        assert(listPopFirst(list) == (void*) 0);
        assert(listPopLast(list) == (void*) 4);

        assert(listGet(list, 0) == (void*) 1);
        assert(listGet(list, 1) == (void*) 2);
        assert(listGet(list, 2) == (void*) 3);

        assert(listSize(list) == 3);

        listDestroy(list);
    }

    {
        List* const list = listCreate(deallocator);
        assert(list);

        for (int i = 0; i < 5; i++) {
            int* const j = SDL_malloc(sizeof(int));
            *j = i;
            listAddBack(list, j);
        }

        List* const list2 = listCopy(list, duplicator);
        listDestroy(list);

        for (int i = 0; i < 5; i++)
            assert(*(int*) listGet(list2, i) == i);

        listClear(list2);
        assert(listSize(list2) == 0);

        listDestroy(list2);
    }

    assert(allocations == SDL_GetNumAllocations());
}
#endif
