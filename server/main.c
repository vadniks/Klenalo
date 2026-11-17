// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#include "core/lifecycle.h"

// const int argc, const char* const* const argv, const char* const* const envp
int main() {
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    return 0;
}
