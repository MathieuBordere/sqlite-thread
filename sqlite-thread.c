#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>
#include <uv.h>

enum operating_mode {
    MODE_BASE,
    MODE_PTHREAD,
    MODE_UVPTHREAD,
    MODE_UVPTHREADCONT,
};

enum bm_state {
    BM_STATE_INIT,
    BM_STATE_CONTINUE,
    BM_STATE_DONE,
};


static sqlite3* open_db_connection(char *path) {
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

/* ====== BASE  ============================================================= */

static void run_base(char *path) {
    sqlite3 *db;
    int rc = -1;
    sqlite3_stmt *stmt;
    char* q = "SELECT * FROM benchmark";
    db = open_db_connection(path);
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

/* ====== PTHREAD =========================================================== */

struct thread_ctx {
    char *db_path;
    int batch_size;
    enum bm_state state;
    sqlite3 *db;
    sqlite3_stmt *stmt;
    sem_t *sem_in; // signals thread that there is work to do.
    sem_t *sem_out; // signals main thread that work is done.
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
            context->db = open_db_connection(context->db_path);
            rc = sqlite3_prepare_v2(context->db, q, strlen(q), &context->stmt, NULL);
            assert(rc == 0);
            context->state = BM_STATE_CONTINUE;
            // Fallthrough is intended.
        }
        case BM_STATE_CONTINUE: {
            n_steps = 0;
            rc = SQLITE_ROW;
            while (rc == SQLITE_ROW && n_steps < context->batch_size) {
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

static void runPthread(char *path, int batch_size) {
    pthread_t t;
    sem_t sem_in;
    sem_t sem_out;
    sem_init(&sem_in, 0, 1);
    sem_init(&sem_out, 0, 0);
    struct thread_ctx ctx = {
        .db_path = path,
        .batch_size = batch_size,
        .state = BM_STATE_INIT,
        .db = NULL,
        .stmt = NULL,
        .sem_in = &sem_in,
        .sem_out = &sem_out,
    };
    assert(pthread_create(&t, NULL, pthread_task, &ctx) == 0);
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

/* ====== UVPTHREAD ========================================================= */

struct async_thread_ctx {
    char *db_path;
    int batch_size;
    enum bm_state state;
    sqlite3 *db;
    sqlite3_stmt *stmt;
    sem_t *sem_in; // signals thread that there is work to do.
    uv_async_t *async; // signals eventloop that work is done.
};

void* uvpthread_task(void* ctx) {
    int rc = -1;
    char* q = "SELECT * FROM benchmark";
    struct async_thread_ctx *context = ctx;
    int n_steps = 0;

    while (1) {
        sem_wait(context->sem_in);
        switch (context->state) {
        case BM_STATE_INIT: {
            context->db = open_db_connection(context->db_path);
            rc = sqlite3_prepare_v2(context->db, q, strlen(q), &context->stmt, NULL);
            assert(rc == 0);
            context->state = BM_STATE_CONTINUE;
            // Fallthrough is intended.
        }
        case BM_STATE_CONTINUE: {
            n_steps = 0;
            rc = SQLITE_ROW;
            while (rc == SQLITE_ROW && n_steps < context->batch_size) {
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
        uv_async_send(context->async);
    }
    return 0;
}

void async_cb(uv_async_t *handle) {
    struct async_thread_ctx *ctx = uv_handle_get_data((void*)handle);

    // Process output here.

    if (ctx->state == BM_STATE_DONE) {
        sem_close(ctx->sem_in);
        exit(EXIT_SUCCESS);
    }

    sem_post(ctx->sem_in);
}

static void run_uv(char *path, int batch_size) {
    pthread_t t;
    sem_t sem_in;
    sem_init(&sem_in, 0, 1);
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    uv_async_t async;
    assert(uv_async_init(loop, &async, async_cb) == 0);
    struct async_thread_ctx ctx = {
        .db_path = path,
        .batch_size = batch_size,
        .state = BM_STATE_INIT,
        .db = NULL,
        .stmt = NULL,
        .sem_in = &sem_in,
        .async = &async,
    };
    uv_handle_set_data((uv_handle_t *)&async, &ctx);
    assert(pthread_create(&t, NULL, uvpthread_task, &ctx) == 0);
    assert(uv_run(loop, UV_RUN_DEFAULT) == 0);
    uv_loop_close(loop);
    free(loop);
    exit(EXIT_SUCCESS);
}

/* ======= UVPTHREADCONT ==================================================== */

struct async_cont_thread_ctx {
    char *db_path;
    int batch_size;
    enum bm_state state;
    sqlite3 *db;
    sqlite3_stmt *stmt;
    uv_async_t *async; // signals eventloop that there is output available.
    pthread_mutex_t *mtx; // protects state
};


void* uvpthreadcont_task(void* ctx) {
    int rc = -1;
    char* q = "SELECT * FROM benchmark";
    struct async_cont_thread_ctx *context = ctx;
    int n_steps = 0;

    while (1) {
        pthread_mutex_lock(context->mtx);
        switch (context->state) {
        case BM_STATE_INIT: {
            context->db = open_db_connection(context->db_path);
            rc = sqlite3_prepare_v2(context->db, q, strlen(q), &context->stmt, NULL);
            assert(rc == 0);
            context->state = BM_STATE_CONTINUE;
            // Fallthrough is intended.
        }
        case BM_STATE_CONTINUE: {
            n_steps = 0;
            rc = SQLITE_ROW;
            pthread_mutex_unlock(context->mtx);
            while (rc == SQLITE_ROW && n_steps < context->batch_size) {
                rc = sqlite3_step(context->stmt);
                // Don't process output, this will take the same time in all modes.
                n_steps++;
            }
            pthread_mutex_lock(context->mtx);
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
            pthread_mutex_unlock(context->mtx);
            break;
        }
        case BM_STATE_DONE:
            pthread_mutex_unlock(context->mtx);
            return 0;
        }
        uv_async_send(context->async);
    }
    return 0;
}

void async_cb_cont(uv_async_t *handle) {
    struct async_cont_thread_ctx *ctx = uv_handle_get_data((void*)handle);
    pthread_mutex_lock(ctx->mtx);

    // Process output:

    if (ctx->state == BM_STATE_DONE) {
        exit(EXIT_SUCCESS);
    }

    pthread_mutex_unlock(ctx->mtx);
}

static void run_uv_cont(char *path, int batch_size) {
    pthread_t t;
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    uv_async_t async;
    assert(uv_async_init(loop, &async, async_cb_cont) == 0);
    pthread_mutex_t mtx;
    assert(pthread_mutex_init(&mtx, NULL) == 0);
    struct async_cont_thread_ctx ctx = {
        .db_path = path,
        .batch_size = batch_size,
        .state = BM_STATE_INIT,
        .db = NULL,
        .stmt = NULL,
        .mtx = &mtx,
        .async = &async,
    };
    uv_handle_set_data((uv_handle_t *)&async, &ctx);
    assert(pthread_create(&t, NULL, uvpthreadcont_task, &ctx) == 0);
    assert(uv_run(loop, UV_RUN_DEFAULT) == 0);
    uv_loop_close(loop);
    free(loop);
    pthread_mutex_destroy(&mtx);
    exit(EXIT_SUCCESS);
}

/* ========================================================================== */

static void parseCommandLine(int argc, char *argv[], enum operating_mode *mode,
                             int *batch_size, char **path) {
    int opt;
    *mode = -1;
    *batch_size = 1;
    *path = NULL;

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
                } else if (strcmp(optarg, "uvpthreadcont") == 0) {
                    *mode = MODE_UVPTHREADCONT;
                } else {
                    fprintf(stderr, "Invalid mode: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'b':
                *batch_size = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "Usage: %s -p <path> -m <mode> -b <batch_size>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}
int main(int argc, char *argv[]) {
    enum operating_mode mode;
    int batch_size = 1;
    sqlite3 *db = NULL;
    char *path = NULL;

    parseCommandLine(argc, argv, &mode, &batch_size, &path);

    switch (mode) {
        case MODE_BASE:
            run_base(path);
            break;
        case MODE_PTHREAD:
            runPthread(path, batch_size);
            break;
        case MODE_UVPTHREAD:
            run_uv(path, batch_size);
            break;
        case MODE_UVPTHREADCONT:
            run_uv_cont(path, batch_size);
            break;
        default:
            fprintf(stderr, "Invalid mode\n");
            return 1;
    }

    sqlite3_close(db);
    return 0;
}

