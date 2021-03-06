#include <linux/kallsyms.h>
#include "etc.h"
#include "hooking.h"

// table for holding all the requested hooks
static struct hook {
    unsigned long old_func;
    unsigned long new_func;
    int index;
} hook_table[MAX_HOOKS];
static int table_index;

// to lock hooking and adding hooks to the table after hook()
static int locked;

// extern unsigned long (*kallsyms_lookup_name)(const char *);
static unsigned long *syscall_table;
static int unhook(void);

int init_hooking(void) {
	memset(hook_table, 0, sizeof(hook_table));
	table_index = 0;
	locked = 0;

	// look up the syscall table symbol name to get address
	syscall_table = (unsigned long *) kallsyms_lookup_name("sys_call_table");

	return 0;
}

// unhook all syscalls and clear hook table memory
int exit_hooking(void) {
	int ret = unhook();
	if (ret) {
		memset(hook_table, 0, sizeof(hook_table));
	}

	return ret;
}

// adds a new syscall to the hooking function
// if already hooked, error.
int add_hook(unsigned long new_func,int index){
	// if no more space
	if (table_index == MAX_HOOKS) {
		return -1;
	}

	// if already hooked
	if (locked) {
		return -2;
	}

	// if same syscall already added to table
    for (int i = 0; i < table_index; i++){
        if (index == hook_table[i].index) {
            return -3;
        }
    }

	hook_table[table_index].old_func = syscall_table[index];
	hook_table[table_index].new_func = new_func;
	hook_table[table_index].index = index;
	table_index++;
    return 0;
}

// hooks everything in the hooking table.
// if already hooked, error.
int hook(void){
	int syscall_index;

	if (locked) {
		return 1;
	}

	// hook every syscall in table
	set_addr_rw((unsigned long) syscall_table);
    for(int i = 0; i < table_index; i++){
        syscall_index = hook_table[i].index;
        syscall_table[syscall_index] = hook_table[i].new_func;
    }
	set_addr_ro((unsigned long) syscall_table);

	locked = 1;

	return 0;
}

// unhooks everything in the hooking table.
// if didn't hook already, error.
static int unhook(void){
	if (!locked) {
		return 1;
	}

	// unhook every syscall in table
	set_addr_rw((unsigned long) syscall_table);
    for(int i = 0; i < table_index; i++){
        int syscall_index = hook_table[i].index;
        syscall_table[syscall_index] = hook_table[i].old_func;
    }
	set_addr_ro((unsigned long) syscall_table);

	return 0;
}

// return a syscall's address given index
t_syscall get_syscall(int index) {
	return (t_syscall) syscall_table[index];
}
