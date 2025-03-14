#include <string.h>
#include "uart_driver.h"

#define UART_NUM_DEFAULT UART_NUM_0

void initUart() {
    const uart_port_t uart_num = UART_NUM_DEFAULT;
    uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
    .rx_flow_ctrl_thresh = 122,
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    // Set UART pins(TX: IO4, RX: IO5, RTS: IO18, CTS: IO19)
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_DEFAULT, 4, 5, 18, 19));

    // Setup UART buffered IO with event queue
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_DEFAULT, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
}

/*!
 * @brief Sends bytes over UART.
 * 
 * @param[in]   txBuffer    The buffer containing the data to send.
 * @param[in]   numBytes    The number of bytes to send.
 * 
 * @retval      Returns 1 if success; 0 otherwise.
 */
uint8_t sendBytes(char *txBuffer, size_t numBytes) {
    // Write data to UART.
    uart_write_bytes(UART_NUM_DEFAULT, (const char*)txBuffer, numBytes);
    return 1;
}

/*!
 * @brief Receives bytes over UART. Blocking.
 * 
 * @param[in,out]   rxBuffer            The buffer to store the received data in.
 * @param[in]       rxBufLen            The length of the rx buffer.
 * @param[out]      numBytesReceived    The number of bytes received.
 * 
 * @retval          Returns 1 if success; 0 otherwise.
 */
uint8_t receiveBytes(char *rxBuffer, size_t rxBufLen, size_t &numBytesReceived) {
    printf("receiveBytes().");
    return 1;
}