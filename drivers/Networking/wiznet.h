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
#include "wizchip_conf.h"

#ifndef WIZCHIP_SPI_PRESCALER
#define WIZCHIP_SPI_PRESCALER SPI_BAUDRATEPRESCALER_2
#endif


class wiznet_handle {

    private:
        static wiznet_handle* active_instance;  // there are a few functions in here that need to be static so they can be passed as callback functions. Problem is they rely on resources allocated at run time, this is how we check if these resources are ready.

        static void (*irq_callback)(void);
        static volatile bool spin_lock;

        std::unique_ptr<Pin> wiz_cs_pin;    // likely refactor to use HAL pointer to EthComms knowledge of NSS and a reset pin.
        std::unique_ptr<Pin> wiz_rst_pin;

    public: 
        wiznet_handle();
        ~wiznet_handle();
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
};

#endif