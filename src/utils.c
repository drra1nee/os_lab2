#include "common.h"

/* преобразует код GetLastError() в понятный текст ошибки */
void print_error(const char *msg) {
    unsigned long err = GetLastError();
    char buf[256];
    unsigned long len = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, sizeof(buf), NULL
    );
    if (len > 0) {
        /* убираем перенос строки в конце сообщения от Windows */
        while (len > 0 && (buf[len - 1] == '\r' || buf[len - 1] == '\n')) {
            buf[--len] = '\0';
        }
        fprintf(stderr, "[PID %lu TID %lu] %s: %s (code %lu)\n",
                GetCurrentProcessId(), GetCurrentThreadId(), msg, buf, err);
    } else {
        fprintf(stderr, "[PID %lu TID %lu] %s: error code %lu\n",
                GetCurrentProcessId(), GetCurrentThreadId(), msg, err);
    }
}

/* печатает сообщение с текущим PID процесса и TID потока */
void log_pid(const char *tag, const char *msg) {
    fprintf(stderr, "[%s | PID %lu TID %lu] %s\n",
            tag, GetCurrentProcessId(), GetCurrentThreadId(), msg);
    fflush(stderr);
}

