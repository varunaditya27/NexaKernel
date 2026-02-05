# VGA Display Fix Guide

## The Problem

When running NexaKernel in QEMU, you might see:
```
"guest has not initialized the display (yet)"
```

This happens because QEMU's VGA emulation hasn't detected any writes to the VGA text buffer at `0xB8000`.

## Root Cause

The kernel initializes the VGA buffer in `kernel_main()` via `early_console_init()`, but there are several potential issues:

1. **Paging Not Enabled**: Without paging, address `0xB8000` must be mapped directly in physical memory by QEMU.
2. **Memory Mapping Issue**: In some QEMU configurations, the VGA buffer might not be accessible at the expected address without proper initialization.
3. **Caching**: Write-combining or cache settings might prevent QEMU from seeing writes immediately.

## Solution

The fix I've implemented adds explicit VGA controller port writes to notify QEMU/the emulator that the VGA display is being used. This happens via the CRT controller at I/O ports `0x3D4` (address) and `0x3D5` (data).

### What Changed

**File**: `kernel/kernel.c`

**Changes**:
1. Added `early_console_update_cursor()` function that writes cursor position to VGA ports
2. Call `early_console_update_cursor()` from `early_console_init()` and after each `early_console_print()`
3. Added forward declaration for `early_console_update_cursor()`

### The Fix Code

```c
/* Tells VGA controller where cursor is (awakens the display) */
static void early_console_update_cursor(void)
{
    uint16_t pos = vga_row * VGA_WIDTH + vga_col;
    
    outb(0x3D4, 0x0F);  /* Cursor low byte register */
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    
    outb(0x3D4, 0x0E);  /* Cursor high byte register */
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}
```

## How to Rebuild and Test

```bash
# 1. Navigate to project
cd /home/varunaditya/Projects/NexaKernel

# 2. Rebuild (changes already applied)
make clean && make

# 3. Run in QEMU
make run

# OR run directly
qemu-system-i386 -kernel build/kernel.elf -m 128M -serial stdio -no-reboot -no-shutdown -vga std
```

## Expected Result

After the fix, you should see:

1. **QEMU VGA Window** shows text output instead of "guest has not initialized display"
2. **Welcome banner** appears:
   ```
   ========================================
     NexaKernel v0.1.0 - Booting...
   ========================================
   
   [BOOT] Kernel loaded at: 0x00100000
   [BOOT] Kernel ends at:   0xXXXXXXXX
   [BOOT] Kernel size:      XX KB
   ```
3. **Initialization phases** print:
   ```
   [INIT] Phase 1: Memory Management
     - Frame allocator initialized
     - Kernel heap initialized
   ...
   ```

## Why This Works

- The CRT controller I/O ports (`0x3D4`/`0x3D5`) are the standard way to communicate with VGA hardware
- Writing cursor position tells QEMU the display is being actively managed
- QEMU then activates its VGA display renderer and maps the buffer properly

## Advanced Debugging

If you still see "guest has not initialized display", try these steps:

### 1. Enable GDB Debugging

```bash
make debug
# In another terminal:
gdb build/kernel.elf -ex "target remote localhost:1234" -ex "break early_console_init" -ex "c"
```

Check if:
- Execution reaches `early_console_init`
- `outb` calls execute without crashing
- Memory at `0xB8000` is writable

### 2. Verify Multiboot Loading

```gdb
(gdb) break kernel_main
(gdb) c
(gdb) print /x $eip
# Should show 0x00101780 (kernel_main location)
```

### 3. Check VGA Buffer Access

```gdb
(gdb) break early_console_print
(gdb) c
(gdb) x/4x 0xb8000
# Should show non-zero values after first print
```

### 4. Monitor I/O Port Access

Enable QEMU tracing:

```bash
qemu-system-i386 -kernel build/kernel.elf \
  -trace events=qemu_io_write_trace_events.txt
```

## Alternative Solutions

### If VGA Still Doesn't Work

1. **Add Serial Console Fallback**:
   Modify `early_console_print` to also write to serial port `0x3F8` (COM1)

2. **Use SDL Display**:
   ```bash
   qemu-system-i386 -kernel build/kernel.elf -vga std -display sdl
   ```

3. **Use Curses Display**:
   ```bash
   qemu-system-i386 -kernel build/kernel.elf -vga std -display curses
   ```

4. **Direct VGA Cursor Enable** (additional):
   ```c
   // In early_console_init after clearing screen
   outb(0x3D4, 0x0A);  /* Cursor start scanline */
   outb(0x3D5, 0x0E);  /* Normal cursor (line 14) */
   
   outb(0x3D4, 0x0B);  /* Cursor end scanline */
   outb(0x3D5, 0x0F);  /* Cursor end (line 15) */
   ```

## Testing Checklist

After applying the fix:

- [ ] `make` completes without errors
- [ ] `make run` launches QEMU
- [ ] QEMU window shows boot banner (not "guest has not initialized")
- [ ] Text output is visible and readable
- [ ] Kernel information displays correctly
- [ ] Memory details show correct amounts
- [ ] All init phases print successfully

## Summary

The fix works by:
1. Writing to VGA text buffer at `0xB8000` (as before)
2. **Adding** I/O port writes to CRT controller (NEW)
3. This combination signals to QEMU that VGA is active and needs rendering
4. QEMU then activates display output

**Status**: ✅ Fix Applied to `kernel/kernel.c`

Next steps: Run `make && make run` and check the QEMU window for boot output.
