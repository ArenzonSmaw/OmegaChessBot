.MODEL SMALL
.STACK 100h
.DATA
; --- global variables ---
true db 'true$'
false db 'false$'
_UNDERLINE dw 0
_UNDERLINE_hi dw 0
; --- code section ---
.CODE
MAIN PROC
    mov ax, DGROUP
    mov ds, ax
    mov es, ax
    mov bp, sp
    sub sp, 10
    mov ah, 1
    int 21h

.DATA
_f1 dw 0
_f1_hi dw 0
.CODE
    mov ax, 3
    mov bx, 2
    div bx
    push ax
    push bx
    mov ax, dx
    mov bx, 100
    mul bx
    pop bx
    xor dx, dx
    div bx
    mov bx, ax
    pop ax
    mov [_f1], ax
    mov [_f1_hi], bx

.DATA
_f2 dw 0
_f2_hi dw 0
.CODE
    mov ax, 2
    mov bx, 97
    mov [_f2], ax
    mov [_f2_hi], bx

.DATA
_m dw 0
.CODE
    mov ax, 2
    mov [_m], ax
    mov ax, [_f1]
    mov bx, [_f1_hi]
    call print_float
    mov [_UNDERLINE], ax
    mov ax, [_f1]
    mov bx, [_f1_hi]
    push ax
    push bx
    mov ax, [_m]
    mov cx, ax
    pop bx
    pop ax
    call f_mul
    mov [_UNDERLINE], ax
    call print_float
    mov [_UNDERLINE], ax
    mov ax, [_f2]
    mov bx, [_f2_hi]
    call print_float
    mov [_UNDERLINE], ax
    mov ax, [_f1]
    mov bx, [_f1_hi]
    push ax
    push bx
    mov ax, [_m]
    mov cx, ax
    pop bx
    pop ax
    call f_mul
    mov [_UNDERLINE], ax
    push ax
    push bx
    mov ax, [_f2]
    mov bx, [_f2_hi]
    mov cx, ax
    mov dx, bx
    pop bx
    pop ax
    call f_cmp
    jng _grtr_false_0
    mov ax, 1
    jmp _grtr_end_0
_grtr_false_0:
    mov ax, 0
_grtr_end_0:
    mov [_UNDERLINE], ax
    call print_bool
    mov [_UNDERLINE], ax
    add sp, 10
    mov dl, 0Ah
    call print_char
    mov ax, 4C00h
    int 21h
MAIN ENDP

print_int proc
    cmp ax, 0
    jge pi_positive
    push ax
    mov dl, '-'
    mov ah, 02h
    int 21h
    pop ax
    neg ax
pi_positive:
    mov cx, 0
    mov bx, 10
pi_divide_loop:
    mov dx, 0
    div bx
    push dx
    inc cx
    cmp ax, 0
    jne pi_divide_loop
pi_print_loop:
    pop dx
    add dl, '0'
    mov ah, 02h
    int 21h
    loop pi_print_loop
    ret
print_int endp

print_char proc
    cmp al, '\'
    jne _print_char
    mov dl, 0Ah
_print_char:
    mov ah, 02h
    int 21h
    ret
print_char endp

print_string proc
    mov ah, 09h
    int 21h
    ret
print_string endp

print_rational proc
    push bx
    call print_int
    pop bx
    push bx
    mov dl, '/'
    mov ah, 02h
    int 21h
    pop ax
    call print_int
    mov dl, 0Ah
    mov ah, 02h
    int 21h
    ret
print_rational endp

print_float proc
    push bx
    call print_int
    pop bx
    mov dl, '.'
    mov ah, 02h
    int 21h
    mov ax, bx
    cmp ax, 10
    jge pf_print_frac
    mov dl, '0'
    mov ah, 02h
    int 21h
    mov ax, bx
pf_print_frac:
    call print_int
    mov dl, 0Ah
    mov ah, 02h
    int 21h
    ret
print_float endp

print_bool proc
    cmp al, 0
    je print_bool_false
    lea dx, true
    jmp _print_bool
print_bool_false:
    lea dx, false
_print_bool:
    mov ah, 09h
    int 21h
    mov dl, 0Ah
    mov ah, 02h
    int 21h
    ret
print_bool endp

