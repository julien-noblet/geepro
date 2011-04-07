#ifndef __STORINGS_H__
#define __STORINGS_H__
#include <stdio.h>

typedef struct
{
    FILE *file;
    char *sval;
    int   ival;
    float fval;
} store_str;

extern int store_constr(store_str *, const char *path, const char *file);
extern void store_destr(store_str *);
extern int store_get(store_str *, const char *key, char **value); // alloc memory for value, have to free()
extern int store_set(store_str *, const char *key, const char *string_to_store);

#endif

