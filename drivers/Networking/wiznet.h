/**
 * Copyright (c) 2022 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WIZNET_H
#define WIZNET_H

//#include <stdio.h>
//#include <stdbool.h>
#include <memory>

#include "remora-hal/hal_utils.h"
#include "remora-hal/pin/pin.h"
#include "ioLibrary_Driver/Ethernet/wizchip_conf.h"

#ifndef WIZCHIP_SPI_PRESCALER
#define WIZCHIP_SPI_PRESCALER SPI_BAUDRATEPRESCALER_2
#endif

class STM32F4_EthComms; // Forward declaration

class wiznet_handle {

    private:
        static wiznet_handle* active_instance;  // there are a few functions in here that need to be static so they can be passed as callback functions. Problem is they rely on resources allocated at run time, this is how we check if these resources are ready.

        std::shared_ptr<STM32F4_EthComms> eth_comms; // for callbacks

        static void (*irq_callback)(void);
        static volatile bool spin_lock;

        std::unique_ptr<Pin> wiz_cs_pin;  // may need to upgrade to shared pointer in parent. 
        std::unique_ptr<Pin> wiz_rst_pin;

        static inline void wizchip_select(void);
        static inline void wizchip_deselect(void);
        static void wizchip_critical_section_lock(void);
        static void wizchip_critical_section_unlock(void);

        void wizchip_reset();        
        void wizchip_spi_initialize(void);
        void wizchip_cris_initialize(void);
        void wizchip_initialize(void);
        void wizchip_check(void);
        void network_initialize(wiz_NetInfo);
        void print_network_information(wiz_NetInfo);

    public: 
        wiznet_handle(std::shared_ptr<STM32F4_EthComms>);
        ~wiznet_handle();
};

#endif