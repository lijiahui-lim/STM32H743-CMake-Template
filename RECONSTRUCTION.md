# STM32H743 + LVGL 显示/触摸/YAFFS2 工程复现文档

本文档的目标：**在没有源代码的情况下**，仅凭公开可下载的工具和本文给出的全部参数，重建出功能与配置完全相同的工程。
所有关键寄存器值、时钟分频、时序参数、补丁位置均已列出，按章节顺序执行即可复现。

---

## 1. 硬件平台

| 部件 | 型号 / 参数 | 接口 |
|---|---|---|
| MCU | STM32H743VIT6（LQFP100，2MB Flash / 1MB RAM） | — |
| 开发板 | 野火 STM32H743 Pro | — |
| HSE 晶振 | 25 MHz | PH0/PH1 |
| LSE 晶振 | **未焊接**（RTC 必须用 LSI，否则时钟初始化死等 LSE 失败） | — |
| LCD | 800×480 RGB 接口屏（5/7 寸通用），RGB565 | LTDC，24-bit 并口 |
| 背光 | PD13，高电平点亮 | GPIO |
| 触摸 | GT1151（GT11xx 系列）电容触摸 | I2C2 + RST/INT |
| SDRAM | W9825G6KH-6，32-bit 总线，CAS=3 | FMC Bank2（SDNE1/SDCKE1） |
| NAND Flash | 2KB 页 + 64B OOB，64 页/块，1024 块，8-bit | FMC Bank3 |
| QSPI Flash | 32MB（FlashSize=24 → 2^25） | QUADSPI BK1 |
| 调试器 | CMSIS-DAP（OpenOCD 烧录） | USB |

### 引脚分配汇总

- **LTDC**（AF14）：PI12=HSYNC, PI13=VSYNC, PI14=CLK, PI15=R0；PJ0–PJ6=R1–R7, PJ7–PJ11=G0–G4, PJ12–PJ15=B0–B3；PK0–PK2=G5–G7, PK3–PK6=B4–B7, PK7=DE
- **FMC SDRAM**（AF12）：PC0=SDNWE；PD0/1=D2/D3, PD8/9/10=D13/14/15, PD14/15=D0/D1；PE0/1=NBL0/1, PE7–PE15=D4–D12；PF0–PF5=A0–A5, PF11=SDNRAS, PF12–PF15=A6–A9；PG0/1/2=A10/11/12, PG4/5=BA0/BA1, PG8=SDCLK, PG15=SDNCAS；PH6=SDNE1, PH7=SDCKE1, PH8–PH15=D16–D23；PI0–PI3=D24–D27, PI4/5=NBL2/3, PI6/7=D28/29, PI9/10=D30/31
- **FMC NAND**（AF12）：PD4=NOE, PD5=NWE, PD6=NWAIT, PD11=CLE, PD12=ALE, PG9=NCE
- **I2C2**（AF4，开漏+上拉）：PH4=SCL, PH5=SDA
- **触摸 GPIO**：PG3=INT（输入，EXTI3 下降沿），PG7=RST（推挽输出，下拉，初始低）
- **USART1**（AF7）：PA9=TX, PA10=RX，115200-8N1
- **QUADSPI**：PB2=CLK(AF9), PG6=NCS(AF10), PF6/7=IO3/IO2(AF9), PF8/9=IO0/IO1(AF10)
- **USB_OTG_HS**（内置 FS PHY，AF12）：PB14=DM, PB15=DP
- **背光**：PD13 推挽输出

---

## 2. 需要公开下载的工具与组件

