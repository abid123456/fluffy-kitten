.prog:
    code_length 256
.data:
    string str1 "the ascii characters :"
    short  nline 10
.var:
    var 2 ptr1
    var 2 c_1
    var 2 i_1
    var 2 i_2
.code:
    mov ax 0
    
    mov bx <str1>
    mov ptr1 bx
    loop1:
        out [ptr1]
        add ptr1 1
     cmp [ptr1] 0
    jne loop1
    out nline
    
    mov ax 15
    mov c_1 0
    mov i_1 0
    
    ; main loop
    loop2:
        mov i_2 0
        loop3:
            out c_1
            inc c_1
            inc i_2
         cmp i_2 16
        jne loop3
        out nline
        inc i_1
     cmp i_1 32
    jne loop2
    ; end of main loop
    
    exit
.end.
