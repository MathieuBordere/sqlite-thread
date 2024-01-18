## GOAL
Measure the performance overhead of running a SQLite query in a thread that
needs to coordinate with a libuv eventloop. The tool just selects all the rows
from the database provided to the tool.

## Scenarios
1. base:  Run the query in a single thread, this is the fastest possible way to step a
SQLite statement till the end.
2. pthread: Run the query from a pthread that coordinates with a regular main thread by
means of a semaphore. The threads play "ping-pong", the main thread kicks the
worker thread to do work, when the work is done the worker signals the main
thread that the work is done and sits waiting to be kicked again. This simulates
the case where a request that is not handled fully is put on the input queue of
a worker thread again. The worker thread returns control to the main thread after `batch_size` rows
have been returned. This simulates the case where a request is put on the input queue
of a worker thread, the worker thread steps the query, fills a batch, returns control
to another thread that sends the result to a client and asks the worker thread to continue
stepping the query.
3. uvpthread: Run the query from a pthread that coordinates with a libuv eventloop. The
eventloop wakes up the pthread when there's work to do while the pthread wakes
up the libuv eventloop with `uv_async_send`. The threads also play ping-pong
like before and also returns control to the eventloop after `batch_size` rows. Simulates the
same use-case as pthread, but with a libuv eventloop acting as the main thread.
4. uvpthreadcont: Run the query from a pthread, but the worker thread keeps on working until
all work is done and pushes results onto a queue every time `batch_size` rows have been collected
for the main thread to handle.
In this test there is no real output pushed on a queue, but it's simulated by the main
thread and worker thread coordinating with a mutex to access a shared data-structure.

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

## Results

Preleminary conclusions:

- Batch size has large influence on running time of stepping through the whole query, the bigger the batch, the better.
- `uvpthreadcont` running time is close to `base` case.
- The initial model of putting a request on the in_queue of a DbThread, stepping the query until the batch is full and
the only continuing to step the query once the main thread has handled the output looks to be slow.
- A better model looks to be `uvpthreadcont` where the worker thread is busy a larger part of the time. An extra
point of attention in this mode is that the worker shouldn't let the output queue grow boundless. The idea would
be to put a limit on the output queue and when that limit is reached for a certain query, the DbThread starts handling
another query in parallell.

```
format: data/measurements/output-$mode-$rows-$batch_size

==============================================================================
Output of running tests on database with 1.000 rows of 1KB

data/measurements/output-base-1000-1:                2,15 msec task-clock
data/measurements/output-pthread-1000-1:             17,88 msec task-clock
data/measurements/output-uvpthread-1000-1:           19,11 msec task-clock
data/measurements/output-uvpthreadcont-1000-1:       4,03 msec task-clock

data/measurements/output-base-1000-4:                1,87 msec task-clock
data/measurements/output-pthread-1000-4:             6,59 msec task-clock
data/measurements/output-uvpthread-1000-4:           7,07 msec task-clock
data/measurements/output-uvpthreadcont-1000-4:       3,90 msec task-clock

data/measurements/output-base-1000-32:               1,84 msec task-clock
data/measurements/output-pthread-1000-32:            3,25 msec task-clock
data/measurements/output-uvpthread-1000-32:          3,39 msec task-clock
data/measurements/output-uvpthreadcont-1000-32:      3,96 msec task-clock

data/measurements/output-base-1000-128:              1,81 msec task-clock
data/measurements/output-pthread-1000-128:           3,06 msec task-clock
data/measurements/output-uvpthread-1000-128:         3,06 msec task-clock
data/measurements/output-uvpthreadcont-1000-128:     3,01 msec task-clock

data/measurements/output-base-1000-1024:             1,78 msec task-clock
data/measurements/output-pthread-1000-1024:          2,81 msec task-clock
data/measurements/output-uvpthread-1000-1024:        2,87 msec task-clock
data/measurements/output-uvpthreadcont-1000-1024:    2,87 msec task-clock

==============================================================================
Output of running tests on database with 100.000 rows of 1KB

data/measurements/output-base-100000-1:               39,16 msec task-clock   
data/measurements/output-pthread-100000-1:            1.306,64 msec task-clock 
data/measurements/output-uvpthread-100000-1:          1.669,49 msec task-clock 
data/measurements/output-uvpthreadcont-100000-1:      79,65 msec task-clock   

data/measurements/output-base-100000-4:               38,38 msec task-clock   
data/measurements/output-pthread-100000-4:            613,87 msec task-clock  
data/measurements/output-uvpthread-100000-4:          450,15 msec task-clock  
data/measurements/output-uvpthreadcont-100000-4:      76,74 msec task-clock   

data/measurements/output-base-100000-32:              38,21 msec task-clock   
data/measurements/output-pthread-100000-32:           106,25 msec task-clock  
data/measurements/output-uvpthread-100000-32:         93,04 msec task-clock   
data/measurements/output-uvpthreadcont-100000-32:     66,80 msec task-clock   

data/measurements/output-base-100000-128:             38,20 msec task-clock   
data/measurements/output-pthread-100000-128:          66,01 msec task-clock   
data/measurements/output-uvpthread-100000-128:        53,47 msec task-clock   
data/measurements/output-uvpthreadcont-100000-128     46,81 msec task-clock   

data/measurements/output-base-100000-1024:            37,70 msec task-clock   
data/measurements/output-pthread-100000-1024:         43,79 msec task-clock   
data/measurements/output-uvpthread-100000-1024:       43,07 msec task-clock   
data/measurements/output-uvpthreadcont-100000-1024    50,73 msec task-clock   

==============================================================================
Output of running tests on database with 1.000.000 rows of 1KB

data/measurements/output-base-1000000-1:              352,74 msec task-clock             
data/measurements/output-pthread-1000000-1:           13.576,76 msec task-clock          
data/measurements/output-uvpthread-1000000-1:         14.547,69 msec task-clock        
data/measurements/output-uvpthreadcont-1000000-1:     742,61 msec task-clock    

data/measurements/output-base-1000000-4:              364,46 msec task-clock             
data/measurements/output-pthread-1000000-4:           3.862,99 msec task-clock          
data/measurements/output-uvpthread-1000000-4:         4.059,26 msec task-clock        
data/measurements/output-uvpthreadcont-1000000-4:     713,30 msec task-clock    

data/measurements/output-base-1000000-32:             381,65 msec task-clock            
data/measurements/output-pthread-1000000-32:          817,66 msec task-clock         
data/measurements/output-uvpthread-1000000-32:        862,75 msec task-clock       
data/measurements/output-uvpthreadcont-1000000-32:    649,60 msec task-clock   

data/measurements/output-base-1000000-128:            364,95 msec task-clock           
data/measurements/output-pthread-1000000-128:         492,86 msec task-clock        
data/measurements/output-uvpthread-1000000-128:       512,24 msec task-clock      
data/measurements/output-uvpthreadcont-1000000-128:   460,03 msec task-clock  

data/measurements/output-base-1000000-1024:           355,45 msec task-clock          
data/measurements/output-pthread-1000000-1024:        393,58 msec task-clock       
data/measurements/output-uvpthread-1000000-1024:      398,66 msec task-clock     
data/measurements/output-uvpthreadcont-1000000-1024:  378,08 msec task-clock 
```