| 组件 | 版本 | 获取方式 |
|---|---|---|
| STM32CubeMX | 6.18.1 | st.com 官网 |
| STM32CubeH7 固件包 | 与 CubeMX 6.18.1 匹配 | CubeMX 内下载（含 HAL、FreeRTOS、CMSIS-RTOS2、LibJPEG） |
| arm-none-eabi-gcc | 11.2-2022.02 | developer.arm.com（安装到 `/opt/gcc-arm-11.2-2022.02-x86_64-arm-none-eabi`） |
| CMake | ≥ 3.22 | cmake.org |
| OpenOCD | 任意近期版本 | openocd.org / 发行版包 |
| LVGL | **v9.6.0** | `git clone https://gitee.com/mirrors/lvgl.git`（或 github.com/lvgl/lvgl） |
| YAFFS2 | 上游 master | `git clone https://github.com/Aleph-One-Ltd/yaffs2.git` |

LVGL 与 YAFFS2 源码**不入库**，由 CMake 在配置阶段按需 clone（见第 9 节）。

---

## 3. CubeMX 工程配置（等价于 .ioc）

新建工程选择 STM32H743VITx，工具链选 **CMake**（CMakeLists 生成后会被手工扩展，CubeMX 不会重写顶层 CMakeLists.txt）。

### 3.1 时钟树（RCC）

- HSE：Crystal/Ceramic Resonator；**RTC Clock Mux：LSI**（板子无 32.768k 晶振）
- PLL1（源 HSE=25MHz）：M=5, N=192, P=2, **Q=20**, R=2 → SYSCLK=480MHz, PLL1Q=48MHz
- 总线：SYSCLK DIV1，AHB DIV2（240MHz），APB1/2/3/4 均 DIV2
- Flash Latency：4 WS；VOS：Scale 0；LDO 供电；CSS 使能（HSE）
- **PLL3（小数分频，LTDC 像素时钟，精确 60Hz）**：
  - M=25（VCO 输入 1MHz，RGE=1–2MHz 档），N=386，**FRACN=5710**，P=2, Q=8, **R=14**
  - VCO = (386 + 5710/8192) MHz ≈ 386.70MHz；像素时钟 = VCO/14 ≈ **27.6212MHz**
  - 校验：27.6212MHz ÷ (887 × 519) = **60.00Hz**
- USB 时钟源：**PLL1Q**（48MHz 精确）。PLL3Q 不再供 USB（VCO 不是 384MHz）
- FMC 时钟源：D1HCLK（240MHz）→ SDCLK = 120MHz

### 3.2 LTDC

- 时序（寄存器值为“累计值−1”，CubeMX 中直接填下列物理参数）：
  - HSync=1, HBP=46, HFP=40, ActiveW=800
  - VSync=3, VBP=23, VFP=13, ActiveH=480
  - 极性：HS/VS/DE 低有效，**Pixel Clock = LTDC_PCPOLARITY_IPC（上升沿，勿用 IIPC）**
- 生成代码后图层 0 是 0 尺寸的，需要在 USER CODE 里手工重配（见 §5.3）

### 3.3 FMC

- **NAND Bank3**：8-bit，Wait feature 使能，ECC 禁用，TCLR=2, TAR=2；页 2048，OOB 64，块 64 页 ×1024 块，1 plane；Common/Attribute 时序均为 Setup=2, WaitSetup=6, Hold=2, HiZ=6；ExtraCommand 使能
- **SDRAM Bank2**：Column=9, Row=13, 32-bit, 4 banks, CAS=3, 写保护禁用, **SDClock=HCLK/2**, Read Burst 使能, Read Pipe=1
  - 时序（SDCLK 周期数）：TMRD=2, TXSR=8, TRAS=5, TRC=8, **TWR=4**, TRP=2, TRCD=2
  - 刷新计数器 824（= 7.8µs × 120MHz − 20，代码中手工调用，见 §5.2）

### 3.4 其它外设

