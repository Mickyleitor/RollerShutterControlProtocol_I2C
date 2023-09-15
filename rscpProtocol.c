/**
 * @file rscpProtocol.c
 * @brief Roller Shutter Control Panel Protocol (RSCP) for i2c communication
 *
 * This protocol is used to communicate between the main CPU and radio module.
 *
 * @author MickySim: https://www.mickysim.com
 * @date 2023
 * @copyright
 * Copyright (c) 2023 MickySim All rights reserved.
 */

//---[ Includes ]---------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>

#include "rscpProtocol.h"

#include "../moduleConfigs/rscpProtocolCallbacks.h"

//---[ Macros ]-----------------------------------------------------------------

//---[ Constants ]--------------------------------------------------------------

//---[ Types ]------------------------------------------------------------------

//---[ Private Variables ]------------------------------------------------------

//---[ Public Variables ]-------------------------------------------------------

//---[ Private Functions ]------------------------------------------------------

//---[ Public Functions ]-------------------------------------------------------

/**
 * @brief Blocking function to get a byte from the receive buffer.
 *
 * @param readByte Pointer to the variable to store the received byte.
 * @param timeout_ticks The timeout duration in ticks.
 * @return 0 on success, -1 on timeout.
 */
int32_t rscpGetRxByteBlocking(uint8_t *readByte, uint32_t timeout_ticks) {
    while (rscpGetRxByteCallback(readByte) < 0) {
        if (timeout_ticks-- == 0) {
            return -1;
        }
        rscpRxWaitingCallback();
    }
    return 0;
}

/**
 * @brief Receives an RSCP message.
 *
 * @param frame Pointer to the RSCP frame to be filled.
 * @param timeout_ticks The timeout duration in ticks.
 * @return RSCP error code.
 */
RSCP_ErrorType rscpGetMsg(struct RSCP_frame *frame, uint32_t timeout_ticks) {
    uint32_t bufferIndex = 0;
    uint8_t rscpRxStatus = 0;
    uint8_t readByte;
    while (bufferIndex < sizeof(frame->data)) {
        if (rscpGetRxByteBlocking(&readByte, timeout_ticks) < 0) {
            return RSCP_ERR_TIMEOUT;
        }
        switch (rscpRxStatus) {
            case 0: // Waiting for length byte
                if (readByte != RSCP_PREAMBLE_BYTE) {
                    frame->length = readByte;
                    rscpRxStatus = 1;
                }
                break;
            case 1: // Waiting for command byte
                frame->command = readByte;
                rscpRxStatus = 2;
                break;
            case 2: // Waiting for data bytes
                frame->data[bufferIndex++] = readByte;
                // Retrieve CRC by the current buffer index
                if (bufferIndex >= (uint32_t)(frame->length - sizeof(frame->crc))) {
                    rscpRxStatus = 3;
                }
                break;
            case 3: // Waiting for CRC high byte
                frame->crc = (readByte << 8);
                rscpRxStatus = 4;
                break;
            case 4: // Waiting for CRC low byte
                frame->crc |= readByte;
                return RSCP_ERR_OK;
        }
    }
    return RSCP_ERR_OVERFLOW;
}

#if RSCP_DEVICE_IS_MASTER

/**
 * @brief Sends a CPU query request and receives the reply.
 *
 * @param reply Pointer to the reply structure to be filled.
 * @param timeout_ticks The timeout duration in ticks.
 * @return RSCP error code.
 */
RSCP_ErrorType rscpRequestCPUQuery(struct RSCP_Reply_cpuquery *reply, uint32_t timeout_ticks) {
    RSCP_ErrorType err = RSCP_ERR_OK;
    uint32_t txBufferIndex = 0;
    uint8_t txBuffer[RSCP_MAX_TX_BUFFER_SIZE];

    // Fill txBuffer
    txBuffer[txBufferIndex++] = RSCP_PREAMBLE_BYTE;
    txBuffer[txBufferIndex++] = 3;                  // Length + cmd + data (unused data)
    txBuffer[txBufferIndex++] = RSCP_CMD_CPU_QUERY;
    txBuffer[txBufferIndex++] = 0x00;               // No data

    uint16_t crc = rscpGetCrcCallback(&txBuffer[1], txBufferIndex - 1);
    txBuffer[txBufferIndex++] = (crc >> 8) & 0xFF;
    txBuffer[txBufferIndex++] = (crc & 0xFF);

    if (rscpSendSlotCallback(txBuffer, txBufferIndex) < 0) {
        return RSCP_ERR_TX_FAILED;
    }

    struct RSCP_frame frame;
    uint32_t rxBufferMaxLength = 1 + sizeof(frame.length) + sizeof(frame.command) + sizeof(struct RSCP_Reply_cpuquery) + sizeof(frame.crc);

    if (rscpRequestSlotCallback(rxBufferMaxLength) < 0) {
        return RSCP_ERR_REQUEST_FAILED;
    }

    if ((err = rscpGetMsg(&frame, timeout_ticks)) != RSCP_ERR_OK) {
        return err;
    }

    if (rscpGetCrcCallback(((uint8_t *)&frame), frame.length) != frame.crc) {
        return RSCP_ERR_MALFORMED;
    }

    if (frame.command != RSCP_CMD_CPU_QUERY) {
        return RSCP_ERR_INVALID_ANSWER;
    }

    for (uint32_t i = 0; i < sizeof(struct RSCP_Reply_cpuquery); i++) {
        ((uint8_t *)reply)[i] = frame.data[i];
    }

    return err;
}

