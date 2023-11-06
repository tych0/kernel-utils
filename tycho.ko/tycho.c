#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kstrtox.h>
#include <linux/slab.h>

extern int proc_tycho;

static struct proc_dir_entry *tycho = NULL;

static ssize_t read_tycho(struct file *file, char __user *buf, size_t count,
			  loff_t *ppos)
{
	char kbuf[64];
	int n;

	n = snprintf(kbuf, sizeof(kbuf), "%d\n", proc_tycho);
	if (n > count)
		return -EINVAL;

	if (*ppos >= n)
		return 0;

	if (copy_to_user(buf, kbuf, n))
		return -EFAULT;

	*ppos += n;
	return n;
}

static ssize_t write_tycho(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	char *val;
	int new, ret;

#define MAX 64
	if (count > MAX)
		return -EINVAL;
	val = memdup_user(buf, count);
	val[count-1] = 0;

	ret = kstrtoint(val, 0, &new);
	kfree(val);
	if (ret < 0)
		return ret;
	proc_tycho = new;
	return count;
}

const struct proc_ops tycho_ops = {
	.proc_read = read_tycho,
	.proc_write = write_tycho,
	.proc_lseek = default_llseek,
};

static int __init custom_init(void) {
	proc_tycho = 0;

	tycho = proc_create("tycho", 0666, NULL, &tycho_ops);
	if (!tycho) {
		pr_err("failed to create procfs node");
		return -1;
	}

	return 0;
}

static void __exit custom_exit(void) {
	if (tycho)
		proc_remove(tycho);
}
module_init(custom_init);
module_exit(custom_exit);
MODULE_AUTHOR("Tycho Andersen");
MODULE_DESCRIPTION("Fault injection for dummies");
MODULE_LICENSE("GPL");
