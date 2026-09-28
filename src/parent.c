#include "common.h"

/* удаление символов перевода строки \r и \n с конца буфера */
static void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
}

/* запуск дочернего процесса с перенаправлением stdin на канал и stdout на файл */
static BOOL spawn_child(const char *exe_name, HANDLE hInput, HANDLE hOutput, PROCESS_INFORMATION *pi) {
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdInput = hInput;   /* stdin дочернего процесса: чтение из канала */
    si.hStdOutput = hOutput; /* stdout дочернего процесса: запись в файл */
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE); /* stderr: вывод логов в консоль */
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(pi, sizeof(*pi));

    char cmd[MAX_PATH];
    snprintf(cmd, sizeof(cmd), "%s", exe_name);

    return CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, pi);
}

int main() {
    /* настройка кодировки UTF-8 в консоли для корректного вывода */
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    log_pid("PARENT", "Starting lab2 (Variant 18)");

    char file1[MAX_PATH];
    char file2[MAX_PATH];

    /* 1-я строка от пользователя: имя файла для child1 */
    printf("Enter filename for Child 1: ");
    fflush(stdout);
    if (!fgets(file1, sizeof(file1), stdin)) return 1;
    trim_newline(file1);

    /* 2-я строка от пользователя: имя файла для child2 */
    printf("Enter filename for Child 2: ");
    fflush(stdout);
    if (!fgets(file2, sizeof(file2), stdin)) return 1;
    trim_newline(file2);

    /* атрибуты безопасности: дескрипторы наследуются дочерними процессами */
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    /* открытие выходного файла для child1 (наследуемый дескриптор) */
    HANDLE hFile1 = CreateFileA(file1, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile1 == INVALID_HANDLE_VALUE) {
        print_error("CreateFile failed for file1");
        return 1;
    }

    /* создание канала pipe1 для передачи данных в child1 */
    HANDLE hPipe1Read = NULL, hPipe1Write = NULL;
    if (!CreatePipe(&hPipe1Read, &hPipe1Write, &sa, 0)) {
        print_error("CreatePipe 1 failed");
        CloseHandle(hFile1);
        return 1;
    }
    /* дескриптор записи родителя не должен наследоваться потомком */
    SetHandleInformation(hPipe1Write, HANDLE_FLAG_INHERIT, 0);

    /* запуск первого дочернего процесса */
    PROCESS_INFORMATION pi1;
    if (!spawn_child("child1.exe", hPipe1Read, hFile1, &pi1)) {
        print_error("Failed to spawn child1.exe");
        CloseHandle(hPipe1Read);
        CloseHandle(hPipe1Write);
        CloseHandle(hFile1);
        return 1;
    }
    /* закрываем дескрипторы чтения и файла в родителе - они переданы потомку */
    CloseHandle(hPipe1Read);
    CloseHandle(hFile1);
    printf("[PARENT] Spawned child1 (PID %lu) for file '%s'\n", pi1.dwProcessId, file1);

    /* открытие выходного файла для child2 (наследуемый дескриптор) */
    HANDLE hFile2 = CreateFileA(file2, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile2 == INVALID_HANDLE_VALUE) {
        print_error("CreateFile failed for file2");
        CloseHandle(hPipe1Write);
        return 1;
    }

    /* создание канала pipe2 для передачи данных в child2 */
    HANDLE hPipe2Read = NULL, hPipe2Write = NULL;
    if (!CreatePipe(&hPipe2Read, &hPipe2Write, &sa, 0)) {
        print_error("CreatePipe 2 failed");
        CloseHandle(hPipe1Write);
        CloseHandle(hFile2);
        return 1;
    }
    SetHandleInformation(hPipe2Write, HANDLE_FLAG_INHERIT, 0);

    /* запуск второго дочернего процесса */
    PROCESS_INFORMATION pi2;
    if (!spawn_child("child2.exe", hPipe2Read, hFile2, &pi2)) {
        print_error("Failed to spawn child2.exe");
        CloseHandle(hPipe2Read);
        CloseHandle(hPipe2Write);
        CloseHandle(hPipe1Write);
        CloseHandle(hFile2);
        return 1;
    }
    /* закрываем дескрипторы чтения и файла в родителе - они переданы потомку */
    CloseHandle(hPipe2Read);
    CloseHandle(hFile2);
    printf("[PARENT] Spawned child2 (PID %lu) for file '%s'\n", pi2.dwProcessId, file2);

    printf("\nEnter lines (empty line to finish):\n");
    char line[BUFFER_SIZE];
    size_t line_index = 0;

    /* чтение строк от пользователя и отправка по правилу варианта 18 */
    while (fgets(line, sizeof(line), stdin)) {
        trim_newline(line);
        if (line[0] == '\0') {
            break;
        }

        line_index++;
        size_t len = strlen(line);
        line[len] = '\n';
        line[len + 1] = '\0';
        unsigned long to_write = (unsigned long)(len + 1);
        unsigned long written = 0;

        /* нечетные строки отправляются в pipe1, четные - в pipe2 */
        if (line_index % 2 != 0) {
            printf("[PARENT] Line %zu (odd) -> child1\n", line_index);
            WriteFile(hPipe1Write, line, to_write, &written, NULL);
        } else {
            printf("[PARENT] Line %zu (even) -> child2\n", line_index);
            WriteFile(hPipe2Write, line, to_write, &written, NULL);
        }
    }

    log_pid("PARENT", "Input completed, closing pipes");
    /* закрываем концы записи каналов */
    CloseHandle(hPipe1Write);
    CloseHandle(hPipe2Write);

    /* ожидание корректного завершения обоих дочерних процессов */
    HANDLE children[2] = { pi1.hProcess, pi2.hProcess };
    WaitForMultipleObjects(2, children, TRUE, INFINITE);

    /* закрытие дескрипторов дочерних процессов и их первичных потоков */
    CloseHandle(pi1.hThread);
    CloseHandle(pi1.hProcess);
    CloseHandle(pi2.hThread);
    CloseHandle(pi2.hProcess);

    log_pid("PARENT", "All child processes finished. Exiting");
    return 0;
}
