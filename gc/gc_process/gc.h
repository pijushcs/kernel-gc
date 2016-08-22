#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>

// Define macro for system calls
#define gc_add(ptr,size) syscall(329,ptr,size)
#define gc_inc(ptr) syscall(326,ptr)
#define gc_dec(ptr) syscall(327,ptr)
#define gc_reg()	syscall(328)

// Allocate new memory region to *ptr
void gcalloc(void **ptr,size_t size){ 
	if(*ptr) gc_dec(*ptr);
	*ptr=mmap((void*)1,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,0,0);
	gc_add(*ptr,size);
}

// Assign ptr_a=ptr_b
void gcassign(void **ptr_a, void *ptr_b){
	if(ptr_b) gc_inc(ptr_b);
	if(*ptr_a) gc_dec(*ptr_a);
	*ptr_a=ptr_b;
}