/**
 * @brief Sends a switch relay request and receives the reply.
 *
 * @param reply Pointer to the reply structure to be filled.
 * @param timeout_ticks The timeout duration in ticks.
 * @return RSCP error code.
 */
RSCP_ErrorType rscpRequestSwitchRelay(struct RSCP_Reply_switchrelay *reply, uint32_t timeout_ticks) {
    RSCP_ErrorType err = RSCP_ERR_OK;
    uint32_t txBufferIndex = 0;
    uint8_t txBuffer[RSCP_MAX_TX_BUFFER_SIZE];

    // Fill txBuffer
    txBuffer[txBufferIndex++] = RSCP_PREAMBLE_BYTE;
    txBuffer[txBufferIndex++] = 3;
    txBuffer[txBufferIndex++] = RSCP_CMD_GET_SWITCH_RELAY;
    txBuffer[txBufferIndex++] = 0x00;               // No data

    uint16_t crc = rscpGetCrcCallback(&txBuffer[1], txBufferIndex - 1);
    txBuffer[txBufferIndex++] = (crc >> 8) & 0xFF;
    txBuffer[txBufferIndex++] = (crc & 0xFF);

    if (rscpSendSlotCallback(txBuffer, txBufferIndex) < 0) {
        return RSCP_ERR_TX_FAILED;
    }

    struct RSCP_frame frame;
    uint32_t rxBufferMaxLength = 1 + sizeof(frame.length) + sizeof(frame.command) + sizeof(struct RSCP_Reply_switchrelay) + sizeof(frame.crc);

    if (rscpRequestSlotCallback(rxBufferMaxLength) < 0) {
        return RSCP_ERR_REQUEST_FAILED;
    }

    if ((err = rscpGetMsg(&frame, timeout_ticks)) != RSCP_ERR_OK) {
        return err;
    }

    if (rscpGetCrcCallback(((uint8_t *)&frame), frame.length) != frame.crc) {
        return RSCP_ERR_MALFORMED;
    }

    if (frame.command != RSCP_CMD_GET_SWITCH_RELAY) {
        return RSCP_ERR_INVALID_ANSWER;
    }

    for (uint32_t i = 0; i < sizeof(struct RSCP_Reply_switchrelay); i++) {
        ((uint8_t *)reply)[i] = frame.data[i];
    }

    return err;
}

/**
 * @brief Sends a buzzer action request.
 *
 * @param arg Pointer to the buzzer action argument.
 * @param timeout_ticks The timeout duration in ticks.
 * @return RSCP error code.
 */
