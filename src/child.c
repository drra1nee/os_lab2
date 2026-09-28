#include "common.h"

/* проверка латинских гласных букв */
static int is_ascii_vowel(char c) {
    switch (c) {
        case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':
        case 'A': case 'E': case 'I': case 'O': case 'U': case 'Y':
            return 1;
        default:
            return 0;
    }
}

/* проверка символа на гласную букву */
static int is_utf8_vowel(const unsigned char *s, int *len) {
    if (s[0] < 0x80) {
        *len = 1;
        return is_ascii_vowel((char)s[0]);
    }

    /* русские гласные в 2-байтовой кодировке */
    if (s[0] == 0xD0 && s[1] != '\0') {
        *len = 2;
        unsigned char c = s[1];
        if (c == 0x90 || c == 0xB0 ||  /* А, а */
            c == 0x95 || c == 0xB5 ||  /* Е, е */
            c == 0x81 ||              /* Ё */
            c == 0x98 || c == 0xB8 ||  /* И, и */
            c == 0x9E || c == 0xBE ||  /* О, о */
            c == 0xA3 ||              /* У */
            c == 0xAB ||              /* Ы */
            c == 0xAD ||              /* Э */
            c == 0xAE ||              /* Ю */
            c == 0xAF) {              /* Я */
            return 1;
        }
        return 0;
    }

    if (s[0] == 0xD1 && s[1] != '\0') {
        *len = 2;
        unsigned char c = s[1];
        if (c == 0x91 ||  /* ё */
            c == 0x83 ||  /* у */
            c == 0x8B ||  /* ы */
            c == 0x8D ||  /* э */
            c == 0x8E ||  /* ю */
            c == 0x8F) {  /* я */
            return 1;
        }
        return 0;
    }

    /* определение длины других многобайтовых символов */
    if ((s[0] & 0xE0) == 0xC0) *len = 2;
    else if ((s[0] & 0xF0) == 0xE0) *len = 3;
    else if ((s[0] & 0xF8) == 0xF0) *len = 4;
    else *len = 1;

    return 0;
}

/* формирование строки без гласных букв */
static void filter_vowels(const char *src, char *dst) {
    const unsigned char *p = (const unsigned char *)src;
    int di = 0;
    while (*p) {
        int char_len = 1;
        if (is_utf8_vowel(p, &char_len)) {
            p += char_len; /* Пропускаем гласную букву */
        } else {
            for (int i = 0; i < char_len && *p; ++i) {
                dst[di++] = (char)(*p++);
            }
        }
    }
    dst[di] = '\0';
}

int main(void) {
    log_pid("CHILD", "Started, reading from pipe...");

    /* стандартные потоки были перенаправлены родителем: stdin из pipe, stdout в файл */
    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hStdIn == INVALID_HANDLE_VALUE || hStdOut == INVALID_HANDLE_VALUE) {
        print_error("Failed to get standard handles");
        return 1;
    }

    char in_buf[BUFFER_SIZE];
    char line_buf[BUFFER_SIZE];
    char filtered[BUFFER_SIZE];
    int line_pos = 0;
    unsigned long bytes_read = 0;

    /* читаем байты из stdin (канала) до тех пор, пока родитель не закроет его */
    while (ReadFile(hStdIn, in_buf, sizeof(in_buf) - 1, &bytes_read, NULL) && bytes_read > 0) {
        for (unsigned long i = 0; i < bytes_read; ++i) {
            char c = in_buf[i];
            if (c == '\n' || line_pos >= BUFFER_SIZE - 2) {
                line_buf[line_pos] = '\0';
                filter_vowels(line_buf, filtered);

                int len = strlen(filtered);
                filtered[len] = '\n';
                filtered[len + 1] = '\0';

                unsigned long written = 0;
                /* запись обработанной строки в stdout */
                if (!WriteFile(hStdOut, filtered, (unsigned long)(len + 1), &written, NULL)) {
                    print_error("WriteFile failed");
                }

                line_pos = 0;
            } else if (c != '\r') {
                line_buf[line_pos++] = c;
            }
        }
    }

    /* обработка последней строки, если она не оканчивалась на \n */
    if (line_pos > 0) {
        line_buf[line_pos] = '\0';
        filter_vowels(line_buf, filtered);
        int len = strlen(filtered);
        filtered[len] = '\n';
        filtered[len + 1] = '\0';

        unsigned long written = 0;
        WriteFile(hStdOut, filtered, (unsigned long)(len + 1), &written, NULL);
    }

    log_pid("CHILD", "Finished processing");
    return 0;
}
