// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "../../common/crypto/crypto.h"

// similar to HTTPS's X.509
typedef struct xpacked {
    byte version; // same as app's version
    ulong validSince; // milliseconds since epoch
    ulong validUntil; // same
    bool admin; // other nodes trust this flag only this certificate has been signed with the predefined (for each system of nodes) admin's secret sign key, public key counterpart is possessed by each node in the system
    vec16ub ownerAddress;
    CryptoKey encryptionKey; // secret key to encrypt, but without the hidden public key it's impossible to decrypt
    CryptoSignature hostSignature; // host's own signature (standard/self-generated or admin/predefined)
    CryptoSignature globalSignature; // global system signature as in PingPacket everyone in the system trusts
} HostCertificate;

// if credentials file contains adminPrivateSignKey value then the program will use it and become "admin" by itself, having verified that the admin sign keypair is valid, if it's empty or doesn't exist nothing will happen
