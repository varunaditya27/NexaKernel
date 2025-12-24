; ===========================================================================
; kernel/scheduler/context_switch.asm
; ===========================================================================
;
; Low-Level Context Switch Routine
;
; This file contains the assembly code responsible for saving the state of the
; currently running task (registers, stack pointer) and restoring the state of
; the next task to be run.
;
; Context switching is the core mechanism that enables multitasking. It:
; 1. Saves the current task's CPU state (registers) to its stack
; 2. Saves the current stack pointer to the task's TCB
; 3. Loads the new task's stack pointer from its TCB
; 4. Restores the new task's CPU state from its stack
; 5. Resumes execution of the new task
;
; Stack Layout After Save:
; ┌─────────────────────────────────────────────────────────────────────────┐
; │  [Higher addresses]                                                     │
; │  ┌─────────────────────┐                                               │
; │  │  Return address     │  <- Where to resume (EIP)                     │
; │  │  EAX                │                                               │
; │  │  ECX                │                                               │
; │  │  EDX                │                                               │
; │  │  EBX                │                                               │
; │  │  ESP (ignored)      │  <- Original ESP from PUSHA (not used)        │
; │  │  EBP                │                                               │
; │  │  ESI                │                                               │
; │  │  EDI                │  <- task->stack_pointer points here           │
; │  └─────────────────────┘                                               │
; │  [Lower addresses]                                                      │
; └─────────────────────────────────────────────────────────────────────────┘
;
; ===========================================================================

[BITS 32]
section .text

; ---------------------------------------------------------------------------
; External C Functions
; ---------------------------------------------------------------------------
extern task_set_current         ; void task_set_current(task_t *task)

; ---------------------------------------------------------------------------
; Global Symbols (callable from C)
; ---------------------------------------------------------------------------
global context_switch           ; void context_switch(task_t **old_task_sp, task_t *new_task)
global switch_to_task           ; void switch_to_task(task_t *new_task)
global get_stack_pointer        ; uint32_t get_stack_pointer(void)

; ---------------------------------------------------------------------------
; context_switch - Switch from one task to another
; ---------------------------------------------------------------------------
;
; C Prototype:
;   void context_switch(uint32_t **old_sp_ptr, uint32_t *new_sp)
;
; Parameters (on stack due to cdecl calling convention):
;   [ESP+4]  = old_sp_ptr : Pointer to where current task's SP should be saved
;   [ESP+8]  = new_sp     : Stack pointer of the task to switch to
;
; This function:
; 1. Saves all general-purpose registers (via PUSHA)
; 2. Saves current ESP to *old_sp_ptr
; 3. Loads ESP from new_sp
; 4. Restores all general-purpose registers (via POPA)
; 5. Returns to the new task (RET pops EIP from new task's stack)
;
; ---------------------------------------------------------------------------
context_switch:
    ; =======================================================================
    ; Step 1: Save current task's state
    ; =======================================================================
    
    ; Save all general-purpose registers
    ; PUSHA pushes: EAX, ECX, EDX, EBX, original ESP, EBP, ESI, EDI
    pusha
    
    ; Now stack looks like:
    ;   [ESP+36] = return address (pushed by CALL instruction)
    ;   [ESP+32] = old_sp_ptr (first parameter)
    ;   [ESP+28] = new_sp (second parameter)
    ;   [ESP+0 to ESP+28] = saved registers from PUSHA
    ;
    ; Wait - we need to adjust for the PUSHA!
    ; After PUSHA, parameters are at:
    ;   [ESP+36] = old_sp_ptr
    ;   [ESP+40] = new_sp
    
    ; Get pointer to where we should save current ESP
    mov eax, [esp + 36]         ; eax = old_sp_ptr
    
    ; Save current stack pointer (after PUSHA)
    ; This is where the saved context starts
    mov [eax], esp
    
    ; =======================================================================
    ; Step 2: Load new task's state
    ; =======================================================================
    
    ; Load new stack pointer
    mov esp, [esp + 40]         ; esp = new_sp
                                ; NOTE: This changes our stack!
    
    ; Restore all general-purpose registers from new task's stack
    ; POPA pops: EDI, ESI, EBP, (skip ESP), EBX, EDX, ECX, EAX
    popa
    
    ; Return to new task
    ; RET pops EIP from the stack and jumps there
    ret


; ---------------------------------------------------------------------------
; switch_to_task - Switch to a new task (first time or after creation)
; ---------------------------------------------------------------------------
;
; C Prototype:
;   void switch_to_task(task_t *new_task)
;
; Parameters:
;   [ESP+4] = new_task : Pointer to the task to switch to
;
; This function is used when we don't have a current task to save
; (e.g., starting the scheduler for the first time).
;
; The new task's stack_pointer field contains the initial stack pointer
; set up by task_create().
;
; ---------------------------------------------------------------------------
switch_to_task:
    ; Get the new task pointer
    mov eax, [esp + 4]          ; eax = new_task
    
    ; Update current_task in the task module
    push eax
    call task_set_current
    add esp, 4
    
    ; Get the task again (in case it was clobbered)
    mov eax, [esp + 4]
    
    ; Load the task's stack pointer
    ; task_t structure has stack_pointer as first field (offset 0)
    mov esp, [eax]              ; esp = new_task->stack_pointer
    
    ; Restore registers from the task's stack
    popa
    
    ; Jump to the task's entry point
    ret


; ---------------------------------------------------------------------------
; get_stack_pointer - Get current stack pointer (for debugging)
; ---------------------------------------------------------------------------
;
; C Prototype:
;   uint32_t get_stack_pointer(void)
;
; Returns:
;   Current value of ESP
;
; ---------------------------------------------------------------------------
get_stack_pointer:
    mov eax, esp
    ret


; ---------------------------------------------------------------------------
; task_switch_asm - Complete task switch with C integration
; ---------------------------------------------------------------------------
;
; C Prototype:
;   void task_switch_asm(task_t *old_task, task_t *new_task)
;
; Parameters:
;   [ESP+4] = old_task : Current task (to save state to)
;   [ESP+8] = new_task : Next task (to restore state from)
;
; This is a more complete version that:
; 1. Saves old task's full context
; 2. Updates current_task pointer
; 3. Restores new task's full context
;
; ---------------------------------------------------------------------------
global task_switch_asm
task_switch_asm:
    ; =======================================================================
    ; Step 1: Save current (old) task's context
    ; =======================================================================
    
    ; Save all general-purpose registers
    pusha
    
    ; Get old_task parameter (adjusted for PUSHA)
    mov eax, [esp + 36]         ; eax = old_task
    
    ; Check if old_task is NULL (first time scheduling)
    test eax, eax
    jz .load_new_task
    
    ; Save stack pointer to old_task->stack_pointer
    ; stack_pointer is at offset 0 in task_t
    mov [eax], esp
    
.load_new_task:
    ; =======================================================================
    ; Step 2: Load new task's context
    ; =======================================================================
    
    ; Get new_task parameter
    ; After PUSHA, parameters moved up by 32 bytes
    mov eax, [esp + 40]         ; eax = new_task
    
    ; Update current_task
    push eax
    call task_set_current
    add esp, 4
    
    ; Reload new_task (might have been clobbered by function call)
    mov eax, [esp + 40]
    
    ; Load stack pointer from new_task->stack_pointer
    mov esp, [eax]
    
    ; Restore registers from new task's stack
    popa
    
    ; Return to new task
    ret
