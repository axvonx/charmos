extern isr_common_entry
section .text

; Exceptions that push an error code: 8, 10-14, 17, 21, 29, 30
%macro ISR_STUB_ERR 1
global isr_vector_%1
isr_vector_%1:
    ; error code already on stack from CPU
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rsi
    push rdi
    push rbp
    push rdx
    push rcx
    push rbx
    push rax
    mov rdi, %1
    mov rsi, rsp
    call isr_common_entry
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rbp
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    add rsp, 8          ; skip error code
    iretq
%endmacro

%macro ISR_STUB_NO_ERR 1
global isr_vector_%1
isr_vector_%1:
    push 0               ; dummy error code
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rsi
    push rdi
    push rbp
    push rdx
    push rcx
    push rbx
    push rax
    mov rdi, %1
    mov rsi, rsp
    call isr_common_entry
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rbp
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    add rsp, 8          ; skip dummy error code
    iretq
%endmacro

; Vectors that push an error code
ISR_STUB_ERR 8
ISR_STUB_ERR 10
ISR_STUB_ERR 11
ISR_STUB_ERR 12
ISR_STUB_ERR 13
ISR_STUB_ERR 14
ISR_STUB_ERR 17
ISR_STUB_ERR 21
ISR_STUB_ERR 29
ISR_STUB_ERR 30

; Everything else
%assign i 0
%rep 256
  %if i != 8 && i != 10 && i != 11 && i != 12 && i != 13 && i != 14 && i != 17 && i != 21 && i != 29 && i != 30
    ISR_STUB_NO_ERR i
  %endif
  %assign i i+1
%endrep
