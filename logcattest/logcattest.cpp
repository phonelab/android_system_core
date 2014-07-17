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


#define TAG "LogcatTest"
#define LOG_DIR "/sdcard/logcattest"
#define LOG_FILE "logcat.log"

useconds_t g_interval_usec = 1000L;
int g_msg_size = 1024;
log_id_t g_log_id = LOG_ID_SYSTEM;
android_LogPriority g_prio = ANDROID_LOG_VERBOSE;

static char* msg = NULL;
static char* g_log_format = "threadtimelid";
static int g_rotate_size_kb = 1024;
static int g_rotate_count = 100;
static char g_log_file[128];
static bool g_monitor = false;
static int g_duration_sec = 60;
static int g_abort_lines = 1024;
static char* g_buffer = "system";


static void
show_help(const char *cmd) {
    fprintf(stderr,"Usage: %s [options]\n", cmd);

    fprintf(stderr, "options include:\n"
                    "  -h              Show this help message.\n"
                    "  -i              Set log interval in microseconds.\n"
                    "  -s              Set log message size.\n"
                    "  -n              Set logcat rotation count.\n"
                    "  -r              Set logcat rotation size in KB.\n"
                    "  -v              Set logcat format.\n"
                    "  -b              Set logcat buffer.\n"
                    "  -m              Monitor logcat status.\n"
                    "  -t              Set test duration. Must use without -m.\n"
                    "  -a              Missing lines threshold before logcat abort.\n"
                    "  -p              Print default parameters and exit.\n"
                    
           );
}

static void
show_param() {
    fprintf(stderr, "===========================================================\n");
    fprintf(stderr, "Log interval:                %lu usec.\n", g_interval_usec);
    fprintf(stderr, "Log message size:            %d bytes.\n", g_msg_size);

    if (g_monitor) {
        fprintf(stderr, "Monitor:                     %d.\n", g_monitor);
        fprintf(stderr, "Rotation count:              %d.\n", g_rotate_count);
        fprintf(stderr, "Roation size:                %d KB.\n", g_rotate_size_kb);
        fprintf(stderr, "Log format:                  %s.\n", g_log_format);
        fprintf(stderr, "Log file:                    %s.\n", g_log_file);
        fprintf(stderr, "Abort lines:                 %d.\n", g_abort_lines);
    }
    else {
        fprintf(stderr, "Test duration:               %d sec.\n", g_duration_sec);
    }
    fprintf(stderr, "Buffer:                      %s.\n", g_buffer);
    fprintf(stderr, "===========================================================\n");
}

static void
stress_test() {
    while (true) {
        __android_log_buf_write(g_log_id, g_prio, TAG, msg);

        if (usleep(g_interval_usec) < 0) {
            perror("usleep");
            exit(-1);
        }
    }
}

int
main(int argc, char **argv) {
    int ret;

    ret = mkdir(LOG_DIR, S_IRWXU);
    if (ret < 0) {
        if (errno != EEXIST) {
            perror("mkdir");
            fprintf(stderr, "Failed to create directory %s.\n", LOG_DIR);
            exit(-1);
        }
    }
    sprintf(g_log_file, "%s/%s", LOG_DIR, LOG_FILE);


    while (true) {
        ret = getopt(argc, argv, "hi:s:n:r:v:b:pmt:a:");
        if (ret < 0) {
            break;
        }

        switch(ret) {
            case 'h':
                show_help(argv[0]);
                return 0;

            case 'i': 
                g_interval_usec = strtol(optarg, NULL, 10);
                break;

            case 's':
                g_msg_size = (int) strtol(optarg, NULL, 10);
                break;

            case 'n':
                g_rotate_count = (int) strtol(optarg, NULL, 10);
                break;

            case 'r':
                g_rotate_size_kb = (int) strtol(optarg, NULL, 10);
                break;

            case 'v':
                g_log_format = strdup(optarg);
                break;

            case 'b':
                g_buffer = strdup(optarg);
                break;

            case 'p' :
                show_param();
                return 0;

            case 'm' :
                g_monitor = true;
                break;

            case 't' :
                g_duration_sec = (int) strtol(optarg, NULL, 10);
                break;

            case 'a' :
                g_abort_lines = (int) strtol(optarg, NULL, 10);
                break;

            default:
                fprintf(stderr,"Unrecognized Option\n");
                show_help(argv[0]);
                exit(-1);
                break;
        }
    }

    if (strcmp(g_buffer, "system") == 0) {
        g_log_id = LOG_ID_SYSTEM;
    }
    else if (strcmp(g_buffer, "main") == 0) {
        g_log_id = LOG_ID_MAIN;
    }
    else if (strcmp(g_buffer, "radio") == 0) {
        g_log_id = LOG_ID_RADIO;
    }
    else if (strcmp(g_buffer, "events") == 0) {
        g_log_id = LOG_ID_EVENTS;
    }
    else {
        fprintf(stderr, "Unknown buffer: %s\n", g_buffer);
        exit(-1);
    }

    show_param();


    msg = (char*) malloc(g_msg_size);
    memset(msg, 'X', g_msg_size);
    msg[g_msg_size-1] = '\0';


    char cmd[128];

    sprintf(cmd, "logcat -c -b %s", g_buffer);
    fprintf(stderr, "%s\n", cmd);
    system(cmd);


    time_t start = time(0);
    pid_t child = fork();

    if (child > 0) {
        fprintf(stderr, "Stress test running...\n");
        stress_test();
    }
    else if (child == 0) {
        if (g_monitor) {
            sprintf(cmd, "logcat -v %s -f %s -r %d -n %d -a %d -b %s", g_log_format, g_log_file, g_rotate_size_kb, g_rotate_count, g_abort_lines, g_buffer);
            fprintf(stderr, "%s\n", cmd);
            fprintf(stderr, "Waiting logcat to abort...\n");
            ret = system(cmd);

            time_t end = time(0);
            fprintf(stderr, "It takes %ld seconds for logcat to crash.\n", end-start);
        }
        else {
            fprintf(stderr, "Waiting %d seconds...\n", g_duration_sec);
            sleep(g_duration_sec);
        }
        fprintf(stderr, "Finished. Killing child...\n");
        kill(child, SIGKILL);
    }
    else {
        perror("fork");
        exit(-1);
    }

    return 0;
}
