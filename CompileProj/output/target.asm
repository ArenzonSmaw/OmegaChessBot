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
    sub sp, 4
    lea ax, [_str_0]
    mov dx, ax
    call print_string
    mov [_UNDERLINE], ax

.DATA
_size dw 0
.CODE
    call scan_int
    mov [_size], ax
    mov ax, 0 ;i
    mov [bp - 2], ax
_lbl_0:
    mov ax, [bp - 2]
    push ax
    mov ax, [_size]
    mov bx, ax
    pop ax
    cmp ax, bx
    jl _skip_0
    jmp near ptr _lbl_1
_skip_0:
    mov ax, [_size]
    push ax
    mov ax, [bp - 2]
    mov bx, ax
    pop ax
    sub ax, bx ;space = size-i
    mov [_UNDERLINE], ax
    mov [bp - 4], ax
    mov ax, 0
    mov [bp - 2], ax
_lbl_2:
    mov ax, [bp - 2] ;j 
    push ax
    mov ax, [bp - 4] ; space
    mov bx, ax
    pop ax
    cmp ax, bx
    jl _skip_2
    jmp near ptr _lbl_3
_skip_2:
    lea ax, [_str_1]
    mov dx, ax
    call print_string
    mov [_UNDERLINE], ax
_step_2:
    mov ax, [bp - 2]
    push ax
    mov ax, 1
    mov bx, ax
    pop ax
    add ax, bx
    mov [_UNDERLINE], ax
    mov [bp - 2], ax
    mov [_UNDERLINE], ax
    jmp _lbl_2
_lbl_3:
    mov ax, 0
    mov [bp - 2], ax
_lbl_4:
    mov ax, [bp - 2]
    push ax
    mov ax, 1
    push ax
    mov ax, 2
    push ax
    mov ax, [bp - 2]
    mov bx, ax
    pop ax
    imul bx
    mov [_UNDERLINE], ax
    mov bx, ax
    pop ax
    add ax, bx
    mov [_UNDERLINE], ax
    mov bx, ax
    pop ax
    cmp ax, bx
    jl _skip_4
    jmp near ptr _lbl_5
_skip_4:
    lea ax, [_str_2]
    mov dx, ax
    call print_string
    mov [_UNDERLINE], ax
_step_4:
    mov ax, [bp - 2]
    push ax
    mov ax, 1
    mov bx, ax
    pop ax
    add ax, bx
    mov [_UNDERLINE], ax
    mov [bp - 2], ax
    mov [_UNDERLINE], ax
    jmp _lbl_4
_lbl_5:
    mov ax, '\'
    mov dl, al
    call print_char
    mov [_UNDERLINE], ax
_step_0:
    mov ax, [bp - 2]
    push ax
    mov ax, 1
    mov bx, ax
    pop ax
    add ax, bx
    mov [_UNDERLINE], ax
    mov [bp - 2], ax
    mov [_UNDERLINE], ax
    jmp _lbl_0
_lbl_1:
    mov dl, 0Ah
    call print_char
    mov ax, 4C00h
    int 21h
    ret
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


.DATA
_str_0 db "enter triangle size: ", '$'
_str_1 db " ", '$'
_str_2 db "*", '$'
END MAIN
