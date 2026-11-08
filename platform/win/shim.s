# Xe runtime support for Windows (x86-64, PE). Appended to every program built with `-p win`.
#
# Generated Xe code is platform neutral: it issues Linux-style system calls. For Windows the compiler
# replaces each `syscall` instruction by `call xe.syscall`, and this file implements the calls Xe programs
# use on top of the Win32 API. It also provides the process entry point (command line and environment),
# and emulates Linux's lazy memory commit with a vectored exception handler, because Xe's allocator
# reserves a large address range up front.
.intel_syntax noprefix
.section .text

# ---- Linux syscall emulation: rax = number, rdi rsi rdx r10 r8 r9 = arguments; clobbers only rax rcx r11 ----
.global xe.syscall
xe.syscall:
 push rbp
 mov rbp, rsp
 push rbx
 push rdx
 push r8
 push r9
 push r10
 sub rsp, 192
 and rsp, -16
 movups xmmword ptr [rsp+64], xmm0
 movups xmmword ptr [rsp+80], xmm1
 movups xmmword ptr [rsp+96], xmm2
 movups xmmword ptr [rsp+112], xmm3
 movups xmmword ptr [rsp+128], xmm4
 movups xmmword ptr [rsp+144], xmm5
 cmp rax, 1
 je .LwsWrite
 test rax, rax
 jz .LwsRead
 cmp rax, 257
 je .LwsOpenAt
 cmp rax, 2
 je .LwsOpen
 cmp rax, 3
 je .LwsClose
 cmp rax, 8
 je .LwsSeek
 cmp rax, 9
 je .LwsMmap
 cmp rax, 11
 je .LwsMunmap
 cmp rax, 28
 je .LwsMadvise
 cmp rax, 35
 je .LwsSleep
 cmp rax, 228
 je .LwsClock
 cmp rax, 231
 je .LwsExit
 cmp rax, 60
 je .LwsExit
 cmp rax, 39
 je .LwsGetpid
 cmp rax, 87
 je .LwsUnlink
 cmp rax, 202
 je .LwsFutex
 cmp rax, 24
 je .LwsYield
 cmp rax, 318
 je .LwsRandom
 cmp rax, 83
 je .LwsMkdir
 cmp rax, 84
 je .LwsRmdir
 cmp rax, 82
 je .LwsRename
 cmp rax, 79
 je .LwsGetcwd
 cmp rax, 80
 je .LwsChdir
 mov rax, -38
 jmp .LwsDone

.LwsExit:
 mov ecx, edi
 call qword ptr [rip+__imp_ExitProcess]

.LwsGetpid:
 call qword ptr [rip+__imp_GetCurrentProcessId]
 jmp .LwsDone

# fd -> HANDLE: 0, 1, 2 are the standard handles, everything else is already a handle.
.LwsHandle:
 sub rsp, 40
 mov rax, rdi
 cmp rdi, 2
 ja .LwsHandleDone
 mov ecx, -10
 test rdi, rdi
 jz .LwsStd
 mov ecx, -11
 cmp rdi, 1
 je .LwsStd
 mov ecx, -12
.LwsStd:
 call qword ptr [rip+__imp_GetStdHandle]
.LwsHandleDone:
 add rsp, 40
 ret

.LwsWrite:
 call .LwsHandle
 mov rcx, rax
 mov rdx, rsi
 mov r8, qword ptr [rbp-16]
 lea r9, [rsp+40]
 mov qword ptr [rsp+32], 0
 mov qword ptr [rsp+40], 0
 call qword ptr [rip+__imp_WriteFile]
 test eax, eax
 jz .LwsIoFail
 mov eax, dword ptr [rsp+40]
 jmp .LwsDone

.LwsRead:
 call .LwsHandle
 mov rcx, rax
 mov rdx, rsi
 mov r8, qword ptr [rbp-16]
 lea r9, [rsp+40]
 mov qword ptr [rsp+32], 0
 mov qword ptr [rsp+40], 0
 call qword ptr [rip+__imp_ReadFile]
 test eax, eax
 jz .LwsReadFail
 mov eax, dword ptr [rsp+40]
 jmp .LwsDone
