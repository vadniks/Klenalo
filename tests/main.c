// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#include <stdio.h>
#include "../common/defs.h"

int main() {
    setbuf(stdout, nullptr); // so printf output can be yielded on test fails, otherwise ctest truncates ignores stdout



    return 0;
}
