
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
    List* const new = listInit(old->deallocator);

    for (int i = 0; i < old->size; i++)
        listAddBack(new, itemDuplicator(old->values[i]));

    return new;
}

void listAddBack(List* const list, void* const value) {
    assert(list->size < MAX_SIZE);

    list->values = SDL_realloc(list->values, ++list->size * sizeof(void*));
    list->values[list->size - 1] = value;
}

void listAddFront(List* const list, void* const value) {
    assert(list->size < MAX_SIZE);

    void** const temp = SDL_malloc(++list->size * sizeof(void*));
    temp[0] = value;
    for (int i = 1; i < list->size; temp[i] = list->values[i - 1], i++);

    SDL_free(list->values);
    list->values = temp;
}

void* listGet(const List* const list, const int index) {
    assert(list->size > 0 && list->size < MAX_SIZE && index >= 0 && index < MAX_SIZE);

    assert(list->values);
    return list->values[index];
}

int listSize(const List* const list) {
    return list->size;
}

void* nullable listBinarySearch(const List* const list, const void* const key, const ListComparator comparator) {
    assert(list->size > 0 && list->values);

    const unsigned long index =
        (void**) SDL_bsearch(key, list->values, list->size, sizeof(void*), comparator) - list->values;

    return (int) index >= list->size ? nullptr : list->values[index];
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
