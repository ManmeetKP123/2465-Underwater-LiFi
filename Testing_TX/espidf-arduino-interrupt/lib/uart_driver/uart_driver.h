#include "driver/uart.h"

/*!
 * @brief Sends bytes over UART.
 * 
 * @param[in]   txBuffer    The buffer containing the data to send.
 * @param[in]   numBytes    The number of bytes to send.
 * 
 * @retval      Returns 1 if success; 0 otherwise.
 */
static uint8_t sendBytes(char *txBuffer, size_t numBytes);

/*!
 * @brief Receives bytes over UART. Blocking.
 * 
 * @param[in,out]   rxBuffer            The buffer to store the received data in.
 * @param[in]       rxBufLen            The length of the rx buffer.
 * @param[out]      numBytesReceived    The number of bytes received.
 * 
 * @retval      Returns 1 if success; 0 otherwise.
 */
static uint8_t receiveBytes(char *rxBuffer, size_t rxBufLen, size_t &numBytesReceived);