scan_int PROC
    mov bx, 0
    mov dx, 0
    mov ah, 01h
    int 21h
    cmp al, '-'
    jne _scan_int_cont_1
    mov dx, 1
_scan_int_cont_1:
    push dx
    jne _scan_int_cont_2
_scan_int:
    mov ah, 01h
    int 21h
_scan_int_cont_2:
    cmp al, 0Dh
    je _scan_int_done
    sub al, '0'
    cbw
    xchg ax, bx
    mov cx, 10
    mul cx
    add ax, bx
    mov bx, ax
    jmp _scan_int
_scan_int_done:
    mov ax, bx
    pop dx
    cmp dx, 0
    je _scan_int_ret
    neg ax
_scan_int_ret:
    ret
scan_int ENDP

scan_float PROC
    mov bx, 0
_scan_ipart:
    mov ah, 01h
    int 21h
    cmp al, 0Dh
    je _scan_ipart_end
    cmp al, '.'
    je _scan_fpart_start
    sub al, '0'
    cbw
    xchg ax, bx
    mov cx, 10
    mul cx
    add ax, bx
    mov bx, ax
    jmp _scan_ipart
_scan_fpart_start:
    push bx
_scan_fpart:
    mov ah, 01h
    int 21h
    cmp al, 0Dh
    je _scan_fpart_end
    sub al, '0'
    cbw
    xchg ax, bx
    mov cx, 10
    mul cx
    add ax, bx
    mov bx, ax
    jmp _scan_fpart
_scan_fpart_end:
    mov bx, ax
    pop ax
    jmp _scan_float_done
_scan_ipart_end:
    mov ax, bx
_scan_float_done:
    ret
scan_float ENDP

scan_rational PROC
    mov bx, 0
_scan_npart:
    mov ah, 01h
    int 21h
    cmp al, 0Dh
    je _scan_npart_end
    cmp al, '/'
    je _scan_dpart_start
    sub al, '0'
    cbw
    xchg ax, bx
    mov cx, 10
    mul cx
    add ax, bx
    mov bx, ax
    jmp _scan_npart
_scan_dpart_start:
    push bx
_scan_dpart:
    mov ah, 01h
    int 21h
    cmp al, 0Dh
    je _scan_dpart_end
    sub al, '0'
    cbw
    xchg ax, bx
    mov cx, 10
    mul cx
    add ax, bx
    mov bx, ax
    jmp _scan_dpart
_scan_dpart_end:
    mov bx, ax
    pop ax
    jmp _scan_ratnal_done
_scan_npart_end:
    mov ax, bx
    mov bx, 0
_scan_ratnal_done:
    ret
scan_rational ENDP

scan_bool PROC
    mov ah, 1h
    int 21h
    cmp al, 'y'
    je scan_bool_true
    cmp al, 'Y'
    je scan_bool_true
    cmp al, 't'
    je scan_bool_true
    cmp al, 'T'
    je scan_bool_true
    jmp scan_bool_true
    mov ax, 0
    jmp scan_bool_finish
scan_bool_true:
    mov ax, 1
scan_bool_finish:
    ret
scan_bool ENDP

f_add PROC
    add bx, dx
    cmp bx, 100
    jl _fadd_no_carry
    sub bx, 100
    inc ax
_fadd_no_carry:
    add ax, cx
    ret
f_add ENDP

f_sub PROC
    sub bx, dx
    cmp bx, 0
    jge _fsub_no_borrow
    add bx, 100
    dec ax
_fsub_no_borrow:
    sub ax, cx
    ret
f_sub ENDP

f_mul PROC
    push ax
    mov ax, bx
    mul cx
    push cx
    mov cx, 100
    div cx
    xchg ax, dx
    pop cx
    mov bx, ax
    pop ax
    push dx
    imul cx
    pop dx
    add ax, dx
    ret
f_mul ENDP

f_div PROC
    mov dx, 0
    idiv cx
    push ax
    mov ax, bx
    add ax, dx
    div cx
    mov bx, ax
    pop ax
    ret
f_div ENDP

f_cmp PROC
    cmp ax, cx
    jne _fcmp_done
    cmp bx, dx
_fcmp_done:
    ret
f_cmp ENDP

END MAIN
