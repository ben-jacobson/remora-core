#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <stdio.h>
#include "../remora-hal/hal_configuration.h" // See note below for how to build this file

namespace Config {
    constexpr uint32_t pruBaseFreq = 40000;        // PRU Base thread ISR update frequency (hz)
    constexpr uint32_t pruServoFreq = 1000;        // PRU Servo thread ISR update frequency (hz)
    constexpr uint32_t oversample = 3;
    constexpr uint32_t swBaudRate = 19200;         // Software serial baud rate
    constexpr uint32_t pruSerialFreq = swBaudRate * oversample;

    constexpr uint32_t stepBit = 22;               // Bit location in DDS accum
    constexpr uint32_t stepMask = (1L << stepBit);

    constexpr uint32_t joints = 8;                 // Number of joints
    constexpr uint32_t variables = 6;              // Number of command values

    #ifdef ETH_CTRL
    constexpr uint32_t pruData = 0x64617461;       // "dat_" SPI payload for remora-eth-3.0 component
    #else
    constexpr uint32_t pruData = 0x64617400;       // "dat_" SPI payload for latest remora-core SPI component
    #endif
    constexpr uint32_t pruRead = 0x72656164;       // "read" SPI payload
    constexpr uint32_t pruWrite = 0x77726974;      // "writ" SPI payload
    constexpr uint32_t pruEstop = 0x65737470;      // "estp" SPI payload
    constexpr uint32_t pruAcknowledge = 0x61636b6e;// "ackn" SPI payload
    constexpr uint32_t pruErr = 0x6572726f;        // "erro" payload

    // IRQ priorities
    constexpr uint32_t baseThreadIrqPriority = 1;
    constexpr uint32_t servoThreadIrqPriority = 2;
    constexpr uint32_t serialThreadIrqPriority = 3;
    constexpr uint32_t spiDmaTxIrqPriority = 4;
    constexpr uint32_t spiDmaRxIrqPriority = 5;
    constexpr uint32_t spiNssIrqPriority = 6;

    // Serial configuration
    constexpr uint32_t pcBaud = 115200;            // UART baudrate

    constexpr uint32_t dataErrMax = 100;

    // SPI configuration
    constexpr uint32_t dataBuffSize = 64;          // Size of SPI receive buffer

    
    #ifdef ETH_CTRL
    // Network configuration for Ethernet version 
    constexpr uint8_t ip_address[4] = {10, 10, 10, 10};
    constexpr uint8_t subnet_mask[4] = {255, 255, 255, 0};
    constexpr uint8_t gateway[4] = {10, 10, 10, 1};    
    #endif

    // Default blinky program for non spi config
    constexpr char defaultConfig[] = {
        0x7B, 0x0A, 0x09, 0x22, 0x42, 0x6F, 0x61, 0x72, 0x64, 0x22, 0x3A, 0x20,
        0x22, 0x52, 0x65, 0x6D, 0x6F, 0x72, 0x61, 0x22, 0x2C, 0x0A, 0x09, 0x22,
        0x4D, 0x6F, 0x64, 0x75, 0x6C, 0x65, 0x73, 0x22, 0x3A, 0x5B, 0x0A, 0x09,
        0x7B, 0x0A, 0x09, 0x22, 0x54, 0x68, 0x72, 0x65, 0x61, 0x64, 0x22, 0x3A,
        0x20, 0x22, 0x53, 0x65, 0x72, 0x76, 0x6F, 0x22, 0x2C, 0x0A, 0x09, 0x22,
        0x54, 0x79, 0x70, 0x65, 0x22, 0x3A, 0x20, 0x22, 0x42, 0x6C, 0x69, 0x6E,
        0x6B, 0x22, 0x2C, 0x0A, 0x09, 0x09, 0x22, 0x43, 0x6F, 0x6D, 0x6D, 0x65,
        0x6E, 0x74, 0x22, 0x3A, 0x09, 0x09, 0x09, 0x22, 0x42, 0x6C, 0x69, 0x6E,
        0x6B, 0x79, 0x22, 0x2C, 0x0A, 0x09, 0x09, 0x22, 0x50, 0x69, 0x6E, 0x22,
        0x3A, 0x09, 0x09, 0x09, 0x09, 0x22, 0x50, 0x42, 0x5F, 0x30, 0x22, 0x2C,
        0x0A, 0x09, 0x09, 0x22, 0x46, 0x72, 0x65, 0x71, 0x75, 0x65, 0x6E, 0x63,
        0x79, 0x22, 0x3A, 0x20, 0x09, 0x09, 0x34, 0x0A, 0x09, 0x7D, 0x0A, 0x09,
        0x5D, 0x0A, 0x7D
    };

    // Default file config: 

    // {
    //     "Board": "Remora",
    //     "Modules":[
    //     {
    //     "Thread": "Servo",
    //     "Type": "Blink",
    //         "Comment":			"Blinky",
    //         "Pin":				"PB_0",
    //         "Frequency": 		4
    //     }
    //     ]
    // }    
}

/* 
Explanation of HAL_configuration file:

Some of the hardware specific implementation details cannot be set at the abstract Remora-Core level.
They need to be implemented depending on your specific Hardware requirements. 
For example the memory locations of where the JSON file is stored in FLASH for ethernet builds 

Example ../remora-hal/hal_configuration.h file
-------------------------------

#ifndef HAL_CONFIGURATION_H
#define HAL_CONFIGURATION_H

#include "stm32f4xx.h"      // Example of STM build, replace with your vendors HAL header file

#include <cstdint>

namespace HAL_Config {
    constexpr std::uintptr_t JSON_upload_address            = 0x8040000;        
    constexpr std::uintptr_t JSON_storage_address           = 0x8060000;
    constexpr std::uintptr_t user_flash_last_page_address   = JSON_storage_address;
    constexpr std::uintptr_t user_flash_end_address         = JSON_storage_address + (128 * 1024) - 1;
    constexpr uint32_t JSON_SECTOR                          = FLASH_SECTOR_7;
}
#endif


*/

#endif
