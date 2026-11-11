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
 push rdi
 push rsi
 sub rsp, 640
 and rsp, -16
 movups xmmword ptr [rsp+400], xmm0
 movups xmmword ptr [rsp+416], xmm1
 movups xmmword ptr [rsp+432], xmm2
 movups xmmword ptr [rsp+448], xmm3
 movups xmmword ptr [rsp+464], xmm4
 movups xmmword ptr [rsp+480], xmm5
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
 cmp rax, 4
 je .LwsStat
 cmp rax, 6
 je .LwsStat
 cmp rax, 21
 je .LwsAccess
 cmp rax, 5
 je .LwsFstat
 cmp rax, 63
 je .LwsUname
 cmp rax, 102
 je .LwsZero
 cmp rax, 104
 je .LwsZero
 cmp rax, 110
 je .LwsGetpid
 cmp rax, 217
 je .LwsGetdents
 cmp rax, 41
 je .LwsSocket
 cmp rax, 42
 je .LwsSock3Connect
 cmp rax, 43
 je .LwsSock3Accept
 cmp rax, 49
 je .LwsSock3Bind
 cmp rax, 50
 je .LwsSock2Listen
 cmp rax, 44
 je .LwsSendto
 cmp rax, 45
 je .LwsRecvfrom
 cmp rax, 48
 je .LwsSock2Shutdown
 cmp rax, 51
 je .LwsSock3Getsockname
 cmp rax, 52
 je .LwsSock3Getpeername
 cmp rax, 54
 je .LwsSetsockopt
 cmp rax, 7
 je .LwsPoll
 cmp rax, 204
 je .LwsAffinity
 cmp rax, 13
 je .LwsZero
 cmp rax, 62
 je .LwsKill
 cmp rax, 89
 je .LwsReadlink
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
 call .LwsIsSocket
 test eax, eax
 jnz .LwsSockWrite
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
 call .LwsIsSocket
 test eax, eax
 jnz .LwsSockRead
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
 test edx, 0x10000
 jnz .LwsOpenDir
 mov qword ptr [rsp+56], rdx
 mov r8d, 0x80000000
 mov eax, edx
 and eax, 3
 jz .LwsAccessOk
 mov r8d, 0x40000000
 cmp eax, 1
 je .LwsAccessOk
 mov r8d, 0xC0000000
.LwsAccessOk:
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
 mov rdx, r8
 mov r8d, 7
 xor r9d, r9d
 mov qword ptr [rsp+48], 0
 mov qword ptr [rsp+40], 0x80
 mov qword ptr [rsp+32], rax
 call qword ptr [rip+__imp_CreateFileA]
 cmp rax, -1
 jne .LwsOpened
 mov rax, -2
 jmp .LwsDone
.LwsOpened:
 test dword ptr [rsp+56], 0x400
 jz .LwsDone
 mov qword ptr [rsp+64], rax
 mov rcx, rax
 xor edx, edx
 xor r8d, r8d
 mov r9d, 2
 call qword ptr [rip+__imp_SetFilePointerEx]
 mov rax, qword ptr [rsp+64]
 jmp .LwsDone

.LwsClose:
 cmp rdi, 2
 jbe .LwsCloseOk
 mov rax, rdi
 sub rax, 0x7F000000
 cmp rax, 16
 jb .LwsCloseDir
 call .LwsIsSocket
 test eax, eax
 jnz .LwsCloseSock
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
 jne .LwsMapFile
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
 call qword ptr [rip+__imp_UnmapViewOfFile]
 test eax, eax
 jnz .LwsMunmapDone
 mov rcx, rdi
 xor edx, edx
 mov r8d, 0x8000
 call qword ptr [rip+__imp_VirtualFree]
.LwsMunmapDone:
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

.LwsZero:
 xor eax, eax
 jmp .LwsDone

# access(path, mode)
.LwsAccess:
 mov rcx, rdi
 call qword ptr [rip+__imp_GetFileAttributesA]
 cmp eax, -1
 mov rax, -2
 je .LwsDone
 xor eax, eax
 jmp .LwsDone

# FILETIME (100 ns since 1601) in rax -> Unix seconds in rax
.LwsFt:
 mov rcx, 116444736000000000
 sub rax, rcx
 xor edx, edx
 mov rcx, 10000000
 div rcx
 ret

