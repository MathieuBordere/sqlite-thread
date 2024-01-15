#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <sqlite3.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum {
    MODE_BASE,
    MODE_PTHREAD,
    MODE_UVPTHREAD
} OperatingMode;

static void parseCommandLine(int argc, char *argv[], OperatingMode *mode, int *batchSize, char **path) {
    int opt;
    *mode = -1;
    *batchSize = 1;

    while ((opt = getopt(argc, argv, "p:m:b:")) != -1) {
        switch (opt) {
            case 'p':
                *path = optarg;
                break;
            case 'm':
                if (strcmp(optarg, "base") == 0) {
                    *mode = MODE_BASE;
                } else if (strcmp(optarg, "pthread") == 0) {
                    *mode = MODE_PTHREAD;
                } else if (strcmp(optarg, "uvpthread") == 0) {
                    *mode = MODE_UVPTHREAD;
                } else {
                    fprintf(stderr, "Invalid mode: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'b':
                *batchSize = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "Usage: %s -p <path> -m <mode> -b <batchSize>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

static sqlite3* openDbConnection(char *path) {
    sqlite3 *db;
    int rc;

    rc = sqlite3_open(path, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }

    rc = sqlite3_exec(db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);
    if (rc) {
        fprintf(stderr, "Failed to set wal-mode: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }

    return db;
}

static void runBase(char *path) {
    sqlite3 *db;
    int rc = -1;
    sqlite3_stmt *stmt;
    char* q = "SELECT * FROM benchmark";
    db = openDbConnection(path);
    rc = sqlite3_prepare_v2(db, q, strlen(q), &stmt, NULL);
    assert(rc == 0);
    rc = SQLITE_ROW;
    while (rc == SQLITE_ROW) {
        rc = sqlite3_step(stmt);
        // Don't process output, this will take the same time in all modes.
    }
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "sqlite3_step error: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
}

enum bm_state {
    BM_STATE_INIT,
    BM_STATE_CONTINUE,
    BM_STATE_DONE,
};

struct thread_ctx {
    char *db_path;
    int batchSize;
    enum bm_state state;
    sqlite3 *db;
    sqlite3_stmt *stmt;
    sem_t *sem_in;
    sem_t *sem_out;
};

void* pthread_task(void* ctx) {
    int rc = -1;
    char* q = "SELECT * FROM benchmark";
    struct thread_ctx *context = ctx;
    int n_steps = 0;

    while (1) {
        sem_wait(context->sem_in);
        switch (context->state) {
        case BM_STATE_INIT: {
            context->db = openDbConnection(context->db_path);
            rc = sqlite3_prepare_v2(context->db, q, strlen(q), &context->stmt, NULL);
            assert(rc == 0);
            context->state = BM_STATE_CONTINUE;
            // Fallthrough is intended.
        }
        case BM_STATE_CONTINUE: {
            n_steps = 0;
            rc = SQLITE_ROW;
            while (rc == SQLITE_ROW && n_steps < context->batchSize) {
                rc = sqlite3_step(context->stmt);
                // Don't process output, this will take the same time in all modes.
                n_steps++;
            }
            if (rc == SQLITE_ROW) {
                context->state = BM_STATE_CONTINUE;
            } else if (rc == SQLITE_DONE) {
                context->state = BM_STATE_DONE;
            } else {
                fprintf(stderr, "sqlite3_step error: %s\n", sqlite3_errmsg(context->db));
                sqlite3_close(context->db);
                // Will also cause the main thread to exit.
                exit(EXIT_FAILURE);
            }
            break;
        }
        case BM_STATE_DONE:
            break;
        }
        sem_post(context->sem_out);
    }
    return 0;
}

static void runPthread(char *path, int batchSize) {
    pthread_t t;
    sem_t sem_in;
    sem_t sem_out;
    sem_init(&sem_in, 0, 1);
    sem_init(&sem_out, 0, 0);
    struct thread_ctx ctx = {
        .db_path = path,
        .batchSize = batchSize,
        .state = BM_STATE_INIT,
        .db = NULL,
        .stmt = NULL,
        .sem_in = &sem_in,
        .sem_out = &sem_out,
    };
    pthread_create(&t, NULL, pthread_task, &ctx);
    while(1) {
        sem_wait(&sem_out);

        // Process output here

        if (ctx.state == BM_STATE_DONE) {
            sem_close(&sem_in);
            sem_close(&sem_out);
            exit(EXIT_SUCCESS);
        }

        sem_post(&sem_in);
    }
}

int main(int argc, char *argv[]) {
    OperatingMode mode;
    int batchSize = 1;
    sqlite3 *db = NULL;
    char *path = NULL;

    parseCommandLine(argc, argv, &mode, &batchSize, &path);

    switch (mode) {
        case MODE_BASE:
            runBase(path);
            break;
        case MODE_PTHREAD:
            runPthread(path, batchSize);
            break;
        case MODE_UVPTHREAD:
            // UV Pthread mode operations
            break;
        default:
            fprintf(stderr, "Invalid mode\n");
            return 1;
    }

    sqlite3_close(db);
    return 0;
}

