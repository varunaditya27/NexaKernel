# Keyboard Not Working - Debugging Guide

## Problem
Keyboard input is not being registered in the NexaKernel emulator.

## Diagnosis Steps

### Step 1: Check IRQ1 Statistics
1. Build and run the kernel
2. Press 'i' immediately after boot
3. Check the output for `IRQ1 (Keyboard):`

**Expected**: Shows a count like `IRQ1 (Keyboard): 5` (any number > 0)
**Problem**: Shows `IRQ1 (Keyboard): 0` (never increments)

### Step 2: Test Timer First
1. Press 's' to see scheduler statistics
2. Wait for the `[Xs] System running...` message to appear
3. This tests if IRQ0 (timer) is working

**Expected**: Timer fires every second, updates print regularly
**Problem**: No uptime updates (IRQ0 not working either - bigger issue)

### Step 3: Monitor IRQ1 Fires
1. Press 'i' to see initial count
2. Wait 5-10 seconds
3. Try pressing a keyboard key slowly
4. Press 'i' again immediately
5. Check if the count changed

**Expected**: Count increases when keys are pressed
**Problem**: Count stays 0 even after pressing keys

## Root Cause Analysis

### Possible Issue #1: Keyboard Driver Not Initialized
**Symptom**: IRQ1 count is always 0, even after key press
**Solution**:
```bash
# Check the boot output:
# - Should show "[INIT] Phase 3: Device Drivers"
# - Should show "- Keyboard: OK (PS/2)"
# If missing or shows FAILED, keyboard_init() failed
```

### Possible Issue #2: IRQ1 Not Enabled in PIC
**Symptom**: Driver initializes but interrupts never fire
**Fix Applied**: Modified keyboard_init() to properly call `irq_enable(IRQ1_KEYBOARD)`

Check that keyboard_init() calls:
```c
irq_register_handler(IRQ1_KEYBOARD, keyboard_irq_handler);
irq_enable(IRQ1_KEYBOARD);
```

### Possible Issue #3: QEMU Not Sending Keyboard Events
**Symptom**: IRQ1 never fires even with all other components working
**Solution**: QEMU keyboard might not be working in nographic mode

Try:
```bash
# Instead of nographic, use VGA display
qemu-system-i386 -kernel build/kernel.elf -vga std

# Or try curses display
qemu-system-i386 -kernel build/kernel.elf -display curses

# Or try SDL display
qemu-system-i386 -kernel build/kernel.elf -display sdl
```

### Possible Issue #4: PS/2 Controller Not Ready
**Symptom**: Keyboard initialized but no interrupts
**Fix in Code**: Added keyboard_wait_input/output and status checks

The keyboard driver should:
1. Wait for controller to be ready
2. Flush any pending data
3. Set LEDs (proves communication)
4. Register handler

### Possible Issue #5: Scancode Not Read from Port
**Symptom**: IRQ fires but characters never appear in buffer
**Check**: The IRQ handler reads from port 0x60:
```c
uint8_t scancode = inb(KB_DATA_PORT);  // KB_DATA_PORT = 0x60
process_scancode(scancode);
```

## Testing Procedure

### Quick Test
```bash
make clean && make
qemu-system-i386 -kernel build/kernel.elf -m 128M -vga std

# In QEMU window:
# 1. Wait for boot (5-10 seconds)
# 2. Press 'i' to see interrupts
#    Expected: "IRQ1 (Keyboard): 0"
# 3. Press a key (e.g., 'a')
# 4. Immediately press 'i' again
#    Expected: "IRQ1 (Keyboard): 1"  (count increased by 1)
```

### Extended Test
```bash
# Press these in order:
i     # Check IRQ1 = 0
wait  # 2 seconds
a b c d e f   # Press 6 keys
i     # Check IRQ1 = 6 (or close)
```

##Fixes Applied

### Fix 1: Keyboard Init Completion Check
File: `kernel/drivers/keyboard.c`
- Ensures `keyboard_init()` properly registers and enables IRQ1
- Flushes keyboard buffer before starting
- Sets initial LED state

### Fix 2: Interrupt Statistics Exposure
File: `kernel/kernel.c` & `kernel/interrupts/interrupts.h`
- Added `irq_get_count(irq)` to expose IRQ fire counts
- Updated main task to show interrupt stats with 'i' command
- Allows diagnosis of which IRQs are actually firing

### Fix 3: Debug Output
File: `kernel/kernel.c`
- Main task now prints message about 'i' command
- Displays IRQ0 and IRQ1 counts when requested
- Shows current uptime for timing reference

## If Keyboard Still Doesn't Work

### Option A: Add Serial Debug Output
Modify `keyboard_irq_handler()` to send bytes to serial port 0x3F8:
```c
static void keyboard_irq_handler(interrupt_frame_t *frame)
{
    uint8_t scancode = inb(KB_DATA_PORT);
    // Send 'K' to serial to prove handler fired
    outb(0x3F8, 'K');
    process_scancode(scancode);
}
```

### Option B: Check QEMU Version
```bash
qemu-system-i386 --version
# Requires QEMU 6.0 or newer for reliable keyboard
```

### Option C: Try Mouse Support
If only keyboard fails but other interrupts work, the PS/2 controller might have an issue.
Test mouse support (IRQ12) to narrow down the problem.

### Option D: Disable Optimization
Some QEMU builds have issues with optimized I/O:
```bash
qemu-system-i386 -kernel build/kernel.elf \
  -m 128M \
  -vga std \
  -device i8042,aux=off
```

## Summary

After the fixes in this commit:

✅ **Fixed**: Interrupt statistics now visible (press 'i')
✅ **Fixed**: IRQ1 count displays correctly
✅ **Fixed**: Can diagnose keyboard without GDB

Next Steps:
1. Run the kernel with `make run`
2. Press 'i' to check IRQ counts
3. Press a key and immediately press 'i' again
4. Compare counts - if they increased, keyboard IS working!
5. If character appears, keyboard is FULLY working!

If IRQ1 count never increases even after key presses:
- The PS/2 controller or QEMU keyboard emulation isn't working
- Try a different QEMU display mode (-display sdl)
- Check QEMU version (needs 6.0+)
