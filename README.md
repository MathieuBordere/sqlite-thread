## GOAL
Measure the performance overhead of running a SQLite query in a thread that
needs to coordinate with a libuv eventloop.

## Scenarios
1. base:  Run the query in a single thread, this is the fastest possible way to step a
SQLite statement till the end.
2. pthread: Run the query from a pthread that coordinates with a regular main thread by
means of a semaphore. The threads play "ping-pong", the main thread kicks the
worker thread to do work, when the work is done the worker signals the main
thread that the work is done and sits waiting to be kicked again. This simulates
the case where a request that is not handled fully is put on the input queue of
a worker thread again. The worker thread returns control to the main thread after `batch_size` rows
have been returned.
3. uvpthread: Run the query from a pthread that coordinates with a libuv eventloop. The
eventloop wakes up the pthread when there's work to do while the pthread wakes
up the libuv eventloop with `uv_async_send`. The threads also play ping-pong
like before and also returns control to the eventloop after `batch_size` rows.
4. uvpthreadcont: Run the query from a pthread, but the worker thread keeps on working until
all work is done and pushes results onto a queue for the main thread to
handle. Every time `batch_size` rows have been collected, the worker pushes a result to the main thread.
In this test there is no real output pushed on a queue, but it's simulated by the main
thread and worker thread coordinating with a mutex to access a shared
data-structure.

## Options
Configurable batch size: larger batch => less context switching.

## Usage
```
./sqlite-thread -p <path> -m <mode> -b <batch_size>

<path> the tool expects to find a table called 'benchmark' in the db found here.
<mode> is one of base, pthread, uvpthread or uvpthreadcont
<batch_size> determines the amount of rows queried before returning control to
the main thread or pushing output to some queue.
```

