## GOAL
Measure the performance overhead of running a SQLite query in a thread that
needs to coordinate with a libuv eventloop.

## Scenarios
a. Run the query in a single thread.
b. Run the query from a pthread that coordinates with a regular main thread by
means of a semaphore.
c. Run the query from a pthread that coordinates with a libuv eventloop. The
eventloop wakes up the pthread when there's work to do while the pthread wakes
up the libuv eventloop with `uv_async_send`.

## Options
Configurable batch size: larger batch => less context switching.

## Usage
./sqlite-thread -p <path> -m <mode> -b <batchSize>
@path the tool expects to find a table called 'benchmark' in the db found at here.
@mode is one of base, pthread or uvpthread.
@batchSize determines the amount of rows queried before returning control to
the main thread.