- **I2C2**：Timing = `0x90913232`（≈100kHz @ I2C 内核时钟），模拟滤波开，数字滤波 0
- **USART1**：115200, 8N1, 无流控
- **DMA2D**：M2M 模式，输出 ARGB8888，Layer1 输入 ARGB8888（仅供 LVGL 渲染引擎使用，中断在 USER CODE 里使能）
- **QUADSPI**：Prescaler=1, FlashSize=24, 半周期采样移位, CS High=5 cycles, Mode 0, ID1, 非双闪
- **USB_OTG_HS**：Device Only，内置 PHY，Full Speed，DMA 关，VBUS sensing 关
- **RNG**：时钟错误检测使能，时钟源 HSI48
- **RTC**：LSI，24h，AsynchPrediv=127, SynchPrediv=249（LSI≈32kHz）
- **Middleware → LIBJPEG**：使能（生成 `MX_LIBJPEG_Init()`，位于 LIBJPEG/App 目录）
- **FREERTOS**：CMSIS-RTOS **v2**；heap_4，`configTOTAL_HEAP_SIZE = 131072`（heap 数组被放到 RAM_D2，见 §7）；`configCHECK_FOR_STACK_OVERFLOW=2`、`configUSE_MALLOC_FAILED_HOOK=1`（钩子函数见 §5.6）；defaultTask 栈改为 **6144 words**
- **System Core → SYS**：时基改为 **TIM5**（FreeRTOS 占用 SysTick）
- **NVIC**：使能 TIM5、OTG_HS、EXTI3（PG3）、LTDC、DMA2D 中断。所有使用 FreeRTOS API 或 LVGL 同步对象的外设中断优先级必须 **≥ 6**（`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`）

### 3.5 MPU（CubeMX System Core → CORTEX_M7）

- Region 0：Base 0x0，Size 4GB，SubRegionDisable=**0x87**（屏蔽外设子区），TEX0，No Access，Shareable
- Region 1（SDRAM 帧缓冲）：Base **0xD0000000**，Size 64MB，TEX1，Priv RW，Instruction 允许，**Cacheable + Bufferable**（Write-Back），Not Shareable
- Region 2（NAND 0x80000000，256MB，Device 非缓存）：**CubeMX 里不配**，放在 `main()` 的 `USER CODE BEGIN 1` 中手工配置——因为 CubeMX 重新生成时会整体重写 `MPU_Config()`；MPU 区域寄存器是持久的，随后 `MPU_Config()` 重新使能 MPU 不会清掉 Region 2（详见 §5.1）

---

## 4. 生成代码后的文件布局

```
fire_stm32/
├── CMakeLists.txt          # CubeMX 生成 + 手工扩展（LVGL/YAFFS2/用户源）
├── cmake/stm32cubemx/      # CubeMX 生成的构建子工程
├── Core/{Inc,Src}/         # main.c、msp、it、freertos、touch、cmsis_os2_compat 等
├── Drivers/                # CubeMX 下载的 HAL / CMSIS / FreeRTOS / CMSIS-RTOS2
├── Middlewares/Third_Party/{FreeRTOS,LibJPEG,YAFFS2}
├── LIBJPEG/{App,Target}    # CubeMX LibJPEG 中间件
├── lvgl/                   # v9.6.0，git clone，不入库
├── yaffs2/                 # 上游 git clone，不入库
├── lv_conf.h               # 工程根目录，LVGL 配置
├── STM32H743xx_FLASH.ld    # 链接脚本（手工加 .ram_d2_heap 段）
├── startup_stm32h743xx.s
├── build.sh / flash.sh
└── fire_stm32.ioc
```

**纪律：所有手工代码必须写在 `USER CODE BEGIN/END` 区块内**，否则 CubeMX 重新生成时丢失。唯一例外是 `MPU_Config()` 整体重写问题（用 USER CODE 1 规避）、`stm32h7xx_hal_msp.c` 中 LTDC GPIO 速度、以及链接脚本/顶层 CMakeLists。

---

## 5. 手工补丁（全部位于 USER CODE 区块）

### 5.1 `main()` — USER CODE 1：NAND MPU 区域

在 `MPU_Config()` 调用**之前**配置 Region 2：