.LwsReadFail:
 call qword ptr [rip+__imp_GetLastError]
 cmp eax, 109
 jne .LwsIoFail
 xor eax, eax
 jmp .LwsDone
.LwsIoFail:
 mov rax, -5
 jmp .LwsDone

# open(path, flags, mode)
.LwsOpen:
 mov rcx, rdi
 mov rdx, rsi
 jmp .LwsOpenCommon
# openat(dirfd, path, flags, mode)
.LwsOpenAt:
 mov rcx, rsi
 mov rdx, qword ptr [rbp-16]
.LwsOpenCommon:
 mov r8d, 0x80000000
 mov eax, edx
 and eax, 3
 jz .LwsAccessOk
 mov r8d, 0x40000000
 cmp eax, 1
 je .LwsAccessOk
 mov r8d, 0xC0000000
.LwsAccessOk:
 test edx, 0x400
 jz .LwsNoAppend
 mov r8d, 0x00100004
.LwsNoAppend:
 mov r9d, 3
 test edx, 0x40
 jz .LwsNoCreate
 mov r9d, 4
 test edx, 0x200
 jz .LwsDisposition
 mov r9d, 2
 jmp .LwsDisposition
.LwsNoCreate:
 mov r9d, 3
 test edx, 0x200
 jz .LwsDisposition
 mov r9d, 5
.LwsDisposition:
 mov rax, r9
 mov r9d, 0
 mov r10d, 3
 mov rdx, r10
 mov qword ptr [rsp+48], 0
 mov qword ptr [rsp+40], 0x80
 mov qword ptr [rsp+32], rax
 xor r9d, r9d
 call qword ptr [rip+__imp_CreateFileA]
 cmp rax, -1
 jne .LwsDone
 mov rax, -2
 jmp .LwsDone

.LwsClose:
 cmp rdi, 2
 jbe .LwsCloseOk
 mov rcx, rdi
 call qword ptr [rip+__imp_CloseHandle]
.LwsCloseOk:
 xor eax, eax
 jmp .LwsDone

.LwsUnlink:
 mov rcx, rdi
 call qword ptr [rip+__imp_DeleteFileA]
 test eax, eax
 mov rax, -2
 jz .LwsDone
 xor eax, eax
 jmp .LwsDone

# lseek(fd, offset, whence)
.LwsSeek:
 call .LwsHandle
 mov rcx, rax
 mov rdx, rsi
 lea r8, [rsp+40]
 mov r9, qword ptr [rbp-16]
 call qword ptr [rip+__imp_SetFilePointerEx]
 test eax, eax
 jz .LwsIoFail
 mov rax, qword ptr [rsp+40]
 jmp .LwsDone

# mmap(addr, length, prot, flags, fd, offset): anonymous memory only.
.LwsMmap:
 cmp qword ptr [rbp-24], -1
 jne .LwsEnosys
 mov rdx, rsi
 xor ecx, ecx
 mov r8d, 0x3000
 mov r9d, 4
 cmp rsi, 67108864
 jb .LwsMapNow
 mov r8d, 0x2000
 call qword ptr [rip+__imp_VirtualAlloc]
 test rax, rax
 jz .LwsNoMem
 mov rcx, qword ptr [rip+.LxeWinRangeCount]
 cmp rcx, 64
 jae .LwsDone
 lea rdx, [rip+.LxeWinRanges]
 shl rcx, 4
 add rdx, rcx
 mov qword ptr [rdx], rax
 lea rcx, [rax+rsi]
 mov qword ptr [rdx+8], rcx
 inc qword ptr [rip+.LxeWinRangeCount]
 jmp .LwsDone
.LwsMapNow:
 call qword ptr [rip+__imp_VirtualAlloc]
 test rax, rax
 jnz .LwsDone
.LwsNoMem:
 mov rax, -12
 jmp .LwsDone
.LwsEnosys:
 mov rax, -38
 jmp .LwsDone

