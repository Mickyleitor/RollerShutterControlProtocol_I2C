# Roller Shutter Control Panel Protocol (RSCP)

The Roller Shutter Control Panel Protocol (RSCP) is a communication protocol designed for use over the I2C (Inter-Integrated Circuit) interface. It facilitates communication between the main CPU and a radio module, enabling various commands and actions related to controlling roller shutters, switches, buzzers, and other devices.

## Table of Contents

- [Introduction](#introduction)
- [Usage](#usage)
- [Protocol Commands](#protocol-commands)
- [Frame Structure](#frame-structure)
- [Error Handling](#error-handling)
- [Device Configuration](#device-configuration)
- [Contributing](#contributing)
- [License](#license)

## Introduction

RSCP is designed to enable reliable communication between a master device (main CPU) and slave devices (radio modules) in a control panel system. It defines a set of commands and data structures for controlling roller shutters, switches, buzzers, and querying device information. Key features include error handling, command execution, and device-specific actions.

## Usage

To use the RSCP protocol in your project, follow these steps:

1. Include the `rscpProtocol.h` header file in your project.

```c
#include "rscpProtocol.h"
```

2. Implement device-specific callbacks and functions based on your device's role (master or slave). These callbacks are essential for the protocol to work correctly and should be defined in the `moduleConfigs/rscpProtocolCallbacks.h` file, which is specific to your host repository.

3. Configure the library's behavior by defining the appropriate macros in the `moduleConfigs/rscpProtocolConfig.h` file. Set the `RSCP_DEVICE_IS_MASTER` macro to `1` for master devices and `0` for slave devices.

Here's a basic example of including the protocol and defining the necessary callbacks:

```c
#include "rscpProtocol.h"

// Implement device-specific callbacks and functions based on your role.
// These should be defined in moduleConfigs/rscpProtocolCallbacks.h,
// which is specific to your host repository.
```

## Protocol Commands

RSCP defines several commands that facilitate communication between devices. Each command serves a specific purpose and has a defined payload format. Some key commands include:

- `RSCP_CMD_CPU_QUERY`: Query CPU type and protocol version.
- `RSCP_CMD_SET_SHUTTER_ACTION`: Set shutter action (e.g., open, close, stop).
- `RSCP_CMD_SET_SHUTTER_POSITION`: Set shutter position.
- `RSCP_CMD_GET_SHUTTER_POSITION`: Get shutter position.
- `RSCP_CMD_SET_SWITCH_RELAY`: Set switch relay status (on or off).
- `RSCP_CMD_GET_SWITCH_RELAY`: Get switch relay status.
- `RSCP_CMD_SET_BUZZER_ACTION`: Set buzzer action (on or off).

Refer to the header file for a complete list of commands and their details.

## Frame Structure

The RSCP frame is the fundamental unit of communication in the protocol. It consists of several fields, including length, command, data, and CRC (Cyclic Redundancy Check). Understanding the frame structure at a bit level is crucial for implementing the protocol correctly.

### Frame Structure Diagram

The RSCP frame structure is as follows:

```
   +----------------+--------------+----------------+----------------+-------------+
   |   Preamble (8) |   Length (8) |   Command (8)  |  Data (0-208)  |   CRC (16)  |
   +----------------+--------------+----------------+----------------+-------------+
```

- **Preamble (8 bits)**: The preamble byte is a synchronization byte that helps devices identify the start of a new frame. It is always set to `0xAA` (10101010 in binary).

- **Length (8 bits)**: This field represents the length of the frame in bytes, excluding the CRC field. The maximum frame length is 208 bytes.

- **Command (8 bits)**: The command byte specifies the type of operation or command being performed. It determines how the data field should be interpreted.

- **Data (0-208 bits)**: The data field can vary in length depending on the specific command. It carries additional information or parameters related to the command. The maximum data length is 208 bytes.

- **CRC (16 bits)**: The CRC field contains a 16-bit Cyclic Redundancy Check value. It is used for error checking and ensures the integrity of the frame during transmission.

### Frame Structure Details

- The Preamble byte is a fixed value (`0xAA`) that helps devices synchronize their communication, serving as a marker for the beginning of a new frame.

- The Length field specifies the total length of the frame, including the Command, Data, and CRC fields. It is essential for parsing the frame correctly.

- The Command byte determines the purpose of the frame and how the data should be interpreted, with each command having a unique identifier.

- The Data field carries additional information related to the command, with its length varying depending on the command being executed.

- The CRC field contains a 16-bit Cyclic Redundancy Check value, calculated based on the entire frame (excluding the preamble). It is used to detect and correct transmission errors.

Please refer to the protocol specifications and the header file for specific details on each command's data format.

## Error Handling

RSCP defines error codes to handle different types of errors that can occur during communication. Error codes include:

- `RSCP_ERR_OK`: No error.
- `RSCP_ERR_TIMEOUT`: Timeout occurred.
- `RSCP_ERR_OVERFLOW`: Buffer overflow.
- `RSCP_ERR_MALFORMED`: Malformed message received.
- `RSCP_ERR_NOT_SUPPORTED`: Command not supported.
- `RSCP_ERR_TX_FAILED`: Message transmission failed.
- `RSCP_ERR_REQUEST_FAILED`: Request failed.
- `RSCP_ERR_TASK_BUFFER_FULL`: Task buffer full.
- `RSCP_ERR_INVALID_ANSWER`: Invalid answer received.

These error codes assist in diagnosing and handling communication issues.

## Device Configuration

Ensure that you configure your device properly based on whether it is a master or slave device. Set the `RSCP_DEVICE_IS_MASTER` macro to `1` for master devices and `0` for slave devices in the `moduleConfigs/rscpProtocolConfig.h` file.

## Contributing

Contributions to the RSCP protocol are welcome. If you encounter issues, have suggestions, or would like to contribute improvements, please feel free to open issues or submit pull requests on the project's GitHub repository.

## License

This RSCP protocol is released under the CERN Open Hardware Licence. See the [LICENSE](http://www.ohwr.org/attachments/2388/cern_ohl_v_1_2.txt) file for more information.