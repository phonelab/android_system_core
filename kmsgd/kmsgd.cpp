// Copyright 2006 The Android Open Source Project

#include <log/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include <signal.h>

/**
 * Kernel message daemon.
 *
 * Forward kernel printk messages to logcat.
 */


#define TAG                             "KernelPrintk"
#define MAX_LINE_WIDTH                   (128*1024)

static char line_buf[MAX_LINE_WIDTH];

static android_LogPriority g_prio = ANDROID_LOG_DEBUG;
static log_id_t g_log_id = LOG_ID_MAIN;

static void
read_kmsgs(int fd) {
    fd_set readset;
    int max_fd = fd;

    int ret;
    FILE* fp;
    char* buf;
    size_t len;

    if ((fp = fdopen(fd, "r")) == NULL) {
        perror("fdopen");
        return;
    }

    while (1) {
        do {
            FD_ZERO(&readset);
            FD_SET(fd, &readset);

            ret = select(max_fd+1, &readset, NULL, NULL, NULL);
        } while (ret == -1 && errno == EINTR);

        if (ret > 0 && FD_ISSET(fd, &readset)) {
            buf = line_buf;
            len = MAX_LINE_WIDTH;
            if ((ret = getline(&buf, &len, fp)) != -1) {
                /* fix ^M at line end */
                if (buf[ret-2] == 0xD) {
                    buf[ret-2] = '\n';
                    buf[ret-1] = '\0';
                }
                __android_log_buf_write(g_log_id, g_prio, TAG, buf);
            }
            if (buf != line_buf) {
                free(buf);
            }
        }
    }
}

int
main(int argc, char **argv) {
    int ret;
    int fd;

    if ((fd = open("/proc/kmsg", O_RDONLY)) < 0) {
        perror("open");
        exit(-1);
    }

    read_kmsgs(fd);
    return 0;
}
