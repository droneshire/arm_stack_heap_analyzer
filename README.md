# Stack/Heap Analyzer (tested on stm32 arm)
Tested on stm32 arm, but could easily be added to other GCC based platforms. 

This file is to calculate actual stack/heap size used in a user program.  It paints the memory space of the heap+stack with a unique magic word at the beginning of initialization by linking into the init_array section.  The application then runs, and on exit, an exit check calculates the heap size, and checks the max stack size by viewing memory left between stack and heap that still has the magic number in it.  If these regions ever intersected (and didn't already crash the program), it will jump to a specified fault handler (ideally an infinite loop) so that it can be seen using a debugger.

## Use

Use a debugger and set breakpoints in the OVERRUN_HANDLER and at the end of the exit_check() function.  Then can examine the value of heap_size and stack_size.  If an overrun happened, you'll hit the break set in the OVERRUN_HANDLER.
