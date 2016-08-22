# kernel-gc
Kernel Supported Garbage Collector

Directory Structure:

	gc/gc_module/gc.c      : Garbage Collection moudule

	gc/gc_test/gc_test.c   : Userspace Process for Testing [ $ <process name> [no of threads] [no of operations] ]
	gc/gc_test/gc.h        : Header file used by gc_test [ Helps userspace process in garbage collection ]
	gc/gc_test/gc.sh		   : Shell script for extracting process size after every 2 seconds

	gc/gc_syscall/gc.c		 : syscall files having all syscalls implemented [provided here simply to compare]
	
	linux-4.3.3-project-153050015.patch : patch for linux kernel source modifications made

	Modifications made in kernel:
		mm/mmap.c                | In function do_mmap(..) added mm_regn reuse feature
		inlcude/linux/mm_types.h | defined mm_regn structure, added per process address space list on mm_struct
		kernel/sched/core.c      | In schedule() added call to garbage collector if enabled

	For Execution:
		[] Apply patch: execute 'patch -p1 < ../linux-4.3.3-project-153050015.patch' from linux source director
		[] make module gc.c
		[] insmod module gc.c
		[] Run userspace process gc_test.c

	Note: A process must use gc.h as header file for using garbage collection
