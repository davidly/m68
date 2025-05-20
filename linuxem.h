#pragma once

#define emulator_sys_rand               0x2000
#define emulator_sys_print_double       0x2001
#define emulator_sys_trace_instructions 0x2002
#define emulator_sys_exit               0x2003
#define emulator_sys_print_text         0x2004
#define emulator_sys_get_datetime       0x2005
#define emulator_sys_print_int64        0x2006
#define emulator_sys_print_char         0x2007

// Linux syscall numbers differ by ISA. InSAne. These are RISC and ARM64, which are the same!
// Note that there are differences between these two sets. which is correct?
// https://marcin.juszkiewicz.com.pl/download/tables/syscalls.html
// https://github.com/westerndigitalcorporation/RISC-V-Linux/blob/master/linux/arch/s390/kernel/syscalls/syscall.tbl
// https://blog.xhyeax.com/2022/04/28/arm64-syscall-table/
// https://syscalls.mebeim.net/?table=arm64/64/aarch64/latest

#define SYS_getcwd 17
#define SYS_fcntl 25
#define SYS_ioctl 29
#define SYS_mkdirat 34
#define SYS_unlinkat 35
#define SYS_renameat 38
#define SYS_faccessat 48
#define SYS_chdir 49
#define SYS_openat 56
#define SYS_close 57
#define SYS_getdents64 61
#define SYS_lseek 62
#define SYS_read 63
#define SYS_write 64
#define SYS_writev 66
#define SYS_pselect6 72   // or sigsuspend?
#define SYS_ppoll_time32 73
#define SYS_readlinkat 78
#define SYS_newfstatat 79
#define SYS_newfstat 80
#define SYS_fsync 82
#define SYS_fdatasync 83
#define SYS_exit 93
#define SYS_exit_group 94
#define SYS_set_tid_address 96
#define SYS_futex 98
#define SYS_set_robust_list 99
#define SYS_clock_gettime 113
#define SYS_clock_nanosleep 115
#define SYS_sched_setaffinity 122
#define SYS_sched_getaffinity 123
#define SYS_sched_yield 124
#define SYS_tgkill 131
#define SYS_signalstack 132
#define SYS_sigaction 134
#define SYS_rt_sigprocmask 135
#define SYS_times 153
#define SYS_uname 160
#define SYS_getrusage 165
#define SYS_prctl 167
#define SYS_gettimeofday 169
#define SYS_getpid 172
#define SYS_getuid 174
#define SYS_geteuid 175
#define SYS_getgid 176
#define SYS_getegid 177
#define SYS_gettid 178
#define SYS_sysinfo 179
#define SYS_brk 214
#define SYS_munmap 215
#define SYS_mremap 216
#define SYS_clone 220
#define SYS_mmap 222
#define SYS_mprotect 226
#define SYS_madvise 233
#define SYS_riscv_flush_icache 259 // not in docs; may be riscv only
#define SYS_prlimit64 261
#define SYS_renameat2 276
#define SYS_getrandom 278
#define SYS_rseq 293

// open apparently undefined for riscv? the old RISC-V64 g++ compiler/runtime uses these syscalls

#define SYS_open 1024
#define SYS_link 1025
#define SYS_unlink 1026
#define SYS_mkdir 1030
#define SYS_stat 1038
#define SYS_lstat 1039
#define SYS_time 1062


