%macro x86_EnterRealMode 0
    [bits 32]
    jmp word 18h:.pmode16         ; 1 - jump to 16-bit protected mode segment

.pmode16:
    [bits 16]
    ; 2 - disable protected mode bit in cr0
    mov eax, cr0
    and al, ~1
    mov cr0, eax

    ; 3 - jump to real mode
    jmp word 00h:.rmode

.rmode:
    ; 4 - setup segments
    mov ax, 0
    mov ds, ax
    mov ss, ax

    ; 5 - enable interrupts
    sti

%endmacro


%macro x86_EnterProtectedMode 0
    cli

    ; 4 - set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 5 - far jump into protected mode
    jmp dword 08h:.pmode


.pmode:
    ; we are now in protected mode!
    [bits 32]
    
    ; 6 - setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax

%endmacro

; Convert linear address to segment:offset address
; Args:
;    1 - linear address (memory location or register)
;    2 - (out) target segment (e.g. es)
;    3 - target 32-bit register to use (e.g. eax)
;    4 - target lower 16-bit half of #3 (e.g. ax)

%macro LinearToSegOffset 4
    mov %3, %1      ; linear address to register
    shr %3, 4
    mov %2, %4
    mov %3, %1      ; linear address to register
    and %3, 0xf
%endmacro


global x86_outb
x86_outb:
    [bits 32]
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global x86_inb
x86_inb:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret


global x86_Disk_GetDriveParams
x86_Disk_GetDriveParams:
    [bits 32]

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    x86_EnterRealMode
    [bits 16]

    ; save regs
    push es
    push bx
    push esi
    push di

    ; call int13h
    mov dl, [ebp + 8]    ; dl - disk drive
    mov ah, 08h
    mov di, 0           ; es:di - 0000:0000
    mov es, di
    stc
    int 13h

    ; out params
    mov eax, 1
    sbb eax, 0

    ; drive type from bl
    LinearToSegOffset [ebp + 12], es, esi, si
    mov [es:si], bl

    ; cylinders
    mov bl, ch          ; cylinders - lower bits in ch
    mov bh, cl          ; cylinders - upper bits in cl (6-7)
    shr bh, 6
    inc bx

    LinearToSegOffset [ebp + 16], es, esi, si
    mov [es:si], bx

    ; sectors
    xor ch, ch          ; sectors - lower 5 bits in cl
    and cl, 3Fh
    
    LinearToSegOffset [ebp + 20], es, esi, si
    mov [es:si], cx

    ; heads
    mov cl, dh          ; heads - dh
    inc cx

    LinearToSegOffset [ebp + 24], es, esi, si
    mov [es:si], cx

    ; restore regs
    pop di
    pop esi
    pop bx
    pop es

    ; return
    push eax
    x86_EnterProtectedMode
    [bits 32]
    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret


global x86_Disk_Reset
x86_Disk_Reset:
    [bits 32]

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    x86_EnterRealMode
    [bits 16]

    mov ah, 0
    mov dl, [ebp + 8]    ; dl - drive
    stc
    int 13h

    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    push eax
    x86_EnterProtectedMode
    [bits 32]
    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret


global x86_Disk_Read
x86_Disk_Read:
    [bits 32]
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    x86_EnterRealMode
    [bits 16]

    ; save modified regs
    push ebx
    push es

    ; setup args
    mov dl, [ebp + 8]    ; dl - drive

    mov ch, [ebp + 12]    ; ch - cylinder (lower 8 bits)
    mov cl, [ebp + 13]    ; cl - cylinder to bits 6-7
    shl cl, 6
    
    mov al, [ebp + 16]    ; cl - sector to bits 0-5
    and al, 3Fh
    or cl, al

    mov dh, [ebp + 20]   ; dh - head

    mov al, [ebp + 24]   ; al - count

    LinearToSegOffset [ebp + 28], es, ebx, bx

    ; call int13h
    mov ah, 02h
    stc
    int 13h

    ; set return value
    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    ; restore regs
    pop es
    pop ebx

    push eax
    x86_EnterProtectedMode
    [bits 32]
    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret


;
; int ASMCALL x86_E820GetNextBlock(E820MemoryBlock* block, uint32_t* continuationId);
;
E820Signature   equ 0x534D4150

global x86_E820GetNextBlock
x86_E820GetNextBlock:
    [bits 32]
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    x86_EnterRealMode
    [bits 16]

    ; save modified regs
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ds
    push es

    ; setup params
    LinearToSegOffset [ebp + 8], es, edi, di     ; es:di pointer to structure
    
    LinearToSegOffset [ebp + 12], ds, esi, si    ; ds:si - pointer to continuationId
    mov ebx, [ds:si]

    mov eax, 0xE820                             ; eax - function
    mov edx, E820Signature                      ; edx - signature
    mov ecx, 24                                 ; ecx - size of structure

    ; call interrupt
    int 0x15

    ; test results
    cmp eax, E820Signature
    jne .Error

    .IfSuccedeed:
        mov eax, ecx            ; return size
        mov [ds:si], ebx        ; fill continuation parameter
        jmp .EndIf

    .Error:
        mov eax, -1

    .EndIf:

    ; restore regs
    pop es
    pop ds
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx

    push eax
    x86_EnterProtectedMode
    [bits 32]
    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global x86_Video_GetVbeInfo
x86_Video_GetVbeInfo:
    [bits 32]
    push ebp
    mov ebp, esp

    x86_EnterRealMode
    [bits 16]

    push es
    push di

    LinearToSegOffset [ebp + 8], es, edi, di
    mov ax, 0x4F00
    int 0x10

    cmp ax, 0x004F
    jne .error

    mov eax, 1
    jmp .done

.error:
    mov eax, 0

.done:
    pop di
    pop es

    push eax
    x86_EnterProtectedMode
    [bits 32]
    pop eax

    mov esp, ebp
    pop ebp
    ret


global x86_Video_GetModeInfo
x86_Video_GetModeInfo:
    [bits 32]
    push ebp
    mov ebp, esp

    x86_EnterRealMode
    [bits 16]

    push es
    push di
    push cx

    LinearToSegOffset [ebp + 12], es, edi, di
    mov cx, [ebp + 8]
    mov ax, 0x4F01
    int 0x10

    cmp ax, 0x004F
    jne .error

    mov eax, 1
    jmp .done

.error:
    mov eax, 0

.done:
    pop cx
    pop di
    pop es

    push eax
    x86_EnterProtectedMode
    [bits 32]
    pop eax

    mov esp, ebp
    pop ebp
    ret


global x86_Video_SetMode
x86_Video_SetMode:
    [bits 32]
    push ebp
    mov ebp, esp

    x86_EnterRealMode
    [bits 16]

    push bx

    mov bx, [ebp + 8]
    mov ax, 0x4F02
    int 0x10

    cmp ax, 0x004F
    jne .error

    mov eax, 1
    jmp .done

.error:
    mov eax, 0

.done:
    pop bx

    push eax
    x86_EnterProtectedMode
    [bits 32]
    pop eax

    mov esp, ebp
    pop ebp
    ret