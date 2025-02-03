
#pragma once

#include "defs.h"

typedef enum : byte {
    CONDITION_OBSERVER_VARIABLE_TYPE_BYTE = sizeof(byte),
    CONDITION_OBSERVER_VARIABLE_TYPE_CHAR = CONDITION_OBSERVER_VARIABLE_TYPE_BYTE,
    CONDITION_OBSERVER_VARIABLE_TYPE_BOOL = CONDITION_OBSERVER_VARIABLE_TYPE_BYTE,
    CONDITION_OBSERVER_VARIABLE_TYPE_SHORT = sizeof(short),
    CONDITION_OBSERVER_VARIABLE_TYPE_INT = sizeof(int),
    CONDITION_OBSERVER_VARIABLE_TYPE_LONG = sizeof(long),
} ConditionObserverVariableType;

struct _ConditionObserver;
typedef struct _ConditionObserver ConditionObserver;

ConditionObserver* conditionObserverCreate(void* const variablePointer, const ConditionObserverVariableType variableType);
void conditionObserverGetVariableValue(ConditionObserver* const observer, void* const buffer);
void conditionObserverSetVariableValue(ConditionObserver* const observer, const void* const value);
void conditionObserverWaitForVariableValue(ConditionObserver* const observer, const void* const value);
void conditionObserverDestroy(ConditionObserver* const observer);
