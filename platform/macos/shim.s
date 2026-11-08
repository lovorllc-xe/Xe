# Xe runtime support for macOS (x86-64 Mach-O). Appended to every program built with `-p macos`.
#
# NOT TESTED ON REAL HARDWARE: this was written against the documented BSD system-call interface and only
# checked to assemble and link; there is no macOS machine to run it on in the project's test setup.
#
# Generated Xe code issues Linux-style system calls; for macOS the compiler replaces each `syscall` with
# `call xe.syscall`, which translates the common calls (read, write, open, close, lseek, mmap, munmap,
# madvise, exit, getpid, unlink, mkdir, rmdir, rename, chdir) to the BSD numbering and flags. Threads, sockets,
# clocks and processes are not provided yet; those calls fail with ENOSYS.
.intel_syntax noprefix
.text

.globl xe.syscall
xe.syscall:
 cmp rax, 0
 je .LmsRead
 cmp rax, 1
 je .LmsWrite
 cmp rax, 257
 je .LmsOpenAt
 cmp rax, 2
 je .LmsOpen
 cmp rax, 3
 je .LmsClose
 cmp rax, 8
 je .LmsSeek
 cmp rax, 9
 je .LmsMmap
 cmp rax, 11
 je .LmsMunmap
 cmp rax, 28
 je .LmsMadvise
 cmp rax, 39
 je .LmsGetpid
 cmp rax, 60
 je .LmsExit
 cmp rax, 231
 je .LmsExit
 cmp rax, 87
 je .LmsUnlink
 cmp rax, 83
 je .LmsMkdir
 cmp rax, 84
 je .LmsRmdir
 cmp rax, 82
 je .LmsRename
 cmp rax, 80
 je .LmsChdir
 mov rax, -38
 ret

.LmsRead:
 mov eax, 0x2000003
 jmp .LmsCall
.LmsWrite:
 mov eax, 0x2000004
 jmp .LmsCall
.LmsClose:
 mov eax, 0x2000006
 jmp .LmsCall
.LmsSeek:
 mov eax, 0x2000199
 jmp .LmsCall
.LmsMunmap:
 mov eax, 0x2000073
 jmp .LmsCall
.LmsGetpid:
 mov eax, 0x2000014
 jmp .LmsCall
.LmsExit:
 mov eax, 0x2000001
 jmp .LmsCall
.LmsUnlink:
 mov eax, 0x200000A
 jmp .LmsCall
.LmsMkdir:
 mov eax, 0x2000088
 jmp .LmsCall
.LmsRmdir:
 mov eax, 0x2000089
 jmp .LmsCall
.LmsRename:
 mov eax, 0x2000080
 jmp .LmsCall
.LmsChdir:
 mov eax, 0x200000C
 jmp .LmsCall

# open(path, flags, mode) / openat(dirfd, path, flags, mode): translate the flag bits
.LmsOpenAt:
 mov rdi, rsi
 mov rsi, rdx
 mov rdx, r10
.LmsOpen:
 mov eax, esi
 and eax, 3
 mov r11d, eax
 test esi, 0x40
 jz .LmsNoCreate
 or r11d, 0x200
.LmsNoCreate:
 test esi, 0x200
 jz .LmsNoTrunc
 or r11d, 0x400
.LmsNoTrunc:
 test esi, 0x400
 jz .LmsNoAppend
 or r11d, 0x8
.LmsNoAppend:
 test esi, 0x80
 jz .LmsOpenFlags
 or r11d, 0x800
.LmsOpenFlags:
 mov esi, r11d
 mov eax, 0x2000005
 jmp .LmsCall

# mmap(addr, length, prot, flags, fd, offset): MAP_ANONYMOUS and MAP_NORESERVE have other values on macOS
.LmsMmap:
 mov r11d, r10d
 and r10d, 0x13
 test r11d, 0x20
 jz .LmsMapNoAnon
 or r10d, 0x1000
.LmsMapNoAnon:
 test r11d, 0x4000
 jz .LmsMapNoReserve
 or r10d, 0x40
.LmsMapNoReserve:
 mov eax, 0x2000197
 jmp .LmsCall

# madvise(addr, length, MADV_DONTNEED): macOS does not zero the pages, so replace the mapping with fresh ones
.LmsMadvise:
 mov edx, 3
 mov r10d, 0x1012
 mov r8, -1
 xor r9d, r9d
 mov eax, 0x2000197
 syscall
 jnc .LmsMadviseDone
 neg rax
 ret
.LmsMadviseDone:
 xor eax, eax
 ret

.LmsCall:
 syscall
 jnc .LmsOk
 neg rax
.LmsOk:
 ret

# ---- process entry: dyld calls main(argc, argv, envp, apple) ----
.globl xe.mac.entry
xe.mac.entry:
 push rbp
 mov rbp, rsp
 mov qword ptr [rip+.LxeArgc], rdi
 mov qword ptr [rip+.LxeArgv], rsi
 mov qword ptr [rip+.LxeEnv], rdx
 mov qword ptr [rip+.LxeStackBase], rsp
 call main
 mov edi, eax
 mov eax, 231
 call xe.syscall
 ud2