```c
{
  MPU_Region_InitTypeDef nand_region = {0};
  HAL_MPU_Disable();
  nand_region.Enable = MPU_REGION_ENABLE;
  nand_region.Number = MPU_REGION_NUMBER2;
  nand_region.BaseAddress = 0x80000000;
  nand_region.Size = MPU_REGION_SIZE_256MB;
  nand_region.SubRegionDisable = 0x00;
  nand_region.TypeExtField = MPU_TEX_LEVEL0;
  nand_region.AccessPermission = MPU_REGION_FULL_ACCESS;
  nand_region.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  nand_region.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  nand_region.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  nand_region.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  HAL_MPU_ConfigRegion(&nand_region);
}
```

### 5.2 `MX_FMC_Init()` — USER CODE FMC_Init 2：SDRAM 上电命令序列（点屏的第一前提）

`HAL_SDRAM_Init()` 只写控制器寄存器，**芯片必须收到命令序列才会进入 NORMAL 状态**，CubeMX 不生成：

```c
FMC_SDRAM_CommandTypeDef cmd = {0};

cmd.CommandMode   = FMC_SDRAM_CMD_CLK_ENABLE;      /* 1. 使能时钟 */
cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
HAL_SDRAM_SendCommand(&hsdram2, &cmd, 0xFFFF);
HAL_Delay(1);                                       /* ≥100µs */

cmd.CommandMode   = FMC_SDRAM_CMD_PALL;             /* 2. 全 bank 预充电 */
HAL_SDRAM_SendCommand(&hsdram2, &cmd, 0xFFFF);

cmd.CommandMode     = FMC_SDRAM_CMD_AUTOREFRESH_MODE; /* 3. 8 次自刷新 */
cmd.AutoRefreshNumber = 8;
HAL_SDRAM_SendCommand(&hsdram2, &cmd, 0xFFFF);

cmd.CommandMode  = FMC_SDRAM_CMD_LOAD_MODE;         /* 4. 模式寄存器 */
cmd.AutoRefreshNumber = 1;
cmd.ModeRegisterDefinition = 0x0230;  /* Burst=1, Sequential, CAS=3, 单写 */
HAL_SDRAM_SendCommand(&hsdram2, &cmd, 0xFFFF);

HAL_SDRAM_ProgramRefreshRate(&hsdram2, 824);

/* 清帧缓冲：LTDC 从第一帧起扫描到黑屏而不是 SDRAM 上电垃圾数据。
   FB_ADDR1 = 0xD0000000，两帧 RGB565 800×480，写后清 D-Cache */
for (int i = 0; i < 2; i++) {
    uint32_t *fb = (uint32_t *)(FB_ADDR1 + i * (800 * 480 * 2));
    for (uint32_t j = 0; j < (800 * 480 * 2) / 4; j++) fb[j] = 0;
    SCB_CleanDCache_by_Addr((uint32_t *)fb, 800 * 480 * 2);
}
```

### 5.3 `MX_LTDC_Init()` — USER CODE LTDC_Init 2：图层 0 + 中断

CubeMX 生成的两个图层是 0 尺寸占位，必须重配图层 0：

```c
hltdc.LayerCfg[0].WindowX0 = 0;      hltdc.LayerCfg[0].WindowX1 = 800;
hltdc.LayerCfg[0].WindowY0 = 0;      hltdc.LayerCfg[0].WindowY1 = 480;
hltdc.LayerCfg[0].PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
hltdc.LayerCfg[0].Alpha = 255;
hltdc.LayerCfg[0].FBStartAdress = 0xD0000000;
hltdc.LayerCfg[0].ImageWidth = 800;  hltdc.LayerCfg[0].ImageHeight = 480;
HAL_LTDC_ConfigLayer(&hltdc, &hltdc.LayerCfg[0], 0);

/* LVGL LTDC 驱动依赖寄存器重载中断做帧缓冲切换同步 */
HAL_NVIC_SetPriority(LTDC_IRQn, 6, 0);
HAL_NVIC_EnableIRQ(LTDC_IRQn);
```

