/*
 * YAFFS2 NAND driver for STM32H743 HAL
 * Maps YAFFS2 chunk operations to HAL_NAND calls.
 */

#include "yaffs_stm32_nandif.h"
#include "yaffs_guts.h"
#include "yaffs_trace.h"
#include "main.h"

#include <string.h>

/* NAND geometry - must match hardware */
#define NAND_PAGE_SIZE        2048
#define NAND_SPARE_SIZE       64
#define NAND_PAGES_PER_BLOCK  64
#define NAND_TOTAL_BLOCKS     1024

static void chunk_to_addr(int chunk_id, NAND_AddressTypeDef *addr)
{
	addr->Page  = chunk_id % NAND_PAGES_PER_BLOCK;
	addr->Block = chunk_id / NAND_PAGES_PER_BLOCK;
	addr->Plane = 0;
}

static int stm32_nand_read_chunk(struct yaffs_dev *dev, int nand_chunk,
				 u8 *data, int data_len,
				 u8 *oob, int oob_len,
				 enum yaffs_ecc_result *ecc_result)
{
	NAND_AddressTypeDef addr;
	chunk_to_addr(nand_chunk, &addr);

	if (data && data_len > 0) {
		if (HAL_NAND_Read_Page_8b(&hnand1, &addr, data, 1) != HAL_OK)
			return YAFFS_FAIL;
	}

	if (oob && oob_len > 0) {
		if (HAL_NAND_Read_SpareArea_8b(&hnand1, &addr, oob, 1) != HAL_OK)
			return YAFFS_FAIL;
	}

	if (ecc_result)
		*ecc_result = YAFFS_ECC_RESULT_NO_ERROR;

	return YAFFS_OK;
}

static int stm32_nand_write_chunk(struct yaffs_dev *dev, int nand_chunk,
				  const u8 *data, int data_len,
				  const u8 *oob, int oob_len)
{
	NAND_AddressTypeDef addr;
	chunk_to_addr(nand_chunk, &addr);

	if (data && data_len > 0) {
		if (HAL_NAND_Write_Page_8b(&hnand1, &addr, (uint8_t *)data, 1) != HAL_OK)
			return YAFFS_FAIL;
	}

	if (oob && oob_len > 0) {
		if (HAL_NAND_Write_SpareArea_8b(&hnand1, &addr, (uint8_t *)oob, 1) != HAL_OK)
			return YAFFS_FAIL;
	}

	return YAFFS_OK;
}

static int stm32_nand_erase(struct yaffs_dev *dev, int block_no)
{
	NAND_AddressTypeDef addr = { .Page = 0, .Block = block_no, .Plane = 0 };

	if (HAL_NAND_Erase_Block(&hnand1, &addr) != HAL_OK)
		return YAFFS_FAIL;

	return YAFFS_OK;
}

static int stm32_nand_check_bad(struct yaffs_dev *dev, int block_no)
{
	NAND_AddressTypeDef addr = { .Page = 0, .Block = block_no, .Plane = 0 };
	uint8_t spare[NAND_SPARE_SIZE];

	if (HAL_NAND_Read_SpareArea_8b(&hnand1, &addr, spare, 1) != HAL_OK)
		return 1; /* treat read failure as bad */

	return (spare[0] != 0xFF) ? 1 : 0;
}

static int stm32_nand_mark_bad(struct yaffs_dev *dev, int block_no)
{
	NAND_AddressTypeDef addr = { .Page = 0, .Block = block_no, .Plane = 0 };
	uint8_t spare[NAND_SPARE_SIZE];

	memset(spare, 0xFF, NAND_SPARE_SIZE);
	spare[0] = 0x00; /* bad block marker */

	if (HAL_NAND_Write_SpareArea_8b(&hnand1, &addr, spare, 1) != HAL_OK)
		return YAFFS_FAIL;

	return YAFFS_OK;
}

int yaffs_stm32_nandif_init(struct yaffs_dev *dev)
{
	struct yaffs_param *p = &dev->param;

	p->total_bytes_per_chunk = NAND_PAGE_SIZE;
	p->spare_bytes_per_chunk = NAND_SPARE_SIZE;
	p->chunks_per_block      = NAND_PAGES_PER_BLOCK;
	p->start_block           = 0;
	p->end_block             = NAND_TOTAL_BLOCKS - 1;
	p->is_yaffs2             = 1;
	p->n_reserved_blocks     = 10;
	p->inband_tags           = 0;
	p->no_tags_ecc           = 0;
	p->n_caches              = 10;

	dev->drv.drv_write_chunk_fn  = stm32_nand_write_chunk;
	dev->drv.drv_read_chunk_fn   = stm32_nand_read_chunk;
	dev->drv.drv_erase_fn        = stm32_nand_erase;
	dev->drv.drv_check_bad_fn    = stm32_nand_check_bad;
	dev->drv.drv_mark_bad_fn     = stm32_nand_mark_bad;
	dev->drv.drv_initialise_fn   = NULL;
	dev->drv.drv_deinitialise_fn = NULL;

	return YAFFS_OK;
}
