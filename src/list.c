
#include "rwMutex.h"
#include "list.h"

struct _List {
    void** nullable values;
    int size;
    RWMutex* nullable rwMutex;
    const ListDeallocator nullable deallocator;
};

static const int MAX_SIZE = ~0u / 2u; // 0x7fffffff

List* listCreate(const bool synchronized, const ListDeallocator nullable deallocator) {
    List* const list = xmalloc(sizeof *list);
    assert(list);
    list->values = nullptr;
    list->size = 0;
    list->rwMutex = synchronized ? rwMutexCreate() : nullptr;
    unconst(list->deallocator) = deallocator;
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
