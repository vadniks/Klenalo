
#include "../rwMutex.h"
#include "list.h"

struct _List {
    void** nullable values;
    int size;
    RWMutex* nullable const rwMutex;
    const Deallocator nullable deallocator;
};

static const int MAX_SIZE = ~0u / 2u; // 0x7fffffff

List* listCreate(const bool synchronized, const Deallocator nullable deallocator) {
    List* const list = xmalloc(sizeof *list);
    list->values = nullptr;
    list->size = 0;
    unconst(list->rwMutex) = synchronized ? rwMutexCreate() : nullptr;
    unconst(list->deallocator) = deallocator;
    return list;
}

static inline void deallocateValue(const List* const list, void* const value) {
    if (list->deallocator) list->deallocator(value);
}

static inline void xRwMutexCommand(List* const list, const RWMutexCommand command) {
    if (list->rwMutex) rwMutexCommand(list->rwMutex, command);
}

List* nullable listCopy(List* const old, const bool synchronized, const Duplicator nullable duplicator) {
    xRwMutexCommand(old, RW_MUTEX_COMMAND_READ_LOCK);

    if (!old->size) {
        xRwMutexCommand(old, RW_MUTEX_COMMAND_READ_UNLOCK);
        return nullptr;
    } else
        assert(old->values);

    List* const new = listCreate(synchronized, old->deallocator);

    new->values = xmalloc(old->size * sizeof(void*));
    new->size = old->size;
    for (int i = 0; i < old->size; new->values[i] = duplicator ? duplicator(old->values[i]) : old->values[i], i++);

    xRwMutexCommand(old, RW_MUTEX_COMMAND_READ_UNLOCK);
    return new;
}

void listAddBack(List* const list, void* const value) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(list->size < MAX_SIZE);

    list->values = xrealloc(list->values, ++list->size * sizeof(void*));
    list->values[list->size - 1] = value;

    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void listAddFront(List* const list, void* const value) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(list->size < MAX_SIZE);

    void** const temp = xmalloc(++list->size * sizeof(void*));
    temp[0] = value;
    for (int i = 1; i < list->size; temp[i] = list->values[i - 1], i++);

    xfree(list->values);
    list->values = temp;

    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void* nullable listGet(List* const list, const int index) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_READ_LOCK);
    assert(!!list->size == !!list->values);
    void* const value = index >= 0 && index < list->size && list->values ? list->values[index] : nullptr;
    xRwMutexCommand(list, RW_MUTEX_COMMAND_READ_UNLOCK);
    return value;
}

void* nullable listPopFirst(List* const list) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);

    if (!list->size || !list->values) {
        xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return nullptr;
    }

    void* const value = list->values[0];
    list->size--;

    if (!list->size) {
        xfree(list->values);
        list->values = nullptr;

        xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return value;
    }

    xmemmove(list->values, (void*) list->values + sizeof(void*), list->size * sizeof(void*));
    list->values = xrealloc(list->values, list->size * sizeof(void*));

    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
    return value;
}

void* nullable listPopLast(List* const list) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);

    if (!list->size || !list->values) {
        xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return nullptr;
    }

    list->size--;
    void* const value = list->values[list->size];

    if (list->size)
        list->values = xrealloc(list->values, list->size * sizeof(void*));
    else {
        xfree(list->values);
        list->values = nullptr;
    }

    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
    return value;
}

void listRemove(List* const list, const int index) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(list->size && list->values && index >= 0 && index < list->size);

    deallocateValue(list, list->values[index]);
    for (int i = index; i < list->size - 1; list->values[i] = list->values[i + 1], i++);

    if (--list->size)
        list->values = xrealloc(list->values, list->size * sizeof(void*));
    else {
        xfree(list->values);
        list->values = nullptr;
    }

    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void* nullable listPeekFirst(List* const list) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_READ_LOCK);

    assert(!list->size && !list->values || list->size && list->values);
    void* const value = list->size ? list->values[0] : nullptr;

    xRwMutexCommand(list, RW_MUTEX_COMMAND_READ_UNLOCK);
    return value;
}

void* nullable listPeekLast(List* const list) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_READ_LOCK);

    assert(!list->size && !list->values || list->size && list->values);
    void* const value = list->size ? list->values[list->size - 1] : nullptr;

    xRwMutexCommand(list, RW_MUTEX_COMMAND_READ_UNLOCK);
    return value;
}

int listSize(List* const list) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_READ_LOCK);
    assert(list->size && list->values || !list->size && !list->values);
    const int size = list->size;
    xRwMutexCommand(list, RW_MUTEX_COMMAND_READ_UNLOCK);
    return size;
}

typedef int (* SDL_CompareCallback)(const void* const, const void* const);

void* nullable listBinarySearch(List* const list, const void* const key, const Comparator comparator) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_READ_LOCK);
    assert(list->size && list->values);

    void* SDL_bsearch(
        const void* const, const void* const, const unsigned long, const unsigned long, const SDL_CompareCallback
    );

    const unsigned long index =
        (void**) SDL_bsearch(key, list->values, list->size, sizeof(void*), comparator) - list->values;
    void* const value = (int) index >= list->size ? nullptr : list->values[index];

    xRwMutexCommand(list, RW_MUTEX_COMMAND_READ_UNLOCK);
    return value;
}

void listQSort(List* const list, const Comparator comparator) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);

    void SDL_qsort(void* const, const unsigned long, const unsigned long, const SDL_CompareCallback);

    assert(list->size && list->values);
    SDL_qsort(list->values, list->size, sizeof(void*), comparator);

    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

static void destroyValuesIfNotEmpty(List* const list) {
    if (!list->deallocator) return;
    assert(list->size && list->values || !list->size && !list->values);
    for (int i = 0; i < list->size; list->deallocator(list->values[i++]));
}

void listClear(List* const list) {
    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_LOCK);

    destroyValuesIfNotEmpty(list);
    list->size = 0;

    xfree(list->values);
    list->values = nullptr;

    xRwMutexCommand(list, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void listDestroy(List* const list) {
    if (list->rwMutex) rwMutexDestroy(list->rwMutex);
    destroyValuesIfNotEmpty(list);
    xfree(list->values);
    xfree(list);
}
