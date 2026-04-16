#ifndef MAFILE_H
#define MAFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef _MSC_VER
#define strdup _strdup
#endif
#include "cJSON.h"

typedef struct {
    uint64_t SteamID;
    // blah blah blah
} mafile_session_obj;

typedef struct {
    char *shared_secret;
    // blah blah blah
    char *account_name;
    mafile_session_obj *Session;
} mafile_obj;

mafile_obj *read_mafile(const char *);
void free_mafile_obj(mafile_obj *);

#endif // MAFILE_H