# stat(path, buf) / lstat(path, buf): the fields Xe's stat library reads
.LwsStat:
 mov rbx, rsi
 mov rcx, rdi
 xor edx, edx
 lea r8, [rsp+48]
 call qword ptr [rip+__imp_GetFileAttributesExA]
 test eax, eax
 jz .LwsNoEnt
 mov rdi, rbx
 xor eax, eax
 mov ecx, 18
 rep stosq
 mov eax, dword ptr [rsp+48]
 mov ecx, 0x81A4
 test eax, 0x10
 jz .LwsStatMode
 mov ecx, 0x41ED
.LwsStatMode:
 mov dword ptr [rbx+24], ecx
 mov qword ptr [rbx+16], 1
 mov eax, dword ptr [rsp+76]
 shl rax, 32
 mov ecx, dword ptr [rsp+80]
 or rax, rcx
 mov qword ptr [rbx+48], rax
 mov rax, qword ptr [rsp+60]
 call .LwsFt
 mov qword ptr [rbx+72], rax
 mov rax, qword ptr [rsp+68]
 call .LwsFt
 mov qword ptr [rbx+88], rax
 mov rax, qword ptr [rsp+52]
 call .LwsFt
 mov qword ptr [rbx+104], rax
 xor eax, eax
 jmp .LwsDone
.LwsNoEnt:
 mov rax, -2
 jmp .LwsDone

# fstat(fd, buf): size and a regular-file mode
.LwsFstat:
 mov rbx, rsi
 call .LwsHandle
 mov rcx, rax
 lea rdx, [rsp+48]
 call qword ptr [rip+__imp_GetFileSizeEx]
 test eax, eax
 jz .LwsIoFail
 mov rdi, rbx
 xor eax, eax
 mov ecx, 18
 rep stosq
 mov dword ptr [rbx+24], 0x81A4
 mov qword ptr [rbx+16], 1
 mov rax, qword ptr [rsp+48]
 mov qword ptr [rbx+48], rax
 xor eax, eax
 jmp .LwsDone

# uname(buf)
.LwsUname:
 mov rbx, rdi
 xor eax, eax
 mov ecx, 390
 rep stosb
 mov rax, 0x0073776F646E6957
 mov qword ptr [rbx], rax
 mov dword ptr [rsp+48], 64
 lea rcx, [rbx+65]
 lea rdx, [rsp+48]
 call qword ptr [rip+__imp_GetComputerNameA]
 mov rax, 0x0000000000003031
 mov qword ptr [rbx+130], rax
 mov rax, 0x000034365F363878
 mov qword ptr [rbx+260], rax
 xor eax, eax
 jmp .LwsDone

# mmap of a file (read-only): CreateFileMapping + MapViewOfFile
.LwsMapFile:
 mov rcx, qword ptr [rbp-24]
 xor edx, edx
 mov r8d, 2
 xor r9d, r9d
 mov qword ptr [rsp+32], 0
 mov qword ptr [rsp+40], 0
 call qword ptr [rip+__imp_CreateFileMappingA]
 test rax, rax
 jz .LwsNoMem
 mov rbx, rax
 mov rcx, rbx
 mov edx, 4
 xor r8d, r8d
 xor r9d, r9d
 mov qword ptr [rsp+32], 0
 call qword ptr [rip+__imp_MapViewOfFile]
 mov qword ptr [rsp+48], rax
 mov rcx, rbx
 call qword ptr [rip+__imp_CloseHandle]
 mov rax, qword ptr [rsp+48]
 test rax, rax
 jnz .LwsDone
 jmp .LwsNoMem

# ---- directories: open(O_DIRECTORY) hands out a small fake descriptor, getdents64 walks it with FindFirstFile ----
# slot (640 bytes): +0 find handle, +8 in use, +16 an entry is waiting, +24 search pattern, +304 WIN32_FIND_DATAA
.LwsOpenDir:
 mov rbx, rcx
 call qword ptr [rip+__imp_GetFileAttributesA]
 cmp eax, -1
 je .LwsNoEnt
 test eax, 0x10
 jz .LwsNotDir
 lea rsi, [rip+.LxeWinDirs]
 xor ecx, ecx
.LwsOdFind:
 cmp qword ptr [rsi+8], 0
 je .LwsOdFound
 add rsi, 640
 inc ecx
 cmp ecx, 16
 jb .LwsOdFind
 mov rax, -24
 jmp .LwsDone
