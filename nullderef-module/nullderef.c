/*
 * Copyright (C) 2010 Nelson Elhage
 *
 * Trivial kernel module to provoke a NULL pointer dereference.
 *
 * Based in part on provoke-crash.c
 * <http://thread.gmane.org/gmane.linux.kernel/942633>
 *
 * Author: Nelson Elhage <nelhage@ksplice.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 */
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/debugfs.h>
#include <linux/export.h>
#include <linux/moduleloader.h>
#include <linux/ftrace_event.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/elf.h>
#include <linux/proc_fs.h>
#include <linux/security.h>
#include <linux/seq_file.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/rcupdate.h>
#include <linux/capability.h>
#include <linux/cpu.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/vermagic.h>
#include <linux/notifier.h>
#include <linux/sched.h>
#include <linux/stop_machine.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/rculist.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>
#include <linux/license.h>
#include <asm/sections.h>
#include <linux/tracepoint.h>
#include <linux/ftrace.h>
#include <linux/async.h>
#include <linux/percpu.h>
#include <linux/kmemleak.h>
#include <linux/jump_label.h>
#include <linux/pfn.h>
#include <linux/bsearch.h>
#include <linux/fips.h>
#include <uapi/linux/module.h>

/*
 * Define an 'ops' struct containing a single mostly-pointless
 * function. We just do this to try to make this code look vaguely
 * like something that the actual kernel might contain.
 */
struct my_ops {
	ssize_t (*do_it)(void);
};

/* Define a pointer to our ops struct, "accidentally" initialized to NULL. */
struct my_ops *ops = NULL;

/*
 * Writing to the 'null_read' file just reads the value of the do_it
 * function pointer from the NULL ops pointer.
 */
static ssize_t null_read_write(struct file *f, const char __user *buf,
		size_t count, loff_t *off)
{
	f->private_data = ops->do_it;

	return count;
}


void dummy_function() {
	asm("before: nop;nop;nop;nop;");
	asm("suicide: call *0x0; retq");
	asm("after: nop;nop;nop;nop;");
}

void suicide();

/*
 * Writing to the 'null_read' file calls the do_it member of ops,
 * which results in reading a function pointer from NULL and then
 * calling it.
 */
static ssize_t null_call_write(struct file *f, const char __user *buf,
		size_t count, loff_t *off)
{
	suicide(1);
        return ops->do_it();
}


/* Handles to the files we will create */
static struct dentry *nullderef_root, *read_de, *call_de;

/* Structs telling the kernel how to handle writes to our files. */
static const struct file_operations null_read_fops = {
	.write = null_read_write,
};
static const struct file_operations null_call_fops = {
	.write = null_call_write,
};

/*
 * To clean up our module, we just remove the two files and the
 * directory.
 */
static void cleanup_debugfs(void) {
	if (read_de) debugfs_remove(read_de);
	if (call_de) debugfs_remove(call_de);
	if (nullderef_root) debugfs_remove(nullderef_root);
}

/*
 * This function is called at module load time, and creates the
 * directory in debugfs and the two files.
 */
static int __init nullderef_init(void)
{

	printk(KERN_INFO "info\n");
	printk(KERN_NOTICE "notice\n");
	printk(KERN_DEBUG "debug\n");
	printk(KERN_WARNING "warn\n");

	/* Create the directory our files will live in. */
	nullderef_root = debugfs_create_dir("nullderef", NULL);
	if (!nullderef_root) {
		printk(KERN_ERR "nullderef: creating root dir failed\n");
		return -ENODEV;
	}

	/*
	 * Create the null_read and null_call files. Use the fops
	 * structs defined above so that the kernel knows how to
	 * handle writes to them, and set the permissions to be
	 * writable by anyone.
	 */
	read_de = debugfs_create_file("null_read", 0222, nullderef_root,
				      NULL, &null_read_fops);
	call_de = debugfs_create_file("null_call", 0222, nullderef_root,
				      NULL, &null_call_fops);

	if (!read_de || !call_de)
		goto out_err;


	/* void *addr = suicide; */
	/* long size, offset; */
	/* char *modname; */
	/* char namebuf[128]; */

	//module_address_lookup((long)addr, &size, &offset, &modname, namebuf);

	//printk(KERN_DEFAULT "%s 0x%lx/0x%lx [%s]\n", namebuf, offset, size, modname);

	printk(KERN_DEFAULT "%pB\n", printk);
	printk(KERN_DEFAULT "%pB\n", (void*)((long)printk + 1));

	unsigned long a = kallsyms_lookup_name("printk");
	unsigned long b = printk;

	printk(KERN_DEFAULT "x: %pB, 0x%lx, 0x%lx\n", printk, a, b);

	char buf[128];
	sprint_symbol_no_offset(buf, (long)printk);
	printk(KERN_DEFAULT "x: %pB, 0x%lx, 0x%lx %s\n", printk, a, b, buf);


	return 0;
out_err:
	cleanup_debugfs();

	return -ENODEV;
}

