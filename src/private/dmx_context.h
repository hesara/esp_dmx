/**
 * @file dmx_hal.h
 * @author Mitch Weisbrod
 * @brief This file is the Hardware Abstraction Layer (HAL) of esp_dmx. It
 * contains low-level functions to perform tasks relating to the ESP32 UART
 * hardware. Many of these functions are labelled with IRAM_ATTR to place them
 * in Instruction RAM memory. This is done to every function that is called from
 * the DMX interrupt service routines. Functions labelled with IRAM_ATTR must
 * not call any functions that are not also labelled with IRAM_ATTR. Functions
 * called from IRAM HAL functions must be inlined if they are not also in IRAM.
 * Calling non-IRAM, non-inlined functions will cause the ESP32 to crash when
 * the DMX interrupt service routines are called while the ESP32 cache is also
 * disabled.
 */
#pragma once

#include "driver.h"
#include "hal/uart_hal.h"

#ifdef CONFIG_DMX_ISR_IN_IRAM
#define DMX_ISR_ATTR IRAM_ATTR
#else
#define DMX_ISR_ATTR
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the UART for DMX.
 *
 * @param hal A pointer to a UART HAL context.
 */
void dmx_uart_init(dmx_port_t dmx_num, spinlock_t *spinlock) {  // FIXME: remove ctx
  // Initialize the UART peripheral
  taskENTER_CRITICAL(spinlock);
  periph_module_enable(uart_periph_signal[dmx_num].module);
  if (dmx_num != CONFIG_ESP_CONSOLE_UART_NUM) {
#if SOC_UART_REQUIRE_CORE_RESET
    // ESP32-C3 workaround to prevent UART outputting garbage data.
    uart_ll_set_reset_core(uart, true);
    periph_module_reset(uart_periph_signal[dmx_num].module);
    uart_ll_set_reset_core(uart, false);
#else
    periph_module_reset(uart_periph_signal[dmx_num].module);
#endif
  }
  taskEXIT_CRITICAL(spinlock);

  uart_dev_t *const restrict uart = UART_LL_GET_HW(dmx_num);

  // Configure the UART for DMX output
  uart_ll_set_sclk(uart, UART_SCLK_APB);
  uart_ll_set_baudrate(uart, DMX_BAUD_RATE);
  uart_ll_set_mode(uart, UART_MODE_RS485_HALF_DUPLEX);
  uart_ll_set_parity(uart, UART_PARITY_DISABLE);
  uart_ll_set_data_bit_num(uart, UART_DATA_8_BITS);
  uart_ll_set_stop_bits(uart, UART_STOP_BITS_2);
  uart_ll_tx_break(uart, 0);
  uart_ll_set_tx_idle_num(uart, 0);
  uart_ll_set_hw_flow_ctrl(uart, UART_HW_FLOWCTRL_DISABLE, 0);

#if defined(CONFIG_IDF_TARGET_ESP32C3)
  // Fix inter-byte time on ESP32-C3. See below:
  // https://github.com/someweisguy/esp_dmx/issues/17#issuecomment-1133748359
  uart->rs485_conf.dl0_en = 0;
  uart->rs485_conf.dl1_en = 0;
#endif
}

/**
 * @brief Gets the UART baud rate of the selected UART hardware.
 *
 * @param hal A pointer to a UART HAL context.
 * @return The baud rate of the UART hardware.
 */
uint32_t dmx_uart_get_baud_rate(uart_dev_t *uart) {
  return uart_ll_get_baudrate(uart);
}

/**
 * @brief Sets the baud rate for the UART.
 *
 * @param hal A pointer to a UART HAL context.
 * @param baud_rate The baud rate to use.
 */
void dmx_uart_set_baud_rate(uart_dev_t *uart, uint32_t baud_rate) {
  uart_ll_set_baudrate(uart, baud_rate);
}

/**
 * @brief Sets the number of bytes that the UART must receive to trigger a RX
 * FIFO full interrupt.
 *
 * @param hal A pointer to a UART HAL context.
 * @param threshold The number of bytes needed to trigger an RX FIFO full
 * interrupt.
 */
void dmx_uart_set_rxfifo_full(uart_dev_t *uart, uint8_t threshold) {
  uart_ll_set_rxfifo_full_thr(uart, threshold);
}

/**
 * @brief Sets the number of bytes that the UART TX FIFO must have remaining in
 * it to trigger a TX FIFO empty interrupt.
 *
 * @param hal A pointer to a UART HAL context.
 * @param threshold The number of bytes remaining to trigger a TX FIFO empty
 * interrupt.
 */
void dmx_uart_set_txfifo_empty(uart_dev_t *uart, uint8_t threshold) {
  uart_ll_set_txfifo_empty_thr(uart, threshold);
}

/**
 * @brief Inverts or un-inverts the TX line on the UART.
 *
 * @param hal A pointer to a UART HAL context.
 * @param invert_mask 1 to invert, 0 to un-invert.
 */
DMX_ISR_ATTR void dmx_uart_invert_tx(uart_dev_t *uart, uint32_t invert) {
#if defined(CONFIG_IDF_TARGET_ESP32)
  uart->conf0.txd_inv = invert ? 1 : 0;
#elif defined(CONFIG_IDF_TARGET_ESP32C2)
#error ESP32-C2 is not yet supported.
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  uart->conf0.txd_inv = invert ? 1 : 0;
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
#error ESP32-H2 is not yet supported.
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  uart->conf0.txd_inv = invert ? 1 : 0;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  uart->uart_conf0_reg_t.txd_inv = invert ? 1 : 0;
#else
#error Unknown target hardware.
#endif
}