.LwsMunmap:
 mov rcx, rdi
 xor edx, edx
 mov r8d, 0x8000
 call qword ptr [rip+__imp_VirtualFree]
 xor eax, eax
 jmp .LwsDone

# madvise(addr, length, MADV_DONTNEED): give the pages back; they read as zero when touched again.
.LwsMadvise:
 mov rcx, rdi
 mov rdx, rsi
 mov r8d, 0x4000
 call qword ptr [rip+__imp_VirtualFree]
 xor eax, eax
 jmp .LwsDone

# nanosleep(req, rem)
.LwsSleep:
 mov rax, qword ptr [rdi]
 imul rax, rax, 1000
 mov rcx, qword ptr [rdi+8]
 add rcx, 999999
 mov rbx, rax
 mov rax, rcx
 xor edx, edx
 mov rcx, 1000000
 div rcx
 add rax, rbx
 mov rcx, rax
 call qword ptr [rip+__imp_Sleep]
 xor eax, eax
 jmp .LwsDone

# clock_gettime(clock, timespec): clock 0 is wall time, everything else a monotonic counter.
.LwsClock:
 mov rbx, rsi
 test rdi, rdi
 jnz .LwsMonotonic
 lea rcx, [rsp+40]
 call qword ptr [rip+__imp_GetSystemTimeAsFileTime]
 mov rax, qword ptr [rsp+40]
 mov rcx, 116444736000000000
 sub rax, rcx
 xor edx, edx
 mov rcx, 10000000
 div rcx
 mov qword ptr [rbx], rax
 imul rdx, rdx, 100
 mov qword ptr [rbx+8], rdx
 xor eax, eax
 jmp .LwsDone
.LwsMonotonic:
 lea rcx, [rsp+40]
 call qword ptr [rip+__imp_QueryPerformanceCounter]
 lea rcx, [rsp+48]
 call qword ptr [rip+__imp_QueryPerformanceFrequency]
 mov rax, qword ptr [rsp+40]
 xor edx, edx
 mov rcx, qword ptr [rsp+48]
 div rcx
 mov qword ptr [rbx], rax
 mov rax, rdx
 imul rax, rax, 1000000000
 xor edx, edx
 div rcx
 mov qword ptr [rbx+8], rax
 xor eax, eax

# futex(addr, op, value): waiting is a polite spin (callers re-check the value, as futex users must anyway)
.LwsFutex:
 mov eax, esi
 and eax, 127
 jnz .LwsFutexWake
 mov eax, dword ptr [rdi]
 cmp eax, dword ptr [rbp-16]
 jne .LwsAgain
 xor ecx, ecx
 call qword ptr [rip+__imp_Sleep]
 xor eax, eax
 jmp .LwsDone
.LwsAgain:
 mov rax, -11
 jmp .LwsDone
.LwsFutexWake:
 xor eax, eax
 jmp .LwsDone

.LwsYield:
 call qword ptr [rip+__imp_SwitchToThread]
 xor eax, eax
 jmp .LwsDone

# getrandom(buffer, length, flags): RtlGenRandom (advapi32 SystemFunction036), looked up on first use
.LwsRandom:
 mov rax, qword ptr [rip+.LxeWinRandom]
 test rax, rax
 jnz .LwsRandomHave
 lea rcx, [rip+.LxeWinAdvapi]
 call qword ptr [rip+__imp_LoadLibraryA]
 mov rcx, rax
 lea rdx, [rip+.LxeWinRandomName]
 call qword ptr [rip+__imp_GetProcAddress]
 mov qword ptr [rip+.LxeWinRandom], rax
 test rax, rax
 jz .LwsEnosys
.LwsRandomHave:
 mov rcx, rdi
 mov rdx, rsi
 call rax
 mov rax, rsi
 jmp .LwsDone

.LwsMkdir:
 mov rcx, rdi
 xor edx, edx
 call qword ptr [rip+__imp_CreateDirectoryA]
 test eax, eax
 mov rax, -17
 jz .LwsDone
 xor eax, eax
 jmp .LwsDone

