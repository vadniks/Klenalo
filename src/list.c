
#include <SDL2/SDL_stdinc.h>
#include "list.h"

struct _List {
    void* nullable* nullable values;
    int size;
    ListDeallocator nullable deallocator;
};

static const int MAX_SIZE = 0x7fffffff;

List* listInit(const ListDeallocator nullable deallocator) {
    List* const list = SDL_malloc(sizeof *list);
    assert(list);
    list->values = nullptr;
    list->size = 0;
    list->deallocator = deallocator;
    return list;
}

List* listCopy(const List* const old, const ListItemDuplicator itemDuplicator) {
    assert(old->size && old->values);
    List* const new = listInit(old->deallocator);

    assert(new->values = SDL_malloc(old->size * sizeof(void*)));
    new->size = old->size;
    for (int i = 0; i < old->size; new->values[i] = itemDuplicator(old->values[i]), i++);

    return new;
}

void listAddBack(List* const list, void* const value) {
    assert(list->size < MAX_SIZE);

    assert(list->values = SDL_realloc(list->values, ++list->size * sizeof(void*)));
    list->values[list->size - 1] = value;
}

void listAddFront(List* const list, void* const value) {
    assert(list->size < MAX_SIZE);

    void** const temp = SDL_malloc(++list->size * sizeof(void*));
    assert(temp);
    temp[0] = value;
    for (int i = 1; i < list->size; temp[i] = list->values[i - 1], i++);

    SDL_free(list->values);
    list->values = temp;
}

void* listGet(const List* const list, const int index) {
    assert(list->size && list->values && index >= 0 && index < list->size);
    return list->values[index];
}

void* listPopFirst(List* const list) {
    assert(list->size && list->values);

    void* const value = list->values[0];
    list->size--;

    if (!list->size) {
        SDL_free(list->values);
        list->values = nullptr;
        return value;
    }

    SDL_memmove(list->values, (void*) list->values + sizeof(void*), list->size * sizeof(void*));
    assert(list->values = SDL_realloc(list->values, list->size));

    return value;
}

void* listPopLast(List* const list) {
    assert(list->size && list->values);

    list->size--;
    void* const value = list->values[list->size];

    if (list->size)
        assert(list->values = SDL_realloc(list->values, list->size * sizeof(void*)));
    else {
        SDL_free(list->values);
        list->values = nullptr;
    }

    return value;
}

void listRemove(List* const list, const int index) {
    assert(list->size && list->values && index >= 0 && index < list->size && list->deallocator);

    list->deallocator(list->values[index]);
    for (int i = index; i < list->size; list->values[i] = list->values[i + 1], i++);

    if (--list->size)
        assert(list->values = SDL_realloc(list->values, list->size * sizeof(void*)));
    else {
        SDL_free(list->values);
        list->values = nullptr;
    }
}

void* nullable listPeekFirst(const List* const list) {
    assert(!list->size && !list->values || list->size && list->values);
    return list->size ? list->values[0] : nullptr;
}

void* nullable listPeekLast(const List* const list) {
    assert(!list->size && !list->values || list->size && list->values);
    return list->size ? list->values[list->size - 1] : nullptr;
}

int listSize(const List* const list) {
    return list->size;
}

void* nullable listBinarySearch(const List* const list, const void* const key, const ListComparator comparator) {
    assert(list->size && list->values);

    const unsigned long index =
        (void**) SDL_bsearch(key, list->values, list->size, sizeof(void*), comparator) - list->values;

    return (int) index >= list->size ? nullptr : list->values[index];
}

void listQSort(List* const list, const ListComparator comparator) {
    assert(list->size && list->values);
    SDL_qsort(list->values, list->size, sizeof(void*), comparator);
}

static void destroyValuesIfNotEmpty(List* const list) {
    if (!list->deallocator) return;
    assert(list->size && list->values || !list->size && !list->values);
    for (int i = 0; i < list->size; list->deallocator(list->values[i++]));
}

void listClear(List* const list) {
    destroyValuesIfNotEmpty(list);
    list->size = 0;

    SDL_free(list->values);
    list->values = nullptr;
}

void listDestroy(List* const list) {
    destroyValuesIfNotEmpty(list);
    SDL_free(list->values);
    SDL_free(list);
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
        List* const list = listInit(stubDeallocator);
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
        List* const list = listInit(nullptr);
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
        List* const list = listInit(deallocator);
        assert(list);

        for (int i = 0; i < 5; i ++)
            listAddBack(list, SDL_malloc(1));

        listDestroy(list);
    }

    {
        List* const list = listInit(nullptr);
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
        List* const list = listInit(nullptr);
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
        List* const list = listInit(deallocator);
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