/**
 * @brief Gets the level of the UART RTS line.
 *
 * @param hal A pointer to a UART HAL context.
 * @return 1 if the UART RTS line is enabled (set low; read), 0 if the UART RTS
 * line is disable (set high; write).
 */
int dmx_uart_get_rts(uart_dev_t *uart) {
#if defined(CONFIG_IDF_TARGET_ESP32)
  return uart->conf0.sw_rts;
#elif defined(CONFIG_IDF_TARGET_ESP32C2)
#error ESP32-C2 is not yet supported.
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  return uart->conf0.sw_rts;
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
#error ESP32-H2 is not yet supported.
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  return uart->conf0.sw_rts;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  return uart->uart_conf0_reg_t.sw_rts;
#else
#error Unknown target hardware.
#endif
}

/**
 * @brief Gets the interrupt status mask from the UART.
 *
 * @param hal A pointer to a UART HAL context.
 * @return The interrupt status mask.
 */
DMX_ISR_ATTR int dmx_uart_get_interrupt_status(uart_dev_t *uart) {
  return uart_ll_get_intsts_mask(uart);
}

/**
 * @brief Enables UART interrupts using an interrupt mask.
 *
 * @param hal A pointer to a UART HAL context.
 * @param mask The UART mask that is enabled.
 */
DMX_ISR_ATTR void dmx_uart_enable_interrupt(uart_dev_t *uart, int mask) {
  uart_ll_ena_intr_mask(uart, mask);
}

/**
 * @brief Disables UART interrupts using an interrupt mask.
 *
 * @param hal A pointer to a UART HAL context.
 * @param mask The UART mask that is disabled.
 */
DMX_ISR_ATTR void dmx_uart_disable_interrupt(uart_dev_t *uart, int mask) {
  uart_ll_disable_intr_mask(uart, mask);
}

/**
 * @brief Clears UART interrupts using a mask.
 *
 * @param hal A pointer to a UART HAL context.
 * @param mask The UART mask that is cleared.
 */
DMX_ISR_ATTR void dmx_uart_clear_interrupt(uart_dev_t *uart, int mask) {
  uart_ll_clr_intsts_mask(uart, mask);
}

/**
 * @brief Gets the current length of the bytes in the UART RX FIFO.
 *
 * @param hal A pointer to a UART HAL context.
 * @return The number of bytes in the UART RX FIFO.
 */
DMX_ISR_ATTR uint32_t dmx_uart_get_rxfifo_len(uart_dev_t *uart) {
  return uart_ll_get_rxfifo_len(uart);
}

/**
 * @brief Gets the level of the UART RX line.
 *
 * @param hal A pointer to a UART HAL context.
 * @return The UART RX line level.
 */

DMX_ISR_ATTR uint32_t dmx_uart_get_rx_level(uart_dev_t *uart) {
#if defined(CONFIG_IDF_TARGET_ESP32)
  return uart->status.rxd;
#elif defined(CONFIG_IDF_TARGET_ESP32C2)
#error ESP32-C2 is not yet supported.
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  return uart->status.rxd;
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
#error ESP32-H2 is not yet supported.
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  return uart->status.rxd;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  return uart->uart_status_reg_t.rxd;
#else
#error Unknown target hardware.
#endif
}

/**
 * @brief Reads from the UART RX FIFO.
 *
 * @param hal A pointer to a UART HAL context.
 * @param buf Destination buffer to be read into.
 * @param num The maximum number of characters to read. Set to 0 to read all
 * data.
 * @return The number of characters read.
 */
DMX_ISR_ATTR void dmx_uart_read_rxfifo(uart_dev_t *uart, uint8_t *buf,
                                       int *size) {
  const size_t rxfifo_len = uart_ll_get_rxfifo_len(uart);
  if (*size > rxfifo_len) {
      *size = rxfifo_len;
  }
  uart_ll_read_rxfifo(uart, buf, *size);
}

/**
 * @brief Enables or disables the UART RTS line.
 *
 * @param hal A pointer to a UART HAL context.
 * @param set 1 to enable the UART RTS line (set low; read), 0 to disable the
 * UART RTS line (set high; write).
 */
DMX_ISR_ATTR void dmx_uart_set_rts(uart_dev_t *uart, int set) {
  uart_ll_set_rts_active_level(uart, set);
}

/**
 * @brief Resets the UART RX FIFO.
 *
 * @param hal A pointer to a UART HAL context.
 */
DMX_ISR_ATTR void dmx_uart_rxfifo_reset(uart_dev_t *uart) {
  uart_ll_rxfifo_rst(uart);
}

/**
 * @brief Gets the length of the UART TX FIFO.
 *
 * @param hal A pointer to a UART HAL context.
 * @return The length of the UART TX FIFO.
 */
DMX_ISR_ATTR uint32_t dmx_uart_get_txfifo_len(uart_dev_t *uart) {
  return uart_ll_get_txfifo_len(uart);
}

/**
 * @brief Writes to the UART TX FIFO.
 *
 * @param hal A pointer to a UART HAL context.
 * @param buf The source buffer from which to write.
 * @param size The number of bytes to write.
 */
DMX_ISR_ATTR void dmx_uart_write_txfifo(uart_dev_t *uart, const void *buf,
                                        size_t *size) {
  const size_t txfifo_len = uart_ll_get_txfifo_len(uart);
  if (*size > txfifo_len) *size = txfifo_len;
  uart_ll_write_txfifo(uart, buf, *size);
}

/**
 * @brief Resets the UART TX FIFO.
 *
 * @param hal A pointer to a UART HAL context.
 */
DMX_ISR_ATTR void dmx_uart_txfifo_reset(uart_dev_t *uart) {
  uart_ll_txfifo_rst(uart);
}

#ifdef __cplusplus
}
#endif