.LwsRmdir:
 mov rcx, rdi
 call qword ptr [rip+__imp_RemoveDirectoryA]
 test eax, eax
 mov rax, -2
 jz .LwsDone
 xor eax, eax
 jmp .LwsDone

.LwsRename:
 mov rcx, rdi
 mov rdx, rsi
 mov r8d, 1
 call qword ptr [rip+__imp_MoveFileExA]
 test eax, eax
 mov rax, -2
 jz .LwsDone
 xor eax, eax
 jmp .LwsDone

# getcwd(buffer, size): returns the length including the NUL, like the kernel call
.LwsGetcwd:
 mov rcx, rsi
 mov rdx, rdi
 call qword ptr [rip+__imp_GetCurrentDirectoryA]
 test eax, eax
 jz .LwsIoFail
 inc eax
 jmp .LwsDone

.LwsChdir:
 mov rcx, rdi
 call qword ptr [rip+__imp_SetCurrentDirectoryA]
 test eax, eax
 mov rax, -2
 jz .LwsDone
 xor eax, eax
 jmp .LwsDone

.LwsDone:
 movups xmm0, xmmword ptr [rsp+64]
 movups xmm1, xmmword ptr [rsp+80]
 movups xmm2, xmmword ptr [rsp+96]
 movups xmm3, xmmword ptr [rsp+112]
 movups xmm4, xmmword ptr [rsp+128]
 movups xmm5, xmmword ptr [rsp+144]
 lea rsp, [rbp-40]
 pop r10
 pop r9
 pop r8
 pop rdx
 pop rbx
 pop rbp
 ret

# ---- lazy commit: a fault inside a reserved range commits the surrounding 1 MiB ----
.global xe.win.fault
xe.win.fault:
 mov rax, qword ptr [rcx]
 cmp dword ptr [rax], 0xC0000005
 jne .LwfNo
 mov rdx, qword ptr [rax+40]
 lea r8, [rip+.LxeWinRanges]
 mov r9, qword ptr [rip+.LxeWinRangeCount]
.LwfScan:
 test r9, r9
 jz .LwfNo
 cmp rdx, qword ptr [r8]
 jb .LwfNext
 cmp rdx, qword ptr [r8+8]
 jae .LwfNext
 mov rcx, rdx
 and rcx, -1048576
 cmp rcx, qword ptr [r8]
 jae .LwfStart
 mov rcx, qword ptr [r8]
.LwfStart:
 mov rdx, qword ptr [r8+8]
 sub rdx, rcx
 cmp rdx, 1048576
 jbe .LwfCommit
 mov edx, 1048576
.LwfCommit:
 sub rsp, 40
 mov r8d, 0x1000
 mov r9d, 4
 call qword ptr [rip+__imp_VirtualAlloc]
 add rsp, 40
 test rax, rax
 jz .LwfNo
 mov eax, -1
 ret
.LwfNext:
 add r8, 16
 dec r9
 jmp .LwfScan
.LwfNo:
 xor eax, eax
 ret

# ---- process entry: build argc/argv/envp like the Linux loader does ----
.global xe.win.entry
xe.win.entry:
 push rbp
 mov rbp, rsp
 sub rsp, 64
 xor ecx, ecx
 mov edx, 131072
 mov r8d, 0x3000
 mov r9d, 4
 call qword ptr [rip+__imp_VirtualAlloc]
 mov rbx, rax
 mov ecx, 1
 lea rdx, [rip+xe.win.fault]
 call qword ptr [rip+__imp_AddVectoredExceptionHandler]
 call qword ptr [rip+__imp_GetCommandLineA]
 mov rsi, rax
 lea rdi, [rbx+32768]
 mov r12, rbx
 xor r13d, r13d
.LweToken:
 mov al, byte ptr [rsi]
 cmp al, 32
 je .LweSkip
 cmp al, 9
 je .LweSkip
 test al, al
 jz .LweArgsDone
 mov qword ptr [r12], rdi
 add r12, 8
 inc r13
 xor r14d, r14d
.LweChar:
 mov al, byte ptr [rsi]
 test al, al
 jz .LweEnd
 cmp al, 34
 jne .LweNotQuote
 xor r14d, 1
 inc rsi
 jmp .LweChar
