/**
 ******************************************************************************
 * @file    cmsis_os2_compat.c
 * @brief   Missing CMSIS-RTOS2 APIs required by third-party middleware.
 ******************************************************************************
 */

#include "cmsis_os2.h"

/*
 * lv_thread_delete() (lvgl/src/osal/lv_cmsis_rtos2.c) calls osThreadDetach()
 * before terminating a thread, but the cmsis_os2.c wrapper shipped with
 * STM32Cube FW does not implement this optional API. The implementation is
 * kept here rather than patching cmsis_os2.c, which CubeMX regenerates.
 *
 * If a future CMSIS-RTOS2 wrapper adds its own osThreadDetach(), delete this
 * definition to avoid a duplicate-symbol link error.
 */
osStatus_t osThreadDetach(osThreadId_t thread_id)
{
    /* osThreadTerminate() in cmsis_os2.c already releases the thread's stack
       and TCB for dynamically allocated threads, so there is nothing to do. */
    (void)thread_id;
    return osOK;
}
