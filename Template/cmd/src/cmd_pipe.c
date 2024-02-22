#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024
volatile int stop_requested = 0;
pthread_mutex_t stop_mutex = PTHREAD_MUTEX_INITIALIZER;

int logFileFd = -1; // グローバル変数でログファイルのファイルディスクリプタを保持

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

void request_stop() {
    pthread_mutex_lock(&stop_mutex);
    stop_requested = 1;
    pthread_mutex_unlock(&stop_mutex);
}

int should_stop() {
    pthread_mutex_lock(&stop_mutex);
    int result = stop_requested;
    pthread_mutex_unlock(&stop_mutex);
    return result;
}

// 名前付きパイプが存在しない場合に作成
int create_named_pipe_if_not_exists(const char *path, mode_t mode) {
    struct stat st;
    int result = stat(path, &st);
    if (result == 0) { // ファイルが存在する
        if (!S_ISFIFO(st.st_mode)) { // しかし、FIFOではない
            fprintf(stderr, "%s is not a FIFO\n", path);
            exit(EXIT_FAILURE);
        }
    } else {
        if (errno == ENOENT) { // ファイルが存在しない
            if (mkfifo(path, mode) != 0) {
                perror("mkfifo");
                exit(EXIT_FAILURE);
            }
        } else {
            perror("stat");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

void* tester_to_cmd(void *arg) {
    char* write_pipe_path = (char*)arg;
    create_named_pipe_if_not_exists(write_pipe_path, 0666); // 書き込み用パイプを作成

    int fd;
    char buffer[BUFFER_SIZE];
    
    set_non_blocking(STDIN_FILENO);
    
    if ((fd = open(write_pipe_path, O_WRONLY)) < 0) {
        perror("open write_pipe");
        exit(EXIT_FAILURE);
    }
    
    while (!should_stop()) {
        ssize_t read_bytes = read(STDIN_FILENO, buffer, BUFFER_SIZE);
        if (read_bytes > 0) {
            if (write(fd, buffer, read_bytes) != read_bytes) {
                perror("write to pipe");
                break;
            }
            if (logFileFd != -1) {
                write(logFileFd, buffer, read_bytes); // ログファイルにも書き込む
            }
        } else if (read_bytes < 0 && errno != EAGAIN) {
            perror("read stdin");
            break;
        }
    }
    
    close(fd);
    return NULL;
}

void cmd_to_tester(const char* read_pipe_path) {
    create_named_pipe_if_not_exists(read_pipe_path, 0666); // 読み込み用パイプを作成

    int fd;
    char buffer[BUFFER_SIZE];
    
    if ((fd = open(read_pipe_path, O_RDONLY)) < 0) {
        perror("open read_pipe");
        exit(EXIT_FAILURE);
    }
    
    while (!should_stop()) {
        ssize_t read_bytes = read(fd, buffer, BUFFER_SIZE);
        if (read_bytes > 0) {
            if (write(STDOUT_FILENO, buffer, read_bytes) != read_bytes) {
                perror("write to stdout");
                break;
            }
            if (logFileFd != -1) {
                write(logFileFd, buffer, read_bytes); // ログファイルにも書き込む
            }
        } else if (read_bytes == 0) {
            request_stop();
            break;
        } else if (errno != EAGAIN) {
            perror("read pipe");
            break;
        }
    }
    
    close(fd);
}

int main(int argc, char *argv[]) {
    pthread_t thread_id;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            logFileFd = open(argv[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (logFileFd == -1) {
                perror("Failed to open log file");
                exit(EXIT_FAILURE);
            }
            i++;
        }
    }

    if (argc < 3) {
        fprintf(stderr, "Usage: %s /path/to/write_named_pipe /path/to/read_named_pipe [--log /path/to/logfile]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if (pthread_create(&thread_id, NULL, tester_to_cmd, (void*)argv[1])) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    cmd_to_tester(argv[2]);

    pthread_join(thread_id, NULL);

    if (logFileFd != -1) {
        close(logFileFd);
    }

    return 0;
}
