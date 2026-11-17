// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "shared.h"

void discoveryInit(const Address* host);
void discoveryListen();
void discoveryPing();
void discoveryQuit();
