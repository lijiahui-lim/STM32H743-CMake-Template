#include "touch.h"

extern I2C_HandleTypeDef hi2c2;

#define GT1151_ADDR_ALT   0x5D

static uint8_t gt_addr = GT1151_ADDR;

static HAL_StatusTypeDef GT1151_WriteReg(uint16_t reg, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Write(&hi2c2, gt_addr << 1, reg,
                             I2C_MEMADD_SIZE_16BIT, data, len, 100);
}

static HAL_StatusTypeDef GT1151_ReadReg(uint16_t reg, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c2, gt_addr << 1, reg,
                            I2C_MEMADD_SIZE_16BIT, data, len, 100);
}

static void Touch_Reset(uint8_t int_level)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* INT level during reset release latches the I2C address:
       LOW -> 0x14, HIGH -> 0x5D */
    GPIO_InitStruct.Pin = TOUCH_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(TOUCH_INT_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(TOUCH_INT_PORT, TOUCH_INT_PIN,
                      int_level ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* Double reset sequence (reference design) */
    HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);

    HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(80);

    /* INT back to floating input */
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(TOUCH_INT_PORT, &GPIO_InitStruct);

    HAL_Delay(50);
}

/* Returns the 7-bit address if a GT11xx ACKs, 0 otherwise */
static uint8_t Touch_Probe(void)
{
    if (HAL_I2C_IsDeviceReady(&hi2c2, GT1151_ADDR << 1, 3, 200) == HAL_OK)
        return GT1151_ADDR;
    if (HAL_I2C_IsDeviceReady(&hi2c2, GT1151_ADDR_ALT << 1, 3, 200) == HAL_OK)
        return GT1151_ADDR_ALT;
    return 0;
}

void Touch_Init(void)
{
    /* INT-low reset should latch 0x14; if nothing answers, retry with INT high */
    Touch_Reset(0);
    gt_addr = Touch_Probe();
    if (gt_addr == 0) {
        Touch_Reset(1);
        gt_addr = Touch_Probe();
    }
}

uint8_t Touch_GetPoints(TouchPoint_t *points, uint8_t max_points)
{
    uint8_t status = 0;
    uint8_t buf[GT_REG_POINT_SIZE * TOUCH_MAX_POINTS];
    uint8_t clear = 0;
    uint8_t count;

    if (GT1151_ReadReg(GT_REG_STATUS, &status, 1) != HAL_OK)
        return 0;

    if ((status & 0x80) == 0)
        return 0;

    count = status & 0x0F;
    if (count > max_points) count = max_points;
    if (count > TOUCH_MAX_POINTS) count = TOUCH_MAX_POINTS;

    if (count > 0)
    {
        if (GT1151_ReadReg(GT_REG_POINT1, buf, count * GT_REG_POINT_SIZE) != HAL_OK)
            return 0;
        for (uint8_t i = 0; i < count; i++)
        {
            uint8_t *p = &buf[i * GT_REG_POINT_SIZE];
            points[i].id = p[0] & 0x0F;
            points[i].x = (uint16_t)(p[1]) | ((uint16_t)(p[2]) << 8);
            points[i].y = (uint16_t)(p[3]) | ((uint16_t)(p[4]) << 8);
        }
    }

    /* Clear the status register so the next event is reported */
    GT1151_WriteReg(GT_REG_STATUS, &clear, 1);

    return count;
}

static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    TouchPoint_t point;
    uint8_t count = Touch_GetPoints(&point, 1);

    if (count > 0)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = point.x;
        data->point.y = point.y;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

lv_indev_t * Touch_CreateLvglIndev(void)
{
    lv_indev_t *indev = lv_indev_create();
    if (indev == NULL)
        return NULL;
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read);
    return indev;
}
