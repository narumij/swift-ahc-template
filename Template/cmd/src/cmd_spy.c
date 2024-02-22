#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#define BUFFER_SIZE 1024

// パイプとログファイルのファイルディスクリプタ
int pipe_stdin[2], pipe_stdout[2], pipe_stderr[2];
int log_fd;
volatile bool stop_processing = false;
bool log_stderr = false; // 標準エラーをログに記録するかどうかのフラグ

pthread_mutex_t stop_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_stop_processing() {
    pthread_mutex_lock(&stop_mutex);
    stop_processing = true;
    pthread_mutex_unlock(&stop_mutex);
}

bool should_stop_processing() {
    pthread_mutex_lock(&stop_mutex);
    bool should_stop = stop_processing;
    pthread_mutex_unlock(&stop_mutex);
    return should_stop;
}

void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        exit(EXIT_FAILURE);
    }
}

void log_write(const char *source, const char *buffer, ssize_t bytes_read) {
    ssize_t line_start = 0;
    for (ssize_t i = 0; i < bytes_read; ++i) {
        if (buffer[i] == '\n') {
            // 行の終わりを見つけたので、その行に対してログを書き込む
            dprintf(log_fd, "[%s] %.*s\n", source, (int)(i - line_start), buffer + line_start);
            line_start = i + 1; // 次の行の開始位置を更新
        }
    }
    // 最後の行が改行で終わっていない場合に対応
    if (line_start < bytes_read) {
        dprintf(log_fd, "[%s] %.*s", source, (int)(bytes_read - line_start), buffer + line_start);
    }
}

// 標準入力から子プロセスへのデータ転送を処理するスレッド関数
void *stdin_to_child(void *arg) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while (!should_stop_processing()) {
        bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            log_write("tester", buffer, bytes_read);
            // 子プロセスの標準入力へ送信
            write(pipe_stdin[1], buffer, bytes_read);
        } else if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // データが利用可能になるまで待機
            continue;
        } else if (bytes_read == 0) {
            set_stop_processing();
            break;
        }
    }
    close(pipe_stdin[1]); // 使用後に閉じる
    return NULL;
}

// 子プロセスの標準出力からのデータを処理するスレッド関数
void *child_stdout_to_parent(void *arg) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while (!should_stop_processing()) {
        bytes_read = read(pipe_stdout[0], buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            log_write("command", buffer, bytes_read);
            write(STDOUT_FILENO, buffer, bytes_read); // 親プロセスの標準出力へ送信
        } else if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // データが利用可能になるまで待機
            continue;
        } else if (bytes_read == 0) {
            set_stop_processing();
        }
    }
    return NULL;
}

// 子プロセスの標準エラーからのデータを処理するスレッド関数
void *child_stderr_to_log(void *arg) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while (!should_stop_processing()) {
        bytes_read = read(pipe_stderr[0], buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            if (log_stderr) {
                log_write("error", buffer, bytes_read);
            }
            write(STDERR_FILENO, buffer, bytes_read); // 親プロセスの標準エラーへ送信
        } else if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // データが利用可能になるまで待機
            continue;
        } else if (bytes_read == 0) {
            set_stop_processing();
        }
    }
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s [--log-stderr] log_file_path -- command [args...]\n", argv[0]);
        return 1;
    }

    int arg_index = 1;
    if (strcmp(argv[arg_index], "--log-stderr") == 0) {
        log_stderr = true;
        arg_index++;
    }

    log_fd = open(argv[arg_index], O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("Failed to open log file");
        return 1;
    }
    arg_index++; // ログファイルパスをスキップ

    if (strcmp(argv[arg_index], "--") != 0) {
        fprintf(stderr, "Missing '--' before command\n");
        return 1;
    }
    arg_index++; // "--" をスキップ

    // パイプの作成
    if (pipe(pipe_stdin) == -1 || pipe(pipe_stdout) == -1 || pipe(pipe_stderr) == -1) {
        perror("Failed to create pipes");
        return 1;
    }

    // 子プロセスの作成
    pid_t pid = fork();
    if (pid == -1) {
        perror("Failed to fork");
        return 1;
    }

    if (pid == 0) { // 子プロセス
        dup2(pipe_stdin[0], STDIN_FILENO);
        dup2(pipe_stdout[1], STDOUT_FILENO);
        dup2(pipe_stderr[1], STDERR_FILENO);

        close(pipe_stdin[1]);
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);

        execvp(argv[arg_index], &argv[arg_index]);
        perror("execvp failed");
        exit(1);
    } else { // 親プロセス
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);

        set_non_blocking(STDIN_FILENO);
        set_non_blocking(pipe_stdout[0]);
        set_non_blocking(pipe_stderr[0]);

        pthread_t thread1, thread2, thread3;
        pthread_create(&thread1, NULL, stdin_to_child, NULL);
        pthread_create(&thread2, NULL, child_stdout_to_parent, NULL);
        pthread_create(&thread3, NULL, child_stderr_to_log, NULL);

        int status;
        waitpid(pid, &status, 0);
        set_stop_processing();

        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);
        pthread_join(thread3, NULL);

        close(pipe_stdout[0]);
        close(pipe_stderr[0]);
        close(log_fd);
    }

    return 0;
}