.LwsOdFound:
 mov qword ptr [rsi+8], 1
 mov qword ptr [rsi], 0
 mov qword ptr [rsi+16], 0
 lea rdi, [rsi+24]
 xor ecx, ecx
.LwsOdCopy:
 mov al, byte ptr [rbx+rcx]
 test al, al
 jz .LwsOdCopied
 mov byte ptr [rdi+rcx], al
 inc ecx
 cmp ecx, 259
 jb .LwsOdCopy
.LwsOdCopied:
 mov byte ptr [rdi+rcx], 92
 mov byte ptr [rdi+rcx+1], 42
 mov byte ptr [rdi+rcx+2], 0
 lea rax, [rip+.LxeWinDirs]
 sub rsi, rax
 mov rax, rsi
 xor edx, edx
 mov ecx, 640
 div rcx
 add rax, 0x7F000000
 jmp .LwsDone
.LwsNotDir:
 mov rax, -20
 jmp .LwsDone

.LwsCloseDir:
 imul rax, rax, 640
 lea rbx, [rip+.LxeWinDirs]
 add rbx, rax
 mov rcx, qword ptr [rbx]
 test rcx, rcx
 jz .LwsCdSkip
 call qword ptr [rip+__imp_FindClose]
.LwsCdSkip:
 mov qword ptr [rbx], 0
 mov qword ptr [rbx+8], 0
 xor eax, eax
 jmp .LwsDone

# getdents64(fd, buffer, size)
.LwsGetdents:
 mov rax, rdi
 sub rax, 0x7F000000
 cmp rax, 16
 jae .LwsBadf
 imul rax, rax, 640
 lea rbx, [rip+.LxeWinDirs]
 add rbx, rax
 cmp qword ptr [rbx+8], 0
 je .LwsBadf
 mov qword ptr [rsp+48], rsi
 mov rax, qword ptr [rbp-16]
 mov qword ptr [rsp+56], rax
 mov qword ptr [rsp+64], 0
.LwsGdLoop:
 cmp qword ptr [rbx+16], 0
 jne .LwsGdHave
 cmp qword ptr [rbx], 0
 jne .LwsGdNext
 lea rcx, [rbx+24]
 lea rdx, [rbx+304]
 call qword ptr [rip+__imp_FindFirstFileA]
 cmp rax, -1
 je .LwsGdDone
 mov qword ptr [rbx], rax
 jmp .LwsGdSet
.LwsGdNext:
 mov rcx, qword ptr [rbx]
 lea rdx, [rbx+304]
 call qword ptr [rip+__imp_FindNextFileA]
 test eax, eax
 jz .LwsGdDone
.LwsGdSet:
 mov qword ptr [rbx+16], 1
.LwsGdHave:
 lea rsi, [rbx+348]
 xor ecx, ecx
.LwsGdLen:
 cmp byte ptr [rsi+rcx], 0
 je .LwsGdSized
 inc ecx
 jmp .LwsGdLen
.LwsGdSized:
 lea eax, [rcx+27]
 and eax, -8
 mov rdx, qword ptr [rsp+64]
 add rdx, rax
 cmp rdx, qword ptr [rsp+56]
 ja .LwsGdFull
 mov rdi, qword ptr [rsp+48]
 add rdi, qword ptr [rsp+64]
 mov qword ptr [rdi], 1
 mov qword ptr [rdi+8], rdx
 mov word ptr [rdi+16], ax
 mov edx, dword ptr [rbx+304]
 mov r8d, 8
 test edx, 0x10
 jz .LwsGdType
 mov r8d, 4
.LwsGdType:
 mov byte ptr [rdi+18], r8b
 lea r9, [rdi+19]
 xor edx, edx
.LwsGdCopy:
 mov r10b, byte ptr [rsi+rdx]
 mov byte ptr [r9+rdx], r10b
 inc edx
 cmp edx, ecx
 jbe .LwsGdCopy
 mov qword ptr [rbx+16], 0
 add qword ptr [rsp+64], rax
 jmp .LwsGdLoop
.LwsGdFull:
 cmp qword ptr [rsp+64], 0
 jne .LwsGdDone
 mov rax, -22
 jmp .LwsDone
.LwsGdDone:
 mov rax, qword ptr [rsp+64]
 jmp .LwsDone
.LwsBadf:
 mov rax, -9
 jmp .LwsDone