RSCP_ErrorType rscpSendBuzzerAction(struct RSCP_Arg_buzzer_action *arg, uint32_t timeout_ticks) {
    RSCP_ErrorType err = RSCP_ERR_OK;
    uint32_t txBufferIndex = 0;
    uint8_t txBuffer[RSCP_MAX_TX_BUFFER_SIZE];

    // Fill txBuffer
    txBuffer[txBufferIndex++] = RSCP_PREAMBLE_BYTE;
    txBuffer[txBufferIndex++] = 2 + sizeof(struct RSCP_Arg_buzzer_action);
    txBuffer[txBufferIndex++] = RSCP_CMD_SET_BUZZER_ACTION;

    // Use memcpy to copy struct data to txBuffer
    memcpy(&txBuffer[txBufferIndex], arg, sizeof(struct RSCP_Arg_buzzer_action));
    txBufferIndex += sizeof(struct RSCP_Arg_buzzer_action);

    uint16_t crc = rscpGetCrcCallback(&txBuffer[1], txBufferIndex - 1);
    txBuffer[txBufferIndex++] = (crc >> 8) & 0xFF;
    txBuffer[txBufferIndex++] = (crc & 0xFF);

    if (rscpSendSlotCallback(txBuffer, txBufferIndex) < 0) {
        return RSCP_ERR_TX_FAILED;
    }

    struct RSCP_frame frame;
    uint32_t rxBufferMaxLength = 1 + sizeof(frame.length) + sizeof(frame.command) + 1 + sizeof(frame.crc);

    if (rscpRequestSlotCallback(rxBufferMaxLength) < 0) {
        return RSCP_ERR_REQUEST_FAILED;
    }

    if ((err = rscpGetMsg(&frame, timeout_ticks)) != RSCP_ERR_OK) {
        return err;
    }

    if (rscpGetCrcCallback(((uint8_t *)&frame), frame.length) != frame.crc) {
        return RSCP_ERR_MALFORMED;
    }

    if (frame.command != RSCP_CMD_SET_BUZZER_ACTION) {
        return RSCP_ERR_INVALID_ANSWER;
    }

    return (RSCP_ErrorType)frame.data[0];
}

#else

/**
 * @brief Handles the CPU query request from the master.
 *
 * @return RSCP error code.
 */
RSCP_ErrorType rscpHandleCPUQuery(void) {
    RSCP_ErrorType err = RSCP_ERR_OK;
    uint32_t txBufferIndex = 0;
    uint8_t txBuffer[RSCP_MAX_TX_BUFFER_SIZE];

    // Fill reply
    struct RSCP_Reply_cpuquery reply;
    reply.flags = 0;
    reply.crcType = RSCP_DEF_CRC_TYPE_MODBUS16;
    reply.protocolversion = RSCP_DEF_PROTOCOL_VERSION;
    reply.cpuType = RSCP_DEF_CPU_TYPE_ATMEGA328P_8MHZ;
    reply.swversion = RSCP_DEF_SWVERSION_VERSION;
    reply.packetMaxLen = sizeof(struct RSCP_frame);

    // Fill txBuffer
    txBuffer[txBufferIndex++] = RSCP_PREAMBLE_BYTE;
    txBuffer[txBufferIndex++] = 2 + sizeof(struct RSCP_Reply_cpuquery);
    txBuffer[txBufferIndex++] = RSCP_CMD_CPU_QUERY;

    // Use memcpy to copy struct data to txBuffer
    memcpy(&txBuffer[txBufferIndex], &reply, sizeof(struct RSCP_Reply_cpuquery));
    txBufferIndex += sizeof(struct RSCP_Reply_cpuquery);

    uint16_t crc = rscpGetCrcCallback(&txBuffer[1], txBuffer[1]);
    txBuffer[txBufferIndex++] = (crc >> 8);
    txBuffer[txBufferIndex++] = (crc & 0xFF);

    if ((rscpSendSlotCallback(txBuffer, txBufferIndex)) < 0) {
        err = RSCP_ERR_TX_FAILED;
    }
    return err;
}

/**
 * @brief Sends a shutter position request to the master.
 *
 * @return RSCP error code.
 */
RSCP_ErrorType rscpGetShutterPosition(void) {
    RSCP_ErrorType err = RSCP_ERR_OK;
    uint32_t txBufferIndex = 0;
    uint8_t txBuffer[RSCP_MAX_TX_BUFFER_SIZE];

    struct RSCP_Reply_rollershutterposition reply;
    (void)rscpGetShutterPositionCallback(&reply);
    txBuffer[txBufferIndex++] = RSCP_PREAMBLE_BYTE;
    txBuffer[txBufferIndex++] = sizeof(struct RSCP_Reply_rollershutterposition) + 2;
    txBuffer[txBufferIndex++] = RSCP_CMD_GET_SHUTTER_POSITION;

    // Use memcpy to copy struct data to txBuffer
    memcpy(&txBuffer[txBufferIndex], &reply, sizeof(struct RSCP_Reply_rollershutterposition));
    txBufferIndex += sizeof(struct RSCP_Reply_rollershutterposition);

    uint16_t crc = rscpGetCrcCallback(&txBuffer[1], txBuffer[1]);
    txBuffer[txBufferIndex++] = (crc >> 8);
    txBuffer[txBufferIndex++] = (crc & 0xFF);

    if ((rscpSendSlotCallback(txBuffer, txBufferIndex)) < 0) {
        err = RSCP_ERR_TX_FAILED;
    }
    return err;
}

