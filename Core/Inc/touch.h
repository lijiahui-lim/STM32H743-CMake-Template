#ifndef TOUCH_H
#define TOUCH_H

#include "stm32h7xx_hal.h"
#include "lvgl.h"

/* GT11xx I2C address (7-bit; INT pin held LOW during reset selects 0x14) */
#define GT1151_ADDR       0x14

/* Control pins */
#define TOUCH_RST_PORT    GPIOG
#define TOUCH_RST_PIN     GPIO_PIN_7
#define TOUCH_INT_PORT    GPIOG
#define TOUCH_INT_PIN     GPIO_PIN_3

/* GT11xx registers */
#define GT_REG_CMD        0x8040
#define GT_REG_CFG        0x8047
#define GT_REG_STATUS     0x814E
#define GT_REG_POINT1     0x814F
#define GT_REG_POINT_SIZE 8

#define TOUCH_MAX_POINTS  5

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t id;
} TouchPoint_t;

void Touch_Init(void);
int8_t Touch_GetPoints(TouchPoint_t *points, uint8_t max_points);
lv_indev_t *Touch_CreateLvglIndev(void);

#endif /* TOUCH_H */
