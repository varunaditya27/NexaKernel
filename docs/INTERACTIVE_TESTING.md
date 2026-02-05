# NexaKernel Interactive Testing Guide

## Current Status
✅ Kernel boots successfully  
✅ VGA display working  
✅ Memory management initialized  
✅ Interrupt system initialized  
✅ Scheduler running with demo tasks  

## You Are Here
In the **interactive scheduler mode** - the kernel is running and accepting keyboard input.

## What to Test

### 1. Scheduler Test
Press **'s'** in the QEMU window to see:
```
=== Scheduler Statistics ===
Context switches: X
Schedule calls: X
Ready tasks: 5
Idle time: X ticks
Active tasks: 5
============================
```

**What this tells you:**
- `Context switches` > 0 = Scheduler is working
- `Ready tasks: 5` = Main + 3 demo tasks + 1 idle task
- `Active tasks: 5` = All tasks are alive

### 2. Keyboard Input Test
Try pressing various keys (a-z, 0-9, Enter, etc.)

**Expected behavior:**
```
Key: 'a'
Key: 'b'
...
```

**What this tells you:**
- Keyboard driver is working
- Interrupt handling (IRQ1) is functional
- Task scheduling is responsive to input

### 3. Timer/Uptime Test
Wait and watch for periodic status updates:
```
[5s] System running... (5 tasks)
[10s] System running... (5 tasks)
...
```

**What this tells you:**
- Timer interrupt (IRQ0) is firing
- Scheduler is respecting time slices
- System time is advancing

### 4. Task Switching Test
You should see the demo tasks running. The main task prints:
```
[Xs] System running... (5 tasks)
```

The `[Xs]` indicates system uptime. Tasks are switching based on:
- **Time slice expiration** (if round-robin enabled)
- **Task yields** (if tasks call `task_yield()`)
- **Priority** (if priority scheduling enabled)

## Full Testing Checklist

### Boot Phase Tests
- [x] Kernel loads at 0x00100000
- [x] Multiboot info detected
- [x] Memory info displayed (640KB lower, XXXMB upper)

### Initialization Phase Tests
- [x] **Phase 1: Memory Management**
  - Frame allocator initialized
  - Kernel heap initialized
  - Memory allocations test passed

- [x] **Phase 2: Interrupt Handling**
  - IDT initialized
  - ISR handlers installed
  - IRQ handlers installed
  - PIC remapped to vectors 32-47

- [x] **Phase 3: Device Drivers**
  - VGA text mode initialized
  - PIT timer initialized
  - PS/2 keyboard initialized

- [x] **Phase 4: Scheduler**
  - Task system initialized
  - Scheduler initialized
  - Demo tasks created

### Runtime Tests
- [ ] **Test Scheduler (press 's')**
  - [ ] Statistics display without crashing
  - [ ] Context switches > 0
  - [ ] Ready tasks = 5
  
- [ ] **Test Keyboard (press keys a-z)**
  - [ ] Keys are echoed back
  - [ ] No hang or crash on input
  
- [ ] **Test Timer (wait 10+ seconds)**
  - [ ] Uptime updates every 5 seconds
  - [ ] Uptime value increases correctly

- [ ] **Test Multitasking (let it run 30+ seconds)**
  - [ ] No crashes or hangs
  - [ ] Tasks continue running
  - [ ] System remains responsive

## How to Exit

To exit QEMU cleanly:

1. In the QEMU window, press: **Ctrl+A** then **X**
2. Or close the QEMU window

The kernel should halt gracefully.

## Troubleshooting

### If You See a Hang
1. Check if the cursor is blinking (kernel may be running, just not printing)
2. Try pressing 's' to trigger scheduler stats output
3. Try pressing a key to trigger keyboard input
4. If completely hung, press Ctrl+C in terminal to kill QEMU

### If Text is Corrupted
1. The kernel may be scrolling too fast
2. Try pressing keys to slow down output
3. Rebuild with: `make clean && make`

### If You Get a Crash
1. Exit QEMU (Ctrl+A, X)
2. Run with debugging: `make debug`
3. In another terminal: `gdb build/kernel.elf -ex "target remote localhost:1234"`
4. Set breakpoint and investigate

## Next Steps

After confirming everything works:

1. **Extended Testing**
   - Let the kernel run for several minutes
   - Verify no memory leaks (heap stats remain stable)
   - Confirm scheduler statistics show progress

2. **Performance Monitoring**
   - Check context switch rate (should be > 100 per second)
   - Monitor idle time (should be reasonable)
   - Verify task switch frequency

3. **Stress Testing**
   - Create more tasks via command line
   - Test with many keyboard inputs
   - Let it run overnight to catch edge cases

4. **Code Review**
   - Check for any WARNING or error messages
   - Review scheduler implementation
   - Verify interrupt handlers are working correctly

## Example Session

```bash
# Start kernel
make run

# In QEMU window:
[Wait for boot to complete]
[Press 's' to see scheduler stats]
[Type some keys to test keyboard]
[Watch uptime updates]
[Let it run for 30+ seconds to confirm stability]
[Press Ctrl+A, X to exit QEMU]
```

## Expected Output During Run

```
[SCHED] Entering scheduler...
[Main] Interactive mode - press keys!
[Main] Press 's' for scheduler stats
[5s] System running... (5 tasks)
Key: 'a'
[10s] System running... (5 tasks)
Key: 'b'
[15s] System running... (5 tasks)
```

## Success Criteria

✅ System boots without errors  
✅ Scheduler runs demo tasks  
✅ Keyboard input is handled  
✅ Timer interrupts work (uptime updates)  
✅ No crashes for 30+ seconds of operation  

---

**Status**: Your kernel is working! This is the normal operational state.

Next: Test the features listed above and verify long-term stability.
