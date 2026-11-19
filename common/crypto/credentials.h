// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "crypto.h"

// Planned to be using systemd-creds

void credentialsInit();
void credentialsGlobalSignKeypair(CryptoGenericKey* publicKey, CryptoSignSecretKey* secretKey);
void credentialsQuit();