.LweNotQuote:
 test r14d, r14d
 jnz .LweCopy
 cmp al, 32
 je .LweEnd
 cmp al, 9
 je .LweEnd
.LweCopy:
 mov byte ptr [rdi], al
 inc rdi
 inc rsi
 jmp .LweChar
.LweEnd:
 mov byte ptr [rdi], 0
 inc rdi
 jmp .LweToken
.LweSkip:
 inc rsi
 jmp .LweToken
.LweArgsDone:
 mov qword ptr [r12], 0
 mov qword ptr [rip+.LxeArgc], r13
 mov qword ptr [rip+.LxeArgv], rbx
 call qword ptr [rip+__imp_GetEnvironmentStringsA]
 mov rsi, rax
 lea r12, [rbx+65536]
 mov qword ptr [rip+.LxeEnv], r12
.LweEnv:
 cmp byte ptr [rsi], 0
 je .LweEnvDone
 mov qword ptr [r12], rsi
 add r12, 8
.LweEnvScan:
 cmp byte ptr [rsi], 0
 je .LweEnvNext
 inc rsi
 jmp .LweEnvScan
.LweEnvNext:
 inc rsi
 jmp .LweEnv
.LweEnvDone:
 mov qword ptr [r12], 0
 mov qword ptr [rip+.LxeStackBase], rsp
 call main
 mov ecx, eax
 call qword ptr [rip+__imp_ExitProcess]

# ---- threads: one Windows thread per Xe thread; the record is {handle, function, argument, result} ----
.global xe.thread.start
xe.thread.start:
 push rbp
 mov rbp, rsp
 push rbx
 push r12
 push r13
 mov rbx, rdi
 mov r12, rsi
 lock inc qword ptr [rip+.LxeThreadsActive]
 and rsp, -16
 mov edi, 32
 call xe.alloc
 test rax, rax
 jz .LwtFail
 mov r13, rax
 mov qword ptr [r13], 0
 mov qword ptr [r13+8], rbx
 mov qword ptr [r13+16], r12
 mov qword ptr [r13+24], 0
 sub rsp, 64
 xor ecx, ecx
 xor edx, edx
 lea r8, [rip+xe.win.threadmain]
 mov r9, r13
 mov qword ptr [rsp+32], 0
 mov qword ptr [rsp+40], 0
 call qword ptr [rip+__imp_CreateThread]
 add rsp, 64
 test rax, rax
 jz .LwtFreeRecord
 mov qword ptr [r13], rax
 mov rax, r13
 jmp .LwtDone
.LwtFreeRecord:
 mov rdi, r13
 call xe.free
.LwtFail:
 lock dec qword ptr [rip+.LxeThreadsActive]
 mov rax, -1
.LwtDone:
 lea rsp, [rbp-24]
 pop r13
 pop r12
 pop rbx
 pop rbp
 ret

xe.win.threadmain:
 push rbp
 mov rbp, rsp
 push rbx
 push rdi
 push rsi
 push r12
 push r13
 push r14
 push r15
 sub rsp, 8
 mov rbx, rcx
 mov rax, qword ptr [rbx+8]
 mov rdi, qword ptr [rbx+16]
 call rax
 mov qword ptr [rbx+24], rax
 add rsp, 8
 pop r15
 pop r14
 pop r13
 pop r12
 pop rsi
 pop rdi
 pop rbx
 pop rbp
 xor eax, eax
 ret

.global xe.thread.join
xe.thread.join:
 push rbp
 mov rbp, rsp
 push rbx
 push r12
 mov rbx, rdi
 and rsp, -16
 sub rsp, 48
 mov rcx, qword ptr [rbx]
 mov edx, 0xFFFFFFFF
 call qword ptr [rip+__imp_WaitForSingleObject]
 mov rcx, qword ptr [rbx]
 call qword ptr [rip+__imp_CloseHandle]
 add rsp, 48
 mov r12, qword ptr [rbx+24]
 mov rdi, rbx
 call xe.free
 lock dec qword ptr [rip+.LxeThreadsActive]
 mov rax, r12
 lea rsp, [rbp-16]
 pop r12
 pop rbx
 pop rbp
 ret

