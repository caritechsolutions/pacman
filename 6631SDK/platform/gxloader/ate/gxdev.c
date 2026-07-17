#include "include/gxdev.h"
#include <stdio.h>
#include "string.h"
#include "fs.h"

#ifdef DBG
#define DEV_ERR(fmt, ...) printf("Dev fs ERR : " "func : <%s> line : <%d> " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DEV_ERR(fmt, ...)
#endif

static int dev_open(struct file_desc *fd_p, const char *name,int access);
static int dev_read(struct file_desc *fd_p, void *buf, size_t len);
static int dev_write(struct file_desc *fd_p, const void *buf, size_t len);
static int dev_ioctl(struct file_desc *fd_p, int key, void *buf);
static int dev_close(struct file_desc *fd_p);

static fstab_entry devfs_fte = {
	.name = DEVFS_NAME,
	.fo_open = &dev_open,
};

mtab_entry devfs_mte = {
	.fsname = DEVFS_NAME,
	.fs = &devfs_fte,
};

static file_ops devfs_op = {
    .fo_read    = &dev_read,
    .fo_write   = &dev_write,
    .fo_lseek   = NULL,
    .fo_ioctl   = &dev_ioctl,
    .fo_select  = NULL,
    .fo_close   = &dev_close,
    .fo_fstat   = NULL,
    .fo_getinfo = NULL,
    .fo_setinfo = NULL,
};

struct gx_devtab_entry gxdevs[MAX_DEV];

static int name_compare(const char *n1, const char *n2)
{
	if (*n2 != '/')
		return -1;

    while (*n1 && *n2) {
        if (*n1++ != *n2++) {
            return -1;
        }
    }
    return 0;
}

static int dev_open(struct file_desc *fd_p, const char *name,int access)
{
    struct gx_devtab_entry *p = NULL;
	int ret = 0;
    int i;

    for(i = 0; i < MAX_DEV; i++) {
		p = &gxdevs[i];
		if(0 == name_compare(name,p->name)) {
			ret = p->ops.open(p,access);
			break;
		}
    }

	if (i == MAX_DEV) {
		DEV_ERR("dev %s doesn't exist\n", name);
		return -1;
	}

	if (ret != 0)
		return ret;

	fd_p->f_ops = &devfs_op;
	fd_p->private = p;

    return 0;
}

static int dev_close(struct file_desc *fd_p)
{
	struct gx_devtab_entry *p = NULL;

	if (!fd_p || !fd_p->private) {
		DEV_ERR("invalid parameter\n");
		return -1;
	}

	p = (struct gx_devtab_entry *)fd_p->private;

	return gxdevs[p->id].ops.close(&gxdevs[p->id]);
}

static int dev_ioctl(struct file_desc *fd_p, int key, void *buf)
{
	struct gx_devtab_entry *p = NULL;

	if (!fd_p || !fd_p->private) {
		DEV_ERR("invalid parameter\n");
		return -1;
	}

	p = fd_p->private;

	return gxdevs[p->id].ops.ioctl(&gxdevs[p->id],key,buf);
}

static int dev_write(struct file_desc *fd_p, const void *buf, size_t len)
{
	struct gx_devtab_entry *p = NULL;

	if (!fd_p || !fd_p->private) {
		DEV_ERR("invalid parameter\n");
		return -1;
	}

	p = fd_p->private;

	return gxdevs[p->id].ops.write(&gxdevs[p->id],buf,len);
}

static int dev_read(struct file_desc *fd_p, void *buf, size_t len)
{
	struct gx_devtab_entry *p = NULL;

	if (!fd_p || !fd_p->private) {
		DEV_ERR("invalid parameter\n");
		return -1;
	}

	p = fd_p->private;

	return gxdevs[p->id].ops.read(&gxdevs[p->id],buf,len);
}

int dev_register(struct gx_devio_ops *dev, char *name,void *priv_dev)
{
	static int init = 0;
	int i;

	if(init == 0){
		init = 1;
		for (i = 0; i < MAX_DEV; i++)
			gxdevs[i].id = -1;
	}
	for (i = 0; i < MAX_DEV; i++){
		if(gxdevs[i].id == -1){
			gxdevs[i].id = i;
			break;
		}
	}
	if(i == MAX_DEV)
		return -1;
	if(dev && name){
		memcpy(gxdevs[i].name,name,strlen(name));
		memcpy((void *)&gxdevs[i].ops,(void*)dev,sizeof(struct gx_devio_ops));
		gxdevs[i].priv = priv_dev;
	}
	return 0;
}

void dev_unregister(char *name)
{
}
