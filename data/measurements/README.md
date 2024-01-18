Preleminary conclusions:

- Batch size has large influence on running time of stepping through the whole query, the bigger the batch, the better.
- `uvpthreadcont` running time is close to `base` case.

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