### 5.4 `MX_DMA2D_Init()` — USER CODE DMA2D_Init 2

```c
HAL_NVIC_SetPriority(DMA2D_IRQn, 6, 0);
HAL_NVIC_EnableIRQ(DMA2D_IRQn);
```

### 5.5 `main()` — USER CODE SysInit / USER CODE 2

```c
/* SysInit：FreeRTOS heap 在 RAM_D2，必须开 D2 SRAM 时钟 */
__HAL_RCC_D2SRAM1_CLK_ENABLE();
__HAL_RCC_D2SRAM2_CLK_ENABLE();
__HAL_RCC_D2SRAM3_CLK_ENABLE();

/* USER CODE 2：背光 PD13 初始为低（关），首帧渲染完成后再点亮 */
```

### 5.6 其它 USER CODE

- **PV 区**：`ucHeap[configTOTAL_HEAP_SIZE]` 加 `__attribute__((section(".ram_d2_heap")))`（heap_4 不需要清零，NOLOAD 段即可）；声明 mountTask（栈 2048 words，osPriorityBelowNormal）
- **RTOS_THREADS**：`osThreadNew(StartMountTask, ...)` —— NAND/YAFFS 挂载可能很慢（或没焊 NAND 时永远不成功），不能阻塞 LVGL 任务
- **USER CODE 4**：`StartMountTask` 调 `yaffs_mount_nand()` 后空转；`vApplicationStackOverflowHook` / `vApplicationMallocFailedHook`（关中断死循环，便于调试器定位）
- **USER CODE 5（defaultTask）**——顺序有讲究：

```c
lv_init();
lv_tick_set_cb(HAL_GetTick);   /* 必须在 lv_init() 之后：lv_init 会清零全局状态 */
lv_st_ltdc_create_direct((void *)FB_ADDR1, (void *)FB_ADDR2, 0);  /* 双帧缓冲 direct 模式 */
Touch_Init();
Touch_CreateLvglIndev();
lv_demo_widgets();

lv_timer_handler();            /* 先渲染首帧，再开背光，避免闪垃圾画面 */
HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);

for (;;) { lv_timer_handler(); osDelay(5); }
```

> LVGL 必须在调度器启动后初始化：lv_st_ltdc 驱动会创建 CMSIS-RTOS2 同步对象。

### 5.7 `stm32h7xx_hal_msp.c`（CubeMX 会重写，注意每次重新生成后检查）

- LTDC 全部 GPIO（GPIOI/J/K）速度必须是 **GPIO_SPEED_FREQ_VERY_HIGH**（CubeMX 默认 LOW，27.6MHz 像素时钟下会花屏/无显示）
- I2C2 PH4/PH5：AF_OD + 上拉，VERY_HIGH

### 5.8 `stm32h7xx_it.c` — USER CODE 1

```c
void EXTI3_IRQHandler(void)  { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3); }

void DMA2D_IRQHandler(void)
{
  /* LVGL DMA2D 渲染器直接操作寄存器（非 HAL handle），完成中断在这里派发 */
  DMA2D->IFCR = 0x3FU;
  lv_draw_dma2d_transfer_complete_interrupt_handler();
}

void LTDC_IRQHandler(void)   { HAL_LTDC_IRQHandler(&hltdc); }
```

包含 `src/draw/dma2d/lv_draw_dma2d.h`；EV 区 `extern LTDC_HandleTypeDef hltdc; extern DMA2D_HandleTypeDef hdma2d;`

### 5.9 `Core/Src/cmsis_os2_compat.c`（新文件）

CubeMX 自带的 `cmsis_os2.c` 没实现可选 API `osThreadDetach()`，而 LVGL 的 `lv_thread_delete()` 会调用它。不能改 cmsis_os2.c（会被重写），单独建文件提供空实现（动态线程的栈/TCB 已由 `osThreadTerminate` 释放）：

