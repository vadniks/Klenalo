
#include <SDL2/SDL_stdinc.h>
#include "list.h"

static const int MAX_SIZE = 0x7fffffff;

struct _List {
    void** values;
    int size;
    ListDeallocator nullable deallocator;
};

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

void listPush(List* const list, void* const value) {
    listAddBack(list, value);
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
    for (int i = index; i < --list->size; list->values[i] = list->values[i + 1], i++);

    if (list->size)
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
