#ifndef STEAM_GUARD_H
#define STEAM_GUARD_H

#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#else
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif
#include "server_time.h"

int gen_auth_code(char *, const char *, const int);

#endif // STEAM_GUARD_H
