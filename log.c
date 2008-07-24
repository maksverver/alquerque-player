#include "log.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

void info(const char *format, ...)
{
    time_t t;
    char buf[32];
    va_list ap;

    time(&t);
    strftime(buf, sizeof(buf), "[%H:%M:%S] ", gmtime(&t));
    fputs(buf, stderr);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fputc('\n', stderr);
}