# ---- sockets: Winsock, loaded on first use; Linux socket calls are translated ----
# the Winsock functions needed, in table order
.LwsWsInit:
 cmp qword ptr [rip+.LxeWinWsReady], 0
 jne .LwsWsInitDone
 push rdi
 push rsi
 sub rsp, 40
 lea rcx, [rip+.LxeWinWsDll]
 call qword ptr [rip+__imp_LoadLibraryA]
 test rax, rax
 jz .LwsWsInitEnd
 mov rbx, rax
 lea rsi, [rip+.LxeWinWsNames]
 xor edi, edi
.LwsWsLoop:
 mov rcx, rbx
 mov rdx, rsi
 call qword ptr [rip+__imp_GetProcAddress]
 lea rcx, [rip+.LxeWinWs]
 mov qword ptr [rcx+rdi*8], rax
.LwsWsSkip:
 cmp byte ptr [rsi], 0
 je .LwsWsNext
 inc rsi
 jmp .LwsWsSkip
.LwsWsNext:
 inc rsi
 inc edi
 cmp edi, 18
 jb .LwsWsLoop
 mov ecx, 0x202
 lea rdx, [rip+.LxeWinWsData]
 call qword ptr [rip+.LxeWinWs]
 mov qword ptr [rip+.LxeWinWsReady], 1
.LwsWsInitEnd:
 add rsp, 40
 pop rsi
 pop rdi
.LwsWsInitDone:
 ret

# is descriptor rdi one of our sockets? -> eax
.LwsIsSocket:
 cmp rdi, 2
 jbe .LwsIsNo
 cmp qword ptr [rip+.LxeWinSocketCount], 0
 je .LwsIsNo
 lea rcx, [rip+.LxeWinSockets]
 mov eax, 256
.LwsIsLoop:
 cmp qword ptr [rcx], rdi
 je .LwsIsYes
 add rcx, 8
 dec eax
 jnz .LwsIsLoop
 ret
.LwsIsNo:
 xor eax, eax
 ret
.LwsIsYes:
 mov eax, 1
 ret

.LwsSockAdd:
 lea rcx, [rip+.LxeWinSockets]
 xor edx, edx
.LwsSaLoop:
 cmp qword ptr [rcx+rdx*8], 0
 je .LwsSaSet
 inc edx
 cmp edx, 256
 jb .LwsSaLoop
 ret
.LwsSaSet:
 mov qword ptr [rcx+rdx*8], rax
 inc qword ptr [rip+.LxeWinSocketCount]
 ret

.LwsSockDel:
 lea rcx, [rip+.LxeWinSockets]
 xor edx, edx
.LwsSdLoop:
 cmp qword ptr [rcx+rdx*8], rdi
 je .LwsSdSet
 inc edx
 cmp edx, 256
 jb .LwsSdLoop
 ret
.LwsSdSet:
 mov qword ptr [rcx+rdx*8], 0
 dec qword ptr [rip+.LxeWinSocketCount]
 ret

.LwsSockWrite:
 mov rcx, rdi
 mov rdx, rsi
 mov r8, qword ptr [rbp-16]
 xor r9d, r9d
 call qword ptr [rip+.LxeWinWs+48]
 movsxd rax, eax
 test rax, rax
 jns .LwsDone
 mov rax, -5
 jmp .LwsDone

.LwsSockRead:
 mov rcx, rdi
 mov rdx, rsi
 mov r8, qword ptr [rbp-16]
 xor r9d, r9d
 call qword ptr [rip+.LxeWinWs+56]
 movsxd rax, eax
 test rax, rax
 jns .LwsDone
 mov rax, -5
 jmp .LwsDone

.LwsSocketClose:
.LwsCloseSock:
 call .LwsSockDel
 mov rcx, rdi
 call qword ptr [rip+.LxeWinWs+80]
 xor eax, eax
 jmp .LwsDone

# socket(domain, type, protocol)
.LwsSocket:
 call .LwsWsInit
 mov rcx, rdi
 mov r8, qword ptr [rbp-16]
 mov rdx, rsi
 and edx, 15
 call qword ptr [rip+.LxeWinWs+8]
 cmp rax, -1
 je .LwsSockFail
 mov qword ptr [rsp+56], rax
 call .LwsSockAdd
 test esi, 0x800
 jz .LwsSocketReady
 mov dword ptr [rsp+48], 1
 mov rcx, qword ptr [rsp+56]
 mov edx, 0x8004667E
 lea r8, [rsp+48]
 call qword ptr [rip+.LxeWinWs+136]
