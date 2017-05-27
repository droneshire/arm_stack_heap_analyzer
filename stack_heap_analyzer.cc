// This file is to calculate actual stack/heap size used in a user
// program.
//
// It hooks into init_array to initialize, and hooks into fini_array to
// calculate stack/heap usage. If no fini_array linkage, then add the 
// exit function as last line in main().
//
// Requires that the program not run indefinitely (must exit busy loop)
// so that the fini_array exit calls exit_check().
//
// This assumes following setup:
//     - stack at end of RAM and grows downward
//     - heap after data+bss and grows upward towards the stack
//     - end symbol must be defined just before the heap
//     - fini_array is compiled into project- if not, just put analyze_memory_on_exit
//       as last call before exiting main()
//

#include <stdlib.h>
#include <stdint.h>

// Define this with a function name so that you can set breakpoint to know if
// stack/heap overrun (function prototype: void Function(void))
// SET BREAKPOINT in this handler
#define OVERRUN_HANDLER Default_Handler

extern "C" void* _sbrk(int increment);
extern "C" void OVERRUN_HANDLER();

// defined in the startup_xxxx.s file as the end of RAM, also is bottom of the stack
extern int* _estack asm("_estack");
// defined by the linker script as the RAM location before the heap/stack starts
extern uint8_t end asm("end");
// stack pointer register
register int* sp asm("sp");

#define STACK_BASE (&_estack)

static constexpr int kMagicWord = 0xDEADBEEF;
// on 32-bit machine, memory aligned on 4 byte words
static constexpr int kIncrementWord = sizeof(uint32_t) - 1;
#define INCREMENT_TO_NEXT_ALIGNED_WORD(address) \
  reinterpret_cast<int*>(((uint32_t)(address) + kIncrementWord) & ~kIncrementWord)

extern "C" void analyze_memory_on_exit() {
  uint8_t* heap_start = &end;
  // sbrk() increments the program's data space by increment bytes. Calling sbrk() with an increment
  // of 0 can be used to find the current location of the program break.
  uint8_t* heap_end = reinterpret_cast<uint8_t*>(_sbrk(0));
  if (heap_end == reinterpret_cast<uint8_t*>(-1)) {
    Default_Handler();
  } else {
    volatile int heap_size = heap_end - heap_start;
    volatile int stack_size;

    int* stack_limit = INCREMENT_TO_NEXT_ALIGNED_WORD(heap_end);
    int* stack_base = reinterpret_cast<int*>(STACK_BASE);

    int max_stack_size = (stack_base - stack_limit) * sizeof(uint32_t);
    while (stack_limit < stack_base && *stack_limit == kMagicWord) {
      stack_limit++;
    }
    stack_size = (stack_base - stack_limit) * sizeof(uint32_t);
    if (stack_size >= max_stack_size) {
      Default_Handler();
    }
    // loop here indefinitely so that size can be examined by a debugger
    while (true) {
      // void cast to get around unused compiler warning
      // SET BREAKPOINT HERE
      (void)heap_size;
      (void)stack_size;
    }
  }
}

extern "C" void init_memory_analysis() {
  // paint magic words from the start of the heap all the way up to the start of the stack
  // (effectively all RAM after data + bss).
  // Use sbrk(0) instead of end symbol location here so that we don't overwrite
  // any potentially used data (as it appears there is heap allocation in play at this
  // point in initialization)
  uint8_t* heap_start = reinterpret_cast<uint8_t*>(_sbrk(0));
  int* magic_word_painter = INCREMENT_TO_NEXT_ALIGNED_WORD(heap_start);
  int* stack_base = sp;
  while (magic_word_painter < stack_base) {
    *magic_word_painter++ = kMagicWord;
  }
}

// Be sure to have "used" attribute, otherwise lto will optimize it away!!!
static void (*init_stack_check_p)()
    __attribute__((used, section(".init_array"))) = init_memory_analysis;
static void (*exit_check_p)()
__attribute__((used, section(".fini_array"))) = analyze_memory_on_exit;
