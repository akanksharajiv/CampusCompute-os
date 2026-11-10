#ifndef INPUT_READER_H
#define INPUT_READER_H
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

/* 1=integer, 0=EOF, -1=malformed/out of range/read error. */
static int input_integer(FILE *file, long long *value)
{
    char token[96];
    size_t length = 0;
    int c, too_long = 0;
    do { c = fgetc(file); } while (c != EOF && isspace((unsigned char)c));
    if (c == EOF) return ferror(file) ? -1 : 0;
    do {
        if (length + 1 < sizeof token) token[length++] = (char)c;
        else too_long = 1;
        c = fgetc(file);
    } while (c != EOF && !isspace((unsigned char)c));
    if (too_long || ferror(file)) return -1;
    token[length] = '\0';
    char *end;
    errno = 0;
    *value = strtoll(token, &end, 10);
    return errno == 0 && end != token && *end == '\0' ? 1 : -1;
}

static int input_finished(FILE *file)
{
    long long extra;
    return input_integer(file, &extra) == 0;
}
#endif