.LwsSocketReady:
 mov rax, qword ptr [rsp+56]
 jmp .LwsDone
.LwsSockFail:
 mov rax, -97
 jmp .LwsDone

# connect / accept / bind / getsockname / getpeername: (socket, pointer, pointer or length)
.LwsSock3Connect:
 mov dword ptr [rsp+48], 2
 jmp .LwsSock3
.LwsSock3Accept:
 mov dword ptr [rsp+48], 3
 jmp .LwsSock3
.LwsSock3Bind:
 mov dword ptr [rsp+48], 4
 jmp .LwsSock3
.LwsSock3Getsockname:
 mov dword ptr [rsp+48], 11
 jmp .LwsSock3
.LwsSock3Getpeername:
 mov dword ptr [rsp+48], 12
 jmp .LwsSock3
.LwsSock3:
 call .LwsWsInit
 mov r8, qword ptr [rbp-16]
 mov rcx, rdi
 mov rdx, rsi
 mov eax, dword ptr [rsp+48]
 lea r11, [rip+.LxeWinWs]
 call qword ptr [r11+rax*8]
 cmp dword ptr [rsp+48], 3
 je .LwsSock3Accepted
 movsxd rax, eax
 jmp .LwsDone
.LwsSock3Accepted:
 cmp rax, -1
 je .LwsDone
 call .LwsSockAdd
 jmp .LwsDone

.LwsSock2Listen:
 mov dword ptr [rsp+48], 5
 jmp .LwsSock2
.LwsSock2Shutdown:
 mov dword ptr [rsp+48], 15
.LwsSock2:
 call .LwsWsInit
 mov rcx, rdi
 mov rdx, rsi
 mov eax, dword ptr [rsp+48]
 lea r11, [rip+.LxeWinWs]
 call qword ptr [r11+rax*8]
 movsxd rax, eax
 jmp .LwsDone

# sendto(fd, buf, len, flags, addr, addrlen) and recvfrom(fd, buf, len, flags, addr, addrlen)
.LwsSendto:
 mov dword ptr [rsp+48], 8
 jmp .LwsSendRecv
.LwsRecvfrom:
 mov dword ptr [rsp+48], 9
.LwsSendRecv:
 call .LwsWsInit
 mov rcx, rdi
 mov rdx, rsi
 mov r8, qword ptr [rbp-16]
 xor r9d, r9d
 mov rax, qword ptr [rbp-24]
 mov qword ptr [rsp+32], rax
 mov rax, qword ptr [rbp-32]
 mov qword ptr [rsp+40], rax
 mov eax, dword ptr [rsp+48]
 lea r11, [rip+.LxeWinWs]
 call qword ptr [r11+rax*8]
 movsxd rax, eax
 jmp .LwsDone

# setsockopt(fd, level, option, value, length): SOL_SOCKET and its common options have other numbers on Windows
.LwsSetsockopt:
 call .LwsWsInit
 mov rcx, rdi
 mov rdx, rsi
 mov r8, qword ptr [rbp-16]
 cmp edx, 1
 jne .LwsSoReady
 mov edx, 0xFFFF
 cmp r8d, 2
 jne .LwsSo1
 mov r8d, 4
 jmp .LwsSoReady
.LwsSo1:
 cmp r8d, 9
 jne .LwsSo2
 mov r8d, 8
 jmp .LwsSoReady
.LwsSo2:
 cmp r8d, 6
 jne .LwsSo3
 mov r8d, 0x20
 jmp .LwsSoReady
.LwsSo3:
 cmp r8d, 7
 jne .LwsSo4
 mov r8d, 0x1001
 jmp .LwsSoReady
.LwsSo4:
 cmp r8d, 8
 jne .LwsSoReady
 mov r8d, 0x1002
.LwsSoReady:
 mov r9, qword ptr [rbp-40]
 mov rax, qword ptr [rbp-24]
 mov qword ptr [rsp+32], rax
 call qword ptr [rip+.LxeWinWs+104]
 movsxd rax, eax
 jmp .LwsDone

# poll(fds, count, timeout): Linux struct pollfd (8 bytes) <-> WSAPOLLFD (16 bytes); sockets only
.LwsPoll:
 call .LwsWsInit
 cmp rsi, 16
 ja .LwsPollBad
 xor ebx, ebx
