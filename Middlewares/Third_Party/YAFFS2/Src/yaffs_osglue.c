/*
 * YAFFS2 OS abstraction layer for FreeRTOS
 */

#include "yaffs_osglue.h"
#include "yaffs_cfg.h"
#include "yaffs_guts.h"
#include "yaffsfs.h"
#include "yaffs_stm32_nandif.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <string.h>

/* YAFFS2 global trace mask */
unsigned int yaffs_trace_mask;

static struct yaffs_dev yaffs_dev_nand;
static int yaffs_mounted;

static SemaphoreHandle_t yaffs_mutex;

void yaffsfs_Lock(void)
{
	if (!yaffs_mutex) {
		yaffs_mutex = xSemaphoreCreateRecursiveMutex();
		configASSERT(yaffs_mutex);
	}
	xSemaphoreTakeRecursive(yaffs_mutex, portMAX_DELAY);
}

void yaffsfs_Unlock(void)
{
	xSemaphoreGiveRecursive(yaffs_mutex);
}

u32 yaffsfs_CurrentTime(void)
{
	return (u32)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void yaffsfs_SetError(int err)
{
	(void)err;
}

void *yaffsfs_malloc(size_t size)
{
	return pvPortMalloc(size);
}

void yaffsfs_free(void *ptr)
{
	vPortFree(ptr);
}

void yaffsfs_get_malloc_values(unsigned *current, unsigned *high_water)
{
	if (current)
		*current = 0;
	if (high_water)
		*high_water = 0;
}

int yaffsfs_CheckMemRegion(const void *addr, size_t size, int write_request)
{
	(void)addr;
	(void)size;
	(void)write_request;
	return 1; /* always OK */
}

void yaffsfs_OSInitialisation(void)
{
	/* mutex created lazily in yaffsfs_Lock */
}

void yaffs_bug_fn(const char *file_name, int line_no)
{
	(void)file_name;
	(void)line_no;
	while (1)
		;
}

int yaffs_mount_nand(void)
{
	if (yaffs_mounted)
		return 0;

	memset(&yaffs_dev_nand, 0, sizeof(yaffs_dev_nand));
	yaffs_dev_nand.param.name = "nand";

	yaffs_stm32_nandif_init(&yaffs_dev_nand);

	yaffs_add_device(&yaffs_dev_nand);

	if (yaffs_mount("/nand") != 0) {
		yaffs_format("/nand", 0, 0, 0);
		if (yaffs_mount("/nand") != 0)
			return -1;
	}

	yaffs_mounted = 1;
	return 0;
}

int yaffs_unmount_nand(void)
{
	if (!yaffs_mounted)
		return 0;

	if (yaffs_unmount("/nand") != 0)
		return -1;

	yaffs_remove_device(&yaffs_dev_nand);
	yaffs_mounted = 0;
	return 0;
}
