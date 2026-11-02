# STM32F103C8T6 八路模拟循迹小车

本仓库包含基于 STM32 标准外设库（SPL）的模块化循迹程序。

## 文件说明
- `main.c`：主循环、PD 控制和丢线搜索
- `main.h`：引脚定义、速度、阈值和控制参数
- `bsp_motor.c/.h`：TB6612FNG 电机方向控制及 TIM1 PWM
- `bsp_sensor.c/.h`：ADC1 八通道扫描、DMA 和加权位置计算
- `pid.c/.h`：PD 控制器

## 默认引脚（必须按实物核对）
| 功能 | STM32 引脚 | 模块连接 |
|---|---|---|
| 传感器 OUT1~OUT8 | PA0~PA7 | ADC1 通道 0~7 |
| 左电机 PWM | PA8 / TIM1_CH1 | PWMA |
| 右电机 PWM | PA9 / TIM1_CH2 | PWMB |
| 左电机方向 | PB12、PB13 | AIN1、AIN2 |
| 右电机方向 | PB14、PB15 | BIN1、BIN2 |
| 驱动使能 | PB0 | STBY |

STM32、TB6612 逻辑地和传感器地必须共地。电机电源按驱动板规格接入，不能接到 STM32 3.3V。

## Keil MDK / SPL 配置
1. 使用 STM32F103C8T6 芯片配置和 SPL 工程。
2. 将 `main.c`、`bsp_motor.c`、`bsp_sensor.c`、`pid.c` 加入工程。
3. 将本目录加入头文件搜索路径。
4. 加入 SPL 对应的 CMSIS 启动文件和库源文件，确保 GPIO、RCC、ADC、DMA、TIM 等模块可用。
5. 本代码按系统时钟 72MHz 编写；确认启动文件和时钟配置一致。

## 首次调试顺序
1. 架空车轮，先验证电机方向和 PWM，避免小车突然冲出。
2. 用调试器观察 PA0~PA7 在白底和黑线上各自的 ADC 值。
3. 黑线 ADC 值高于白底时，`SENSOR_BLACK_HIGH=1`；黑线 ADC 值低于白底时改为 `0`。
4. 将 `SENSOR_THRESHOLD` 设在黑白读数之间。
5. 从较低 `BASE_SPEED` 开始，再调整 `PID_KP` 和 `PID_KD`。

## 注意
这是需要结合实际接线和传感器读数校准的代码框架，尚未在你的硬件上编译、烧录或验证。默认引脚、黑线极性和控制参数都需要确认。十字路口、全黑/全白等特殊赛道情形也可能需要额外策略。