.LwsPollIn:
 cmp rbx, rsi
 jae .LwsPollGo
 movsxd rax, dword ptr [rdi+rbx*8]
 mov rcx, rbx
 shl rcx, 4
 mov qword ptr [rsp+80+rcx], rax
 movzx eax, word ptr [rdi+rbx*8+4]
 xor edx, edx
 test eax, 1
 jz .LwsPollNoIn
 or edx, 0x100
.LwsPollNoIn:
 test eax, 4
 jz .LwsPollNoOut
 or edx, 0x10
.LwsPollNoOut:
 mov word ptr [rsp+88+rcx], dx
 mov word ptr [rsp+90+rcx], 0
 inc rbx
 jmp .LwsPollIn
.LwsPollGo:
 lea rcx, [rsp+80]
 mov rdx, rsi
 mov r8, qword ptr [rbp-16]
 movsxd r8, r8d
 call qword ptr [rip+.LxeWinWs+128]
 movsxd rax, eax
 test rax, rax
 js .LwsDone
 xor ebx, ebx
.LwsPollOut:
 cmp rbx, rsi
 jae .LwsDone
 mov rcx, rbx
 shl rcx, 4
 movzx edx, word ptr [rsp+90+rcx]
 xor r8d, r8d
 test edx, 0x300
 jz .LwsPollO1
 or r8d, 1
.LwsPollO1:
 test edx, 0x10
 jz .LwsPollO2
 or r8d, 4
.LwsPollO2:
 test edx, 1
 jz .LwsPollO3
 or r8d, 8
.LwsPollO3:
 test edx, 2
 jz .LwsPollO4
 or r8d, 16
.LwsPollO4:
 test edx, 4
 jz .LwsPollO5
 or r8d, 32
.LwsPollO5:
 mov word ptr [rdi+rbx*8+6], r8w
 inc rbx
 jmp .LwsPollOut
.LwsPollBad:
 mov rax, -22
 jmp .LwsDone

# sched_getaffinity(pid, size, mask): all processors of the machine
.LwsAffinity:
 lea rcx, [rsp+48]
 call qword ptr [rip+__imp_GetSystemInfo]
 mov ecx, dword ptr [rsp+80]
 mov eax, 1
 shl rax, cl
 dec rax
 mov rdx, qword ptr [rbp-16]
 cmp rcx, 64
 jb .LwsAffOk
 mov rax, -1
.LwsAffOk:
 mov qword ptr [rdx], rax
 mov eax, 8
 jmp .LwsDone

# readlink(path, buffer, size): only "/proc/self/exe" (the path of this program) is supported
.LwsReadlink:
 xor ecx, ecx
 mov rdx, rsi
 mov r8, qword ptr [rbp-16]
 call qword ptr [rip+__imp_GetModuleFileNameA]
 test eax, eax
 jnz .LwsDone
 mov rax, -2
 jmp .LwsDone

# kill(pid, signal): only the "is it alive" check (signal 0) for our own process is supported
.LwsKill:
 mov ebx, edi
 call qword ptr [rip+__imp_GetCurrentProcessId]
 cmp eax, ebx
 mov rax, -3
 jne .LwsDone
 xor eax, eax
 jmp .LwsDone

.LwsDone:
 movups xmm0, xmmword ptr [rsp+400]
 movups xmm1, xmmword ptr [rsp+416]
 movups xmm2, xmmword ptr [rsp+432]
 movups xmm3, xmmword ptr [rsp+448]
 movups xmm4, xmmword ptr [rsp+464]
 movups xmm5, xmmword ptr [rsp+480]
 lea rsp, [rbp-56]
 pop rsi
 pop rdi
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
.LxeWinSocketCount:
.quad 0
.LxeWinWsReady:
.quad 0
.LxeWinWs:
.skip 160
.LxeWinWsNames:
.asciz "WSAStartup"
.asciz "socket"
.asciz "connect"
.asciz "accept"
.asciz "bind"
.asciz "listen"
.asciz "send"
.asciz "recv"
.asciz "sendto"
.asciz "recvfrom"
.asciz "closesocket"
.asciz "getsockname"
.asciz "getpeername"
.asciz "setsockopt"
.asciz "getsockopt"
.asciz "shutdown"
.asciz "WSAPoll"
.asciz "ioctlsocket"
.LxeWinWsDll:
.asciz "ws2_32.dll"
.LxeWinWsData:
.skip 512
.LxeWinSockets:
.skip 2048
.LxeWinDirs:
.skip 10240
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
