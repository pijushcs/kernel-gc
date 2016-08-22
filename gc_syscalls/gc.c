#include<linux/kernel.h>
#include<linux/sched.h>
#include<linux/slab.h>
#include<linux/mm_types.h>
#include<linux/mm.h>
#include<linux/spinlock.h>

// Export function pointers to be used by gc module
void (*fp_gc_inc)(void *)=NULL;
void (*fp_gc_dec)(void *)=NULL;
void (*fp_gc_add)(void *,size_t)=NULL;
EXPORT_SYMBOL(fp_gc_inc);
EXPORT_SYMBOL(fp_gc_dec);
EXPORT_SYMBOL(fp_gc_add);

// Increment reference count
asmlinkage long sys_gc_inc(void *ptr){
	if(fp_gc_inc) fp_gc_inc(ptr);			// gc module must be loaded
	return 0;
}

// Decrement reference count
asmlinkage long sys_gc_dec(void *ptr){
	if(fp_gc_dec) fp_gc_dec(ptr);			// gc module must be loaded
	
	return 0;
}

// Register current process from garbage collection
asmlinkage long sys_gc_reg(void){
	struct list_head **mm_list=&(current->mm->mm_list);		//p->p declared as assignment is done
	*mm_list=kmalloc(sizeof(struct list_head),GFP_KERNEL);

	// Initialize mm_list for current process mm_region ref count list
	(*mm_list)->next=*mm_list; 
	(*mm_list)->prev=*mm_list;

	// Intialize spilock for process address space
	spin_lock_init(&(current->mm->w_lock));

	// Enable gc
	current->mm->enable_gc=1;

	return 0;
}

// add new mm_regn to per process ref-count list
asmlinkage long sys_gc_add(void *ptr,size_t size){
	if(fp_gc_add) fp_gc_add(ptr,size);				// gc module must be loaded

	return 0;
}