```c
osStatus_t osThreadDetach(osThreadId_t thread_id) { (void)thread_id; return osOK; }
```

---

## 6. 触摸驱动（`Core/Src/touch.c` + `Core/Inc/touch.h`，新文件）

GT11xx 协议要点，全部自实现（HAL I2C）：

- **地址锁存**：复位释放瞬间 INT 脚电平决定 I2C 地址——低→`0x14`，高→`0x5D`。驱动先按 INT=LOW 做双次复位序列（RST 低 50ms→高 50ms→低 50ms→高 80ms），再 `HAL_I2C_IsDeviceReady` 探测 0x14；无应答则按 INT=HIGH 重复位再探测 0x5D
- **寄存器**：状态 `0x814E`（bit7=缓冲就绪，bit3:0=触点数），触点数据从 `0x814F` 起每点 8 字节（id, x_lo, x_hi, y_lo, y_hi, ...，小端）；读完向 `0x814E` 写 0 清除标志（同时释放 INT，重新武装中断）
- **事件模型**：PG3 配 EXTI 下降沿（新触摸报告到来时 GT11xx 拉低 INT），ISR 只置 `touch_event = true`；LVGL read_cb 中仅当事件置位或每 5 次轮询兜底（防漏边沿）时才走 I2C，其余时间返回缓存的上一次坐标/状态（静止手指不抖动）
- **总线超时**：所有 I2C 传输 50ms 超时；读失败保持上一状态，不卡 UI
- **LVGL 接入**：`lv_indev_create()` + `LV_INDEV_TYPE_POINTER` + read_cb

复位后把 INT 脚改回**上拉输入**：未接触摸屏时 INT 恒高，无下降沿，read_cb 自然跳过 I2C。

---

## 7. 链接脚本补丁（`STM32H743xx_FLASH.ld`）

DTCMRAM 放不下 128KB FreeRTOS heap，把 heap 放到 RAM_D2：

```ld
MEMORY
{
  ...
  RAM_D2 (xrw) : ORIGIN = 0x30000000, LENGTH = 288K
}

SECTIONS
{
  ...
  /* FreeRTOS heap placed in RAM_D2 */
  .ram_d2_heap (NOLOAD) :
  {
    . = ALIGN(8);
    *(.ram_d2_heap)
    *(.ram_d2_heap*)
    . = ALIGN(8);
  } >RAM_D2
}
```

---

## 8. `lv_conf.h`（工程根目录，基于 v9.6 模板改）

关键差异项：

```c
#define LV_COLOR_DEPTH 16
#define LV_MEM_SIZE (4 * 1024 * 1024)
#define LV_MEM_ADR 0xD0180000        /* LVGL 内存池放 SDRAM，帧缓冲之后 */
#define LV_USE_OS LV_OS_CMSIS_RTOS2
#define LV_USE_DRAW_DMA2D 1
#define LV_USE_DRAW_DMA2D_INTERRUPT 1
#define LV_USE_ST_LTDC 1
#define LV_ST_LTDC_USE_DMA2D_FLUSH 0  /* 渲染走 DMA2D draw unit，flush 不用 */
#define LV_USE_DEMO_WIDGETS 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_USE_LOG 0
```

SDRAM 地址规划：`0xD0000000` FB1（750KB）→ `0xD00BB800` FB2（750KB）→ `0xD0180000` LVGL 池（4MB）。

---

## 9. YAFFS2 集成

### 9.1 上游源码

clone `Aleph-One-Ltd/yaffs2` 到 `yaffs2/`（不入库）。编译以下文件为 OBJECT 库：