# ---- processes: run a command line through the Windows command interpreter ----
# xe.win.launch(command) -> process handle or 0
xe.win.launch:
 push rbp
 mov rbp, rsp
 push rbx
 push r12
 push r13
 mov rbx, rdi
 xor eax, eax
.LwlLength:
 cmp byte ptr [rbx+rax], 0
 je .LwlSized
 inc rax
 jmp .LwlLength
.LwlSized:
 lea rdi, [rax+16]
 and rsp, -16
 call xe.alloc
 mov r12, rax
 mov dword ptr [r12], 0x2E646D63
 mov dword ptr [r12+4], 0x20657865
 mov dword ptr [r12+8], 0x20632F
 xor ecx, ecx
.LwlCopy:
 mov al, byte ptr [rbx+rcx]
 mov byte ptr [r12+11+rcx], al
 inc rcx
 test al, al
 jnz .LwlCopy
 sub rsp, 256
 lea r13, [rsp+96]
 xor eax, eax
 mov ecx, 32
.LwlZero:
 mov qword ptr [rsp+rcx*8-8+0], 0
 dec ecx
 jnz .LwlZero
 mov dword ptr [rsp+96], 104
 xor ecx, ecx
 mov rdx, r12
 xor r8d, r8d
 xor r9d, r9d
 mov qword ptr [rsp+32], 1
 mov qword ptr [rsp+40], 0
 mov qword ptr [rsp+48], 0
 mov qword ptr [rsp+56], 0
 mov qword ptr [rsp+64], r13
 lea rax, [rsp+208]
 mov qword ptr [rsp+72], rax
 call qword ptr [rip+__imp_CreateProcessA]
 mov rbx, rax
 mov rdi, r12
 call xe.free
 xor eax, eax
 test ebx, ebx
 jz .LwlDone
 mov rcx, qword ptr [rsp+216]
 call qword ptr [rip+__imp_CloseHandle]
 mov rax, qword ptr [rsp+208]
.LwlDone:
 lea rsp, [rbp-24]
 pop r13
 pop r12
 pop rbx
 pop rbp
 ret

# xe.win.reap(process handle) -> exit code, or -1
xe.win.reap:
 push rbp
 mov rbp, rsp
 push rbx
 mov rbx, rdi
 and rsp, -16
 sub rsp, 48
 test rbx, rbx
 jz .LwrFail
 mov rcx, rbx
 mov edx, 0xFFFFFFFF
 call qword ptr [rip+__imp_WaitForSingleObject]
 mov dword ptr [rsp+40], 0
 mov rcx, rbx
 lea rdx, [rsp+40]
 call qword ptr [rip+__imp_GetExitCodeProcess]
 mov rcx, rbx
 call qword ptr [rip+__imp_CloseHandle]
 mov eax, dword ptr [rsp+40]
 jmp .LwrDone
.LwrFail:
 mov rax, -1
.LwrDone:
 lea rsp, [rbp-8]
 pop rbx
 pop rbp
 ret

.global xeprocess.run
xeprocess.run:
 push rbp
 mov rbp, rsp
 call xe.win.launch
 mov rdi, rax
 call xe.win.reap
 pop rbp
 ret

.global xeprocess.spawn
xeprocess.spawn:
 push rbp
 mov rbp, rsp
 call xe.win.launch
 test rax, rax
 jnz .LwpSpawned
 mov rax, -1
.LwpSpawned:
 pop rbp
 ret

.global xeprocess.wait
xeprocess.wait:
 push rbp
 mov rbp, rsp
 call xe.win.reap
 pop rbp
 ret

.section .data
.align 8
.LxeWinRandom:
.quad 0
.LxeWinAdvapi:
.asciz "advapi32.dll"
.LxeWinRandomName:
.asciz "SystemFunction036"

.LxeWinRangeCount:
.quad 0
.LxeWinRanges:
.skip 1024
