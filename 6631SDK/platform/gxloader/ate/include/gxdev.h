#ifndef __DEV_H__
#define __DEV_H__

#ifndef O_NONBLOCK
#define O_NONBLOCK 2
#endif

#define MAX_DEV 64
struct gx_devtab_entry;

struct gx_devio_ops {
	int (*ioctl) (struct gx_devtab_entry *entry, unsigned int key, void *buf);
	int (*read)  (struct gx_devtab_entry *entry, void *buf, unsigned int len);
	int (*write) (struct gx_devtab_entry *entry, const void *buf, unsigned int len);
	int (*open)  (struct gx_devtab_entry *entry,int access);
	int (*close) (struct gx_devtab_entry *entry);
};

struct gx_devtab_entry
{
	int id;
	char name[256];
	struct gx_devio_ops ops;
	void *priv;
};
#endif

