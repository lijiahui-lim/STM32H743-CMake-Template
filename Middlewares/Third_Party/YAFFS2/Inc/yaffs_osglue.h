#ifndef YAFFS_OSGLUE_H
#define YAFFS_OSGLUE_H

#include "yaffs_cfg.h"
#include "yportenv.h"

/* Mount/unmount helpers */
int  yaffs_mount_nand(void);
int  yaffs_unmount_nand(void);

#endif