/**
 * @brief Sends a switch relay request to the master.
 *
 * @return RSCP error code.
 */
RSCP_ErrorType rscpGetSwitchRelay(void) {
    RSCP_ErrorType err = RSCP_ERR_OK;
    uint32_t txBufferIndex = 0;
    uint8_t txBuffer[RSCP_MAX_TX_BUFFER_SIZE];

    struct RSCP_Reply_switchrelay reply;
    rscpGetSwitchRelayCallback(&reply);
    txBuffer[txBufferIndex++] = RSCP_PREAMBLE_BYTE;
    txBuffer[txBufferIndex++] = sizeof(struct RSCP_Reply_switchrelay) + 2;
    txBuffer[txBufferIndex++] = RSCP_CMD_GET_SWITCH_RELAY;

    // Use memcpy to copy struct data to txBuffer
    memcpy(&txBuffer[txBufferIndex], &reply, sizeof(struct RSCP_Reply_switchrelay));
    txBufferIndex += sizeof(struct RSCP_Reply_switchrelay);

    uint16_t crc = rscpGetCrcCallback(&txBuffer[1], txBuffer[1]);
    txBuffer[txBufferIndex++] = (crc >> 8);
    txBuffer[txBufferIndex++] = (crc & 0xFF);

    if ((rscpSendSlotCallback(txBuffer, txBufferIndex)) < 0) {
        err = RSCP_ERR_TX_FAILED;
    }
    return err;
}

/**
 * @brief Sends an answer to the master.
 *
 * @param command The command byte to respond to.
 * @param errorCode The error code to send.
 * @return RSCP error code.
 */
RSCP_ErrorType rscpSendAnswer(uint8_t command, uint8_t errorCode) {
    RSCP_ErrorType err = RSCP_ERR_OK;
    uint32_t txBufferIndex = 0;
    uint8_t txBuffer[RSCP_MAX_TX_BUFFER_SIZE];

    txBuffer[txBufferIndex++] = RSCP_PREAMBLE_BYTE;
    txBuffer[txBufferIndex++] = 3;
    txBuffer[txBufferIndex++] = command;
    txBuffer[txBufferIndex++] = errorCode;

    uint16_t crc = rscpGetCrcCallback(&txBuffer[1], txBuffer[1]);
    txBuffer[txBufferIndex++] = (crc >> 8);
    txBuffer[txBufferIndex++] = (crc & 0xFF);

    if ((rscpSendSlotCallback(txBuffer, txBufferIndex)) < 0) {
        err = RSCP_ERR_TX_FAILED;
    }
    return err;
}

/**
 * @brief Handles incoming RSCP messages from the master.
 *
 * @param timeout_ticks The timeout duration in ticks.
 * @return RSCP error code.
 */
RSCP_ErrorType rscpHandle(uint32_t timeout_ticks) {
    RSCP_ErrorType err = RSCP_ERR_OK;
    struct RSCP_frame frame;

    if ((err = rscpGetMsg(&frame, timeout_ticks)) != RSCP_ERR_OK) {
        return err;
    }

    if (rscpGetCrcCallback(((uint8_t *)&frame), frame.length) != frame.crc) {
        return RSCP_ERR_MALFORMED;
    }

    switch (frame.command) {
        case RSCP_CMD_CPU_QUERY:
            return rscpHandleCPUQuery();
        case RSCP_CMD_SET_SHUTTER_ACTION:
            err = rscpSetShutterActionCallback((struct RSCP_Arg_rollershutter *)&frame.data[0]);
            break;
        case RSCP_CMD_SET_SHUTTER_POSITION:
            err = rscpSetShutterPositionCallback((struct RSCP_Arg_rollershutterposition *)&frame.data[0]);
            break;
        case RSCP_CMD_GET_SHUTTER_POSITION:
            return rscpGetShutterPosition();
        case RSCP_CMD_SET_SWITCH_RELAY:
            err = rscpSetSwitchRelayCallback((struct RSCP_Arg_switchrelay *)&frame.data[0]);
            break;
        case RSCP_CMD_GET_SWITCH_RELAY:
            return rscpGetSwitchRelay();
        case RSCP_CMD_SET_BUZZER_ACTION:
            err = rscpSetBuzzerActionCallback((struct RSCP_Arg_buzzer_action *)&frame.data[0]);
            break;
        default:
            err = RSCP_ERR_NOT_SUPPORTED;
            break;
    }

    return rscpSendAnswer(frame.command, err);
}

#endif
