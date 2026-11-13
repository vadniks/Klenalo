
#include <stdlib.h>
#include "vector.h"

struct _Vector {
    void** nullable values;
    int size;
    const Deallocator nullable deallocator;
};

static constexpr int MAX_SIZE = ~0u / 2u; // 0x7fffffff

Vector* vectorCreate(const Deallocator nullable deallocator) {
    Vector* const vector = malloc(sizeof *vector);
    vector->values = nullptr;
    vector->size = 0;
    unconst(vector->deallocator) = deallocator;
    return vector;
}

static void deallocateValue(const Vector* const vector, void* const value) {
    if (vector->deallocator) vector->deallocator(value);
}

Vector* nullable vectorCopy(const Vector* const old, const Duplicator nullable duplicator) {
    if (!old->size) return nullptr;
    else assert(old->values);

    Vector* const new = vectorCreate(old->deallocator);

    new->values = malloc(old->size * sizeof(void*));
    new->size = old->size;
    for (int i = 0; i < old->size; new->values[i] = duplicator ? duplicator(old->values[i]) : old->values[i], i++);

    return new;
}

void vectorAddBack(Vector* const vector, void* const value) {
    assert(vector->size < MAX_SIZE);

    vector->values = realloc(vector->values, ++vector->size * sizeof(void*));
    vector->values[vector->size - 1] = value;
}

void vectorAddFront(Vector* const vector, void* const value) {
    assert(vector->size < MAX_SIZE);

    void** const temp = malloc(++vector->size * sizeof(void*));
    temp[0] = value;
    for (int i = 1; i < vector->size; temp[i] = vector->values[i - 1], i++);

    free(vector->values);
    vector->values = temp;
}

void* nullable vectorGet(const Vector* const vector, const int index) {
    assert(!!vector->size == !!vector->values);
    void* const value = index >= 0 && index < vector->size && vector->values ? vector->values[index] : nullptr;
    return value;
}

void vectorSwap(const Vector* const vector, const int index1, const int index2) {
    assert(vector->size && vector->values && index1 < vector->size && index2 < vector->size && index1 >= 0 && index2 >= 0);

    void* const value1 = vector->values[index1];
    void* const value2 = vector->values[index2];

    vector->values[index1] = value2;
    vector->values[index2] = value1;
}

void* nullable vectorPopFirst(Vector* const vector) {
    if (!vector->size || !vector->values) return nullptr;

    void* const value = vector->values[0];
    vector->size--;

    if (!vector->size) {
        free(vector->values);
        vector->values = nullptr;
        return value;
    }

    memmove(vector->values, (void*) vector->values + sizeof(void*), vector->size * sizeof(void*));
    vector->values = realloc(vector->values, vector->size * sizeof(void*));
    return value;
}

void* nullable vectorPopLast(Vector* const vector) {
    if (!vector->size || !vector->values) return nullptr;

    vector->size--;
    void* const value = vector->values[vector->size];

    if (vector->size)
        vector->values = realloc(vector->values, vector->size * sizeof(void*));
    else {
        free(vector->values);
        vector->values = nullptr;
    }

    return value;
}

void vectorRemove(Vector* const vector, const int index) {
    assert(vector->size && vector->values && index >= 0 && index < vector->size);

    deallocateValue(vector, vector->values[index]);
    for (int i = index; i < vector->size - 1; vector->values[i] = vector->values[i + 1], i++);

    if (--vector->size)
        vector->values = realloc(vector->values, vector->size * sizeof(void*));
    else {
        free(vector->values);
        vector->values = nullptr;
    }
}

void* nullable vectorPeekFirst(const Vector* const vector) {
    assert(!vector->size && !vector->values || vector->size && vector->values);
    void* const value = vector->size ? vector->values[0] : nullptr;
    return value;
}

void* nullable vectorPeekLast(const Vector* const vector) {
    assert(!vector->size && !vector->values || vector->size && vector->values);
    void* const value = vector->size ? vector->values[vector->size - 1] : nullptr;
    return value;
}

int vectorSize(const Vector* const vector) {
    assert(vector->size && vector->values || !vector->size && !vector->values);
    const int size = vector->size;
    return size;
}

void* nullable vectorBinarySearch(const Vector* const vector, const void* const key, const Comparator comparator) {
    assert(vector->size && vector->values);

    const ulong index =
        (void**) bsearch(key, vector->values, vector->size, sizeof(void*), (__compar_fn_t) comparator) - vector->values;
    void* const value = (int) index >= vector->size ? nullptr : vector->values[index];

    return value;
}

void vectorQSort(Vector* const vector, const Comparator comparator) {
    assert(vector->size && vector->values);
    qsort(vector->values, vector->size, sizeof(void*), (__compar_fn_t) comparator);
}

static void destroyValuesIfNotEmpty(Vector* const vector) {
    if (!vector->deallocator) return;
    assert(vector->size && vector->values || !vector->size && !vector->values);
    for (int i = 0; i < vector->size; vector->deallocator(vector->values[i++]));
}

void vectorClear(Vector* const vector) {
    destroyValuesIfNotEmpty(vector);
    vector->size = 0;

    free(vector->values);
    vector->values = nullptr;
}

void vectorDestroy(Vector* const vector) {
    destroyValuesIfNotEmpty(vector);
    free(vector->values);
    free(vector);
}