/*
 * This function is called on module unload, and cleans up our files.
 */
static void __exit nullderef_exit(void)
{
	cleanup_debugfs();
}

/*
 * These two lines register the functions above to be called on module
 * load/unload.
 */
module_init(nullderef_init);
module_exit(nullderef_exit);







#if 0

static inline int is_arm_mapping_symbol(const char *str)
{
	return str[0] == '$' && strchr("atd", str[1])
	       && (str[2] == '\0' || str[2] == '.');
}

static const char *get_ksymbol(struct module *mod,
			       unsigned long addr,
			       unsigned long *size,
			       unsigned long *offset)
{
	unsigned int i, best = 0;
	unsigned long nextval;

	/* At worse, next value is at end of module */
	if (within_module_init(addr, mod))
		nextval = (unsigned long)mod->module_init+mod->init_text_size;
	else
		nextval = (unsigned long)mod->module_core+mod->core_text_size;

	/* Scan for closest preceding symbol, and next symbol. (ELF
	   starts real symbols at 1). */
	for (i = 1; i < mod->num_symtab; i++) {
		if (mod->symtab[i].st_shndx == SHN_UNDEF)
			continue;

		/* We ignore unnamed symbols: they're uninformative
		 * and inserted at a whim. */
		if (mod->symtab[i].st_value <= addr
		    && mod->symtab[i].st_value > mod->symtab[best].st_value
		    && *(mod->strtab + mod->symtab[i].st_name) != '\0'
		    && !is_arm_mapping_symbol(mod->strtab + mod->symtab[i].st_name))
			best = i;
		if (mod->symtab[i].st_value > addr
		    && mod->symtab[i].st_value < nextval
		    && *(mod->strtab + mod->symtab[i].st_name) != '\0'
		    && !is_arm_mapping_symbol(mod->strtab + mod->symtab[i].st_name))
			nextval = mod->symtab[i].st_value;
	}

	if (!best)
		return NULL;

	if (size)
		*size = nextval - mod->symtab[best].st_value;
	if (offset)
		*offset = addr - mod->symtab[best].st_value;
	return mod->strtab + mod->symtab[best].st_name;
}

/* For kallsyms to ask for address resolution.  NULL means not found.  Careful
 * not to lock to avoid deadlock on oopses, simply disable preemption. */
const char *module_address_lookup(unsigned long addr,
			    unsigned long *size,
			    unsigned long *offset,
			    char **modname,
			    char *namebuf)
{
	struct module *mod;
	const char *ret = NULL;

	preempt_disable();
	list_for_each_entry_rcu(mod, &modules, list) {
		if (mod->state == MODULE_STATE_UNFORMED)
			continue;
		if (within_module_init(addr, mod) ||
		    within_module_core(addr, mod)) {
			if (modname)
				*modname = mod->name;
			ret = get_ksymbol(mod, addr, size, offset);
			break;
		}
	}
	/* Make a copy in here where it's safe */
	if (ret) {
		strncpy(namebuf, ret, KSYM_NAME_LEN - 1);
		ret = namebuf;
	}
	preempt_enable();
	return ret;
}

int lookup_module_symbol_name(unsigned long addr, char *symname)
{
	struct module *mod;

	preempt_disable();
	list_for_each_entry_rcu(mod, &modules, list) {
		if (mod->state == MODULE_STATE_UNFORMED)
			continue;
		if (within_module_init(addr, mod) ||
		    within_module_core(addr, mod)) {
			const char *sym;

			sym = get_ksymbol(mod, addr, NULL, NULL);
			if (!sym)
				goto out;
			strlcpy(symname, sym, KSYM_NAME_LEN);
			preempt_enable();
			return 0;
		}
	}
out:
	preempt_enable();
	return -ERANGE;
}
#endif

MODULE_AUTHOR("Nelson Elhage <nelhage@ksplice.com>");
MODULE_DESCRIPTION("Provides debugfs files to trigger NULL pointer dereferences.");
MODULE_LICENSE("GPL");
