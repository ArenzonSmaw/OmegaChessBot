.MODEL SMALL
.STACK 100h
.DATA
; --- global variables ---
_UNDERLINE dw 0
_UNDERLINE_hi dw 0
; --- code section ---
.CODE
MAIN PROC
    mov ax, DGROUP
    mov ds, ax
    mov es, ax

.DATA
_x dw 0
.CODE
    mov ax, 12
    mov [_x], ax

.DATA
_y dw 0
_y_hi dw 0
.CODE
    mov ax, 5 
mov bx, 46
    mov [_y], ax
    mov [_y_hi], bx
    mov ax, [_x]
    push ax
    mov ax, [_y]
    mov bx, [_y_hi]
    mov bx, ax
    pop ax
    imul bx
    push ax
    mov ax, 50
    mov bx, ax
    pop ax
    cmp ax, bx
    mov ax, 0
    je  _true
    mov ax, 0
    jmp _done
_true :
    mov ax, 1
_done :
    cmp ax, 0
    je _lbl_0 
    mov ax, 1
    call print_int
    mov [_UNDERLINE], ax
    jmp _lbl_1

_lbl_0:
    mov ax, 2
    call print_int
    mov [_UNDERLINE], ax
_lbl_1:
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
        ret
    print_rational endp

    print_float proc
        call print_int
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
        ret
    print_float endp

scan_int PROC
    mov bx, 0
_scan_int:
    mov ah, 01h
    int 21h
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

END MAIN
