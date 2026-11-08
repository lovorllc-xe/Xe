s/^\.section \.rodata/.section .rdata,"dr"/
s/^\.section \.bss/.section .bss,"bw"/
s/^\.section \.data/.section .data,"dw"/
s/^\.section \.text/.text/
/^\.type /d
/^\.section \.note/d
s/^\.global /.globl /
s/^ syscall$/ call xe.syscall/
s/^xe[.]thread[.]start:/xe.thread.start.linux:/
s/^xe[.]thread[.]join:/xe.thread.join.linux:/
s/^xeprocess[.]run:/xeprocess.run.linux:/
s/^xeprocess[.]spawn:/xeprocess.spawn.linux:/
s/^xeprocess[.]wait:/xeprocess.wait.linux:/
s/^\.align /.balign /
s/^\.pushsection \.rodata/.pushsection .rdata,"dr"/
