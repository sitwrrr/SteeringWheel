# BUG记录

> 记录开发过程中发现的bug及修复方法。

---

## 2026-06-24 编译错误修复

### BUG-001: app_can.c 重复case标签

**现象**：`duplicate case value: CAN_ID_BMS_CELL_VOLT`

**原因**：修复BMS解析时分两次编辑，两次都写了 `case CAN_ID_BMS_CELL_VOLT`，导致switch中有重复case。

**修复**：删除重复的case块，保留带赋值的那个。

**文件**：`APP/app_can.c`

---

### BUG-002: bsp_lcd.c abs()未声明

**现象**：`implicit declaration of function 'abs'`

**原因**：`BSP_LCD_DrawLine()` 使用了 `abs()` 但没有 `#include <stdlib.h>`。

**修复**：在 bsp_lcd.c 头部添加 `#include <stdlib.h>`。

**文件**：`BSP/bsp_lcd.c`

---

### BUG-003: bsp_usb.c CDC_Transmit函数名错误

**现象**：`implicit declaration of function 'CDC_Transmit'`

**原因**：CubeMX生成的USB CDC函数名是 `CDC_Transmit_FS`，不是 `CDC_Transmit`。

**修复**：`CDC_Transmit(NULL, data, len)` → `CDC_Transmit_FS(data, len)`（两处）。

**文件**：`BSP/bsp_usb.c`

---

### BUG-004: bsp_usb.c USBD_HID_Mouse_SendReport未定义

**现象**：`implicit declaration of function 'USBD_HID_Mouse_SendReport'`

**原因**：项目未集成USB HID类库，该函数不存在。

**修复**：暂时注释掉调用，添加TODO标记。后续集成HID类库后恢复。

**文件**：`BSP/bsp_usb.c`

---

### BUG-005: app_ec200.c sprintf格式不匹配

**现象**：`format '%d' expects argument of type 'int', but argument has type 'double'`

**原因**：sprintf格式串中 `%d` 对应的参数是 `0.0f`（float），类型不匹配。

**修复**：统一格式和参数类型，`%f` 对应 `0.0f`，`%d` 对应 `0`。

**文件**：`APP/app_ec200.c`

---

## 2026-06-24 CAN模块修复

### BUG-006: app_can.c 0x211安全位逻辑反

**现象**：`safety_loop = (data[4] & 0x01) ? 0 : 1` 逻辑与原项目不一致。

**原因**：原项目中 `IMD_Safe = data[4] & 0x01`（1=正常），新代码取反了。

**修复**：改为 `g_vehicleData.safety_loop = data[4] & 0x01`，与原项目一致。

**文件**：`APP/app_can.c`

---

### BUG-007: app_can.c 0x211缺少字段赋值

**现象**：car_travel、gear、steering_angle 只有注释没有赋值，运行时永远为0。

**原因**：编写时只加了注释占位，忘记写赋值代码。

**修复**：补全赋值：
```c
g_vehicleData.car_travel = data[3];
g_vehicleData.gear = (data[5] >> 5) & 0x03;
g_vehicleData.steering_angle = data[6] - 90;
```

**文件**：`APP/app_can.c`

---

### BUG-008: app_can.c BMS缺少字段赋值

**现象**：soh、bat_state、bat_alarm_level、bat_life、cell_voltage_max/min 只有注释没有赋值。

**原因**：同BUG-007。

**修复**：补全所有BMS字段赋值。

**文件**：`APP/app_can.c`

---

### BUG-009: app_can.c IMU姿态角未赋值

**现象**：roll、pitch、yaw 只有注释没有赋值。

**原因**：同BUG-007。

**修复**：补全姿态角赋值：
```c
g_vehicleData.roll = ((int16_t)(data[2] | data[3] << 8)) / 32768.0f * 180.0f;
g_vehicleData.pitch = ((int16_t)(data[4] | data[5] << 8)) / 32768.0f * 180.0f;
g_vehicleData.yaw = ((int16_t)(data[6] | data[7] << 8)) / 32768.0f * 180.0f;
```

**文件**：`APP/app_can.c`

---

### BUG-010: bsp_can.c DataLength写法不规范

**现象**：`txHeader.DataLength = len << 16`，数值正确但写法不规范。

**原因**：原项目使用 `FDCAN_DLC_BYTES_8` 宏定义，更清晰。

**修复**：改为 `txHeader.DataLength = FDCAN_DLC_BYTES_8`（两处Send函数）。

**文件**：`BSP/bsp_can.c`

---

## 2026-06-24 链接脚本修复

### BUG-011: 链接脚本DTCMRAM溢出

**现象**：`region 'DTCMRAM' overflowed by xxx bytes`

**原因**：CubeMX默认把.bss/.data/堆栈放在DTCMRAM（128KB），但FreeRTOS堆128KB + 全局变量 > 128KB。

**修复**：修改链接脚本5处：
1. `_estack` → `ORIGIN(RAM) + LENGTH(RAM)`
2. `_Min_Heap_Size` → `0x2000`
3. `.data` → `>RAM AT> FLASH`
4. `.bss` → `>RAM`
5. `._user_heap_stack` → `>RAM`

**文件**：`STM32H743VITx_FLASH.ld`

---

## 2026-06-24 头文件修复

### BUG-012: app_input.c KeyId_t/InputEvent_t未定义

**现象**：`unknown type name 'KeyId_t'`

**原因**：app_input.c 使用了 `KeyId_t` 和 `InputEvent_t` 但没有定义。

**修复**：在 app_input.c 头部添加这两个类型的定义（枚举）。

**文件**：`APP/app_input.c`

---

### BUG-013: bsp_usb.c usb_device.h找不到

**现象**：`fatal error: usb_device.h: No such file or directory`

**原因**：CubeMX未启用USB Device中间件。

**修复**：在CubeMX中启用USB_DEVICE，选择CDC类，重新Generate Code。

**文件**：CubeMX配置问题

---

## 2026-06-24 CAN接收方式优化

### BUG-014: CAN轮询方式可能丢帧

**现象**：多个ECU同时发送CAN消息时，20ms轮询周期内FIFO可能溢出导致丢帧。

**原因**：原实现使用轮询方式（`BSP_CAN1_Receive`），每20ms检查一次FIFO。VCU/MCU/BMS/IMU同时发数据时，FIFO深度仅64条，轮询延迟可能导致溢出。

**修复**：改为中断回调方式：
- 删除 `BSP_CAN1_Receive`/`BSP_CAN2_Receive` 轮询函数
- 新增 `HAL_FDCAN_RxFifo0Callback` 中断回调（BSP层）
- 新增 `BSP_CAN1_RxCallback`/`BSP_CAN2_RxCallback` 回调实现（APP层）
- 删除 `APP_CAN_Process` 轮询函数

**文件**：`BSP/bsp_can.h`、`BSP/bsp_can.c`、`APP/app_can.h`、`APP/app_can.c`

---

## 待修复

| 编号 | 问题 | 文件 | 状态 |
|------|------|------|------|
| - | USB HID类库未集成，`BSP_USB_SendHIDReport`暂不可用 | bsp_usb.c | TODO |
