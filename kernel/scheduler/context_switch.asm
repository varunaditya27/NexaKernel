; context_switch.asm
;
; Assembly-level context switch helpers. Provide labels that the C scheduler can
; call to save the current CPU state and load a new task state.
;
; Keep the ABI minimal; prefer a single function `context_switch(old_sp, new_sp)`
; or separate `save_context` / `restore_context` functions.
