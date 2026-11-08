s/^ *\.section \.rodata/.section __TEXT,__const/
s/^ *\.section \.bss/.section __DATA,__data/
s/^ *\.section \.data/.section __DATA,__data/
s/^ *\.section \.text/.text/
/^\.type /d
/^\.section \.note/d
s/^\.global /.globl /
s/^ syscall$/ call xe.syscall/
s/^\.align /.balign /
s/^\.pushsection \.rodata/.pushsection __TEXT,__const/