- `core/`: yaffs_allocator, yaffs_bitmap, yaffs_cache, yaffs_checkptrw, yaffs_ecc, yaffs_endian, yaffs_guts, yaffs_nameval, yaffs_nand, yaffs_packedtags1, yaffs_packedtags2, yaffs_summary, yaffs_tagscompat, yaffs_tagsmarshall, yaffs_verify, yaffs_yaffs1, yaffs_yaffs2
- `direct/`: yaffs_attribs, yaffs_error, yaffsfs, yaffs_hweight
- 加上自写胶水层（下）；头文件路径 `yaffs2/core`、`yaffs2/direct`、`Middlewares/Third_Party/YAFFS2/Inc`
- 编译选项：`-include yaffs_cfg.h`（强制前置配置头）、`-Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers`

### 9.2 `yaffs_cfg.h`（Inc/）

```c
#define CONFIG_YAFFS_DIRECT 1
#define CONFIG_YAFFS_YAFFS2 1
#define CONFIG_YAFFS_SHORT_NAMES_IN_RAM 1
#define CONFIG_YAFFS_DEFINES_TYPES 1
#define CONFIG_YAFFS_PROVIDE_DEFS 1
#define CONFIG_YAFFSFS_PROVIDE_VALUES 1
#define CONFIG_YAFFS_NO_TRACE 1
typedef long long loff_t;  /* 带 _LOFF_T_DEFINED 保护 */
typedef long off_t;        /* 带 _OFF_T_DEFINED 保护 */
```

### 9.3 `yaffs_osglue.c`（Src/，FreeRTOS OS 抽象层）

- `yaffsfs_Lock/Unlock`：递归互斥量（首次 Lock 时惰性创建）
- `yaffsfs_CurrentTime`：`xTaskGetTickCount() * portTICK_PERIOD_MS`
- `yaffsfs_malloc/free`：`pvPortMalloc/vPortFree`
- `yaffsfs_CheckMemRegion` 恒 OK；`yaffs_bug_fn` 死循环；`yaffs_trace_mask` 全局置 0
- `yaffs_mount_nand()`：填 `yaffs_dev`（name="nand"）→ `yaffs_stm32_nandif_init()` → `yaffs_add_device()` → `yaffs_mount("/nand")`，失败则 `yaffs_format("/nand",0,0,0)` 后重挂
- `yaffs_remove_device()` 在 yaffsfs.c 中有定义但无头文件声明，自行 `extern`

### 9.4 `yaffs_stm32_nandif.c`（Src/，chunk ↔ HAL_NAND 映射）

几何参数：chunk=2048B，OOB=64B，64 chunk/block，block 0–1023，`is_yaffs2=1`，`n_reserved_blocks=10`，`inband_tags=0`，`no_tags_ecc=0`，`n_caches=10`。

- chunk→地址：`Page = chunk % 64; Block = chunk / 64; Plane = 0`
- 读写 chunk：`HAL_NAND_Read/Write_Page_8b` + `HAL_NAND_Read/Write_SpareArea_8b`（OOB），ECC 结果恒报 `YAFFS_ECC_RESULT_NO_ERROR`（tags ECC 由 YAFFS 软件层做）
- 擦除：`HAL_NAND_Erase_Block`
- 坏块检查：读第 0 页 OOB，`spare[0] != 0xFF` 即坏（读失败也按坏处理）；标坏：OOB 全 0xFF 后写 `spare[0]=0`

---

## 10. 顶层 `CMakeLists.txt`（CubeMX 生成后手工扩展）

要点（CubeMX 不重写此文件）：

