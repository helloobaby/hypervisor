.code

?read_cs@x86@@YA?ATsegment_selector@@XZ proc
  mov ax, cs
  ret
?read_cs@x86@@YA?ATsegment_selector@@XZ endp

?read_ss@x86@@YA?ATsegment_selector@@XZ proc
  mov ax, ss
  ret
?read_ss@x86@@YA?ATsegment_selector@@XZ endp

?read_ds@x86@@YA?ATsegment_selector@@XZ proc
  mov ax, ds
  ret
?read_ds@x86@@YA?ATsegment_selector@@XZ endp

?read_es@x86@@YA?ATsegment_selector@@XZ proc
  mov ax, es
  ret
?read_es@x86@@YA?ATsegment_selector@@XZ endp

?read_fs@x86@@YA?ATsegment_selector@@XZ proc
  mov ax, fs
  ret
?read_fs@x86@@YA?ATsegment_selector@@XZ endp

?read_gs@x86@@YA?ATsegment_selector@@XZ proc
  mov ax, gs
  ret
?read_gs@x86@@YA?ATsegment_selector@@XZ endp

?read_tr@x86@@YA?ATsegment_selector@@XZ proc
  str ax
  ret
?read_tr@x86@@YA?ATsegment_selector@@XZ endp

?read_ldtr@x86@@YA?ATsegment_selector@@XZ proc
  sldt ax
  ret
?read_ldtr@x86@@YA?ATsegment_selector@@XZ endp

?write_ds@x86@@YAXG@Z proc
  mov ds, cx
  ret
?write_ds@x86@@YAXG@Z endp

?write_es@x86@@YAXG@Z proc
  mov es, cx
  ret
?write_es@x86@@YAXG@Z endp

?write_fs@hv@@YAXG@Z proc
  mov fs, cx
  ret
?write_fs@hv@@YAXG@Z endp

?write_gs@x86@@YAXG@Z proc
  mov gs, cx
  ret
?write_gs@x86@@YAXG@Z endp

?write_tr@x86@@YAXG@Z proc
  ltr cx
  ret
?write_tr@x86@@YAXG@Z endp

?write_ldtr@x86@@YAXG@Z proc
  lldt cx
  ret
?write_ldtr@x86@@YAXG@Z endp

end

