#pragma once
#include <stdint.h>
#include "stm32l4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* If your project already defines these in main.h, that's fine.
 * These #ifndef guards avoid duplicate definition warnings. */
#ifndef W5500_CS_GPIO_Port
#define W5500_CS_GPIO_Port   GPIOD
#define W5500_CS_Pin         GPIO_PIN_14
#endif

#ifndef W5500_RST_GPIO_Port
#define W5500_RST_GPIO_Port  GPIOD
#define W5500_RST_Pin        GPIO_PIN_15
#endif

#ifndef W5500_INT_GPIO_Port
#define W5500_INT_GPIO_Port  GPIOF
#define W5500_INT_Pin        GPIO_PIN_15
#endif

/* ===== Public API ===== */

/* Low-level: register SPI callbacks, split TX/RX buffers, HW reset.
 * Returns 0 on success, <0 on failure. */
int  w5500_chip_init(void);

/* Bring network up.
 * Tries DHCP first, and if it fails within a timeout, falls back to a static IP.
 * Prints the assigned (or static) address via printf(). */
void ethernet_start(void);

/* Print current MAC/IP/GW (uses printf). */
void net_print_info(void);

/* Minimal TCP echo service on port 5000.
 * Call this in your main loop (it blocks only while a client is connected). */
void tcp_echo_task(void);

/* Call this from HAL_GPIO_EXTI_Callback when the W5500 INT pin fires. */
void net_w5500_irq_handler(void);

#ifdef __cplusplus
}
#endif
