#ifndef COMMON_H
#define COMMON_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096

/* вывод системной ошибки по коду GetLastError() */
void print_error(const char *msg);

/* логирование с указанием PID процесса и TID потока */
void log_pid(const char *tag, const char *msg);

#endif
