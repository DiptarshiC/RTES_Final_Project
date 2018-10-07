# RTES_Final_Project
This is the repository for my RTES Final project. The system interfaces a logitech c200 camera with 
an ARM a53 processor. The system employs Rate monotonic scheduling policy in which the task with the highest
frequency is given the highest priority. There are four threads :- capture,scan and save and lastly a sequencer
thread that schedules all the others. The two files capture.cpp and capture1.cpp are simply two approaches of the
same task.