1. `set(CMAKE_PROJECT_NAME fire_stm32)`、`CMAKE_EXPORT_COMPILE_COMMANDS TRUE`
2. `add_subdirectory(cmake/stm32cubemx)`
3. **LVGL**：若 `lvgl/CMakeLists.txt` 不存在则 `execute_process` git clone gitee 镜像；`set(CONFIG_LV_BUILD_EXAMPLES OFF)`、`set(CONFIG_LV_BUILD_DEMOS ON)`（FORCE cache）后 `add_subdirectory(lvgl)`；给 `lvgl`、`lvgl_demos` 目标补 CMSIS/RTOS2/HAL/Core Inc 头文件路径及 `USE_HAL_DRIVER STM32H743xx` 宏
4. **YAFFS2**：不存在则 clone（可用 gh-proxy 前缀加速）；按 §9.1 建 OBJECT 库；`target_link_libraries(YAFFS2 PUBLIC stm32cubemx)`
5. 用户源：`Core/Src/cmsis_os2_compat.c`、`Core/Src/touch.c`
6. 最终链接：`stm32cubemx lvgl lvgl_demos YAFFS2`
7. `list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)`

`lv_conf.h` 通过 lvgl 的 CMake 机制自动取工程根目录（`LV_CONF_PATH` 默认行为：lvgl 目录的上一级）。

---

## 11. 构建与烧录

`build.sh`：

```bash
rm -rf build && mkdir build
export PATH="/opt/gcc-arm-11.2-2022.02-x86_64-arm-none-eabi/bin:$PATH"
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
      -DCMAKE_BUILD_TYPE=Debug   # 或 Release
cmake --build build -j$(nproc)
```

`flash.sh`（CMSIS-DAP + OpenOCD）：

```bash
sudo openocd -f interface/cmsis-dap.cfg -c "cmsis_dap_backend hid" \
  -f target/stm32h7x.cfg -c "adapter speed 2000" \
  -c "reset_config srst_only srst_nogate connect_assert_srst" \
  -c "program build/fire_stm32.elf verify reset" -c shutdown
```

---

## 12. 踩坑记录（复现时最容易翻车的地方）

| 症状 | 原因 | 对策 |
|---|---|---|
| 黑屏（背光亮） | SDRAM 没发初始化命令序列，0xD0000000 读写无效 | §5.2，CubeMX 不生成，必须手加 |
| LVGL 画面不刷新 | LTDC 重载中断未使能 / it.c 无 `LTDC_IRQHandler` | §5.3、§5.8 |
| 画面撕裂/偏移/无显示 | 像素时钟极性 IIPC（下降沿） | 改 `LTDC_PCPOLARITY_IPC` |
| 花屏、信号抖动 | LTDC GPIO 速度 LOW | msp 中改 VERY_HIGH（§5.7） |
| 开机死机进 Error_Handler | 板子没焊 32.768k 晶振却配了 LSE | RTC 时钟源改 LSI |
| LVGL tick 停走、动画冻结 | `lv_tick_set_cb` 在 `lv_init()` 之前调用被清零 | 先 `lv_init()` 再 set_cb |
| 链接失败 `osThreadDetach` 未定义 | CubeMX cmsis_os2.c 缺可选 API | §5.9 compat 文件 |
| 运行即 HardFault / MPU fault | FreeRTOS heap 默认在 DTCMRAM 放不下，或 NAND 区未按 Device 配 MPU | §5.1、§7 |
| LTDC/DMA2D/EXTI ISR 里调 RTOS API 崩溃 | 中断优先级高于 max syscall（5） | 全部设为 6 |
| 首帧前闪一屏垃圾数据 | SDRAM 上电内容随机 | §5.2 清帧缓冲 + 首帧后才开背光 |

---

## 13. 验证清单

1. 上电后屏幕先保持黑（背光关），约 1s 内背光点亮并显示 `lv_demo_widgets()` 界面
2. 触摸可点击 demo 控件（GT1151 地址自动探测 0x14/0x5D）
3. mountTask 挂载 `/nand` 成功后可用 yaffsfs API 读写文件；未焊 NAND 时系统其余部分不受影响
4. USART1 115200 可用于调试输出
5. 用 ST-Link/OpenOCD 读 `LTDC->LxCR[0]` 确认图层使能，读 FMC SDSR 确认 SDRAM 处于 NORMAL 模式（MODE2 位域 = 0）
