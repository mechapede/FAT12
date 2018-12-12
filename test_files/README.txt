*********************Readme.txt for MFS**********************************
Assignment P2 - Multi-Flow Scheduler
CSc 360 
David Audet V00150102
November 13th, 2009
*************************************************************************

Design

Following my original design in my design document submitted
October 30th 2009, I used a monitor data structure to aid
in making debugging of the program much simpler.
The monitor has two functions: request_interface and release_interface.
Using the priciples of a monitor, primarily the fact that only one
thread can be active at one time, I was able to schedule the flows as
required by the specifications.

Implementation

This program introduced the idea of simulating flow transmission on
a router like device. To accomplish this simulation I had to use the functions
usleep and gettimeofday to simulate arriving flows and elapsed time of execution.
After parsing the input file containing the flow information, I created a thread
for each flow in the file and populated a flow struct with all the information
relevant for each flow.

Each flow(thread) first called usleep for the amount of time until the arrival time
had elapsed. Once the flow had arrived it attempted to enter the monitor and gain access to the transmission 
interface. If the interface is unavailable then the flows are put into a waiting
array where they will wait on a conditional variable. Sorting this array first
by priority and then by subsequent rules, I was able to schedule their transmission
on the interface. Once the flow satisfied these conditions it was able to transmit (calling usleep
for an amount of time equal to transmission time) and then release the interface to the
other waiting flows.

References

http://www.csce.uark.edu/~aapon/courses/os/examples/
http://www.opengroup.org/onlinepubs/007908799/xsh/pthread.h.html
http://www.opengroup.org/onlinepubs/007908799/

