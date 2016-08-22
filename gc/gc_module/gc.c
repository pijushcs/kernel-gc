#include<linux/module.h>
#include<linux/sched.h>
#include<linux/kernel.h> 
#include<linux/init.h> 
#include<linux/unistd.h>
#include<linux/mm_types.h>
#include<linux/mm.h>
#include<linux/spinlock.h>
#include<linux/notifier.h>
#include<linux/slab.h>

/********************************************************
* Extern Declarations:                                  *
* Most of the declarations are used as a hook in the    *
* garbage collector associated system calls	and the     *
* garbage collector call itself from schedule()         *
********************************************************/
extern void (*fp_gc_dec)(void *);		// defined in [linux_source]/gc/gc.c
extern void (*fp_gc_inc)(void *);		// defined in [linux_source]/gc/gc.c
extern void (*fp_gc_sync)(void);		// defined in [linux_source]/kernel/sched/core.c
extern void (*fp_gc_add)(void*,size_t); // defined in [linux_source]/gc/gc.c
extern int mmap_gc_enable;				// defined in mmap.c for enabling mmap reuse

static BLOCKING_NOTIFIER_HEAD(munmap_notifier);	// used by blocking_notifier_call_chain(..)

/********************************************************
* Global Declarations:                                  *
* sync_thresh: Threshold parameter for defered gc call  *
* sync_call  : Iterator for sync_thresh                 *
* enable_mmap_gc : set mmap_enable_gc                   *
********************************************************/
int sync_thresh=0,enable_mmap_gc=0,sync_call=0; 

// Module paramaters
module_param(sync_thresh, int, 0);	
module_param(enable_mmap_gc, int, 0);	

/***********************************************************
* Garbage Collector:                                       *
* Called from schedule() when a process is being preempted *
***********************************************************/
void gc_sync(void){
	struct list_head *mm_list=(current->mm->mm_list), *list_p;
	struct mm_region *mm_p;		// defined in mm_types.h | holds refernce count per region

	// Check if threshold has reached
	if(enable_mmap_gc && sync_call<sync_thresh){
		sync_call++;
		return;
	}

	// Iterate over per process reference counter list
	list_for_each(list_p,mm_list){
		mm_p=list_entry(list_p,struct mm_region,list);

		// if reference count is 0, proceed to free!
		if( !(atomic_read(&(mm_p->mm_cnt)) ) ){
			spin_lock(&(mm_p->mm_lock));

			// unmap memory region from process address space
			if( !(atomic_read(&(mm_p->mm_cnt)) ) ){
				atomic_dec(&(mm_p->mm_cnt));
				printk("[gc mod free] %lx\n",mm_p->start);
			
				blocking_notifier_call_chain(&munmap_notifier, 0, (void *)(mm_p->start));
				vm_munmap(mm_p->start,mm_p->len);
			}

			spin_unlock(&(mm_p->mm_lock));
		}
	}
	
	sync_call=0;
	return;
}

/************************************************
* Add new mm_region to reference count list:    *
* Called when a new memory region is allocated  *
************************************************/
void gc_add(void* ptr, size_t len){
	struct list_head *mm_list=(current->mm->mm_list), *list_p;
	struct mm_region *mm_p,*old_p=NULL;
	unsigned long start=(unsigned long)ptr;

	// Check if the same address is already present is refernce count list
	list_for_each(list_p,mm_list){
		mm_p=list_entry(list_p,struct mm_region,list);
		if(mm_p->start==start){
			old_p=mm_p; 
			break;
		}
	}

	if(old_p){				
		// reuse old mm_region if found and reinitialize it
		mm_p=old_p;
		mm_p->start=start;
		mm_p->len=len;
		atomic_set(&(mm_p->mm_cnt),1);
	}
	else{					
		// create new mm_region and initialize it
		mm_p=kmalloc(sizeof(struct mm_region), GFP_KERNEL);
		mm_p->start=start;
		mm_p->len=len;
		atomic_set(&(mm_p->mm_cnt),1);
		spin_lock_init(&(mm_p->mm_lock));
		INIT_LIST_HEAD(&mm_p->list);

		// add new mm_region to per process ref-count list
		spin_lock(&(current->mm->w_lock));
		list_add_tail(&mm_p->list, mm_list);
		spin_unlock(&(current->mm->w_lock));
	}

	// Print Status [For checking address reuse]
	list_for_each(list_p,mm_list){
		mm_p=list_entry(list_p,struct mm_region,list);
		printk("[gc mod gcList] %lx ",mm_p->start);
	}
	printk("\n");
}		

/************************************************
* Increment reference count:                    *
************************************************/
void gc_inc(void *ptr){
	struct list_head *mm_list=(current->mm->mm_list), *list_p;
	struct mm_region *mm_p;
	unsigned long start=(unsigned long)ptr;

	list_for_each(list_p,mm_list){
		mm_p=list_entry(list_p,struct mm_region,list);

		if(mm_p->start==start){
			spin_lock(&(mm_p->mm_lock)); 

			if( (atomic_read(&(mm_p->mm_cnt)) ) )
				atomic_inc(&(mm_p->mm_cnt));		// increment atomically

			spin_unlock(&(mm_p->mm_lock));
		}
	}
}

/************************************************
* Decrement reference count:                    *
************************************************/
void gc_dec(void *ptr){
	struct list_head *mm_list=(current->mm->mm_list), *list_p;
	struct mm_region *mm_p;
	unsigned long start=(unsigned long)ptr;

	list_for_each(list_p,mm_list){
		mm_p=list_entry(list_p,struct mm_region,list);

		if(mm_p->start==start){
			spin_lock(&(mm_p->mm_lock));

			if( (atomic_read(&(mm_p->mm_cnt)) ) )
				atomic_dec(&(mm_p->mm_cnt));		// decrement atomically

			spin_unlock(&(mm_p->mm_lock));
		}
	}
}

/***************************
* Initialize Module	       *
***************************/
int init_module(void){
	printk("[gc mod] Started\n");

	// Set extern variables
	mmap_gc_enable=enable_mmap_gc;
	fp_gc_inc=gc_inc;
	fp_gc_dec=gc_dec;
	fp_gc_add=gc_add;
	fp_gc_sync=gc_sync;

	return 0;
}

/*************************
* Cleaeanup Module	     *
*************************/
void cleanup_module(void){
	// Reset extern variables
	mmap_gc_enable=0;
	fp_gc_inc=NULL;
	fp_gc_dec=NULL;
	fp_gc_add=NULL;
	fp_gc_sync=NULL;
}

MODULE_LICENSE("GPL");
