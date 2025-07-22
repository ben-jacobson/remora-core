/**
 * Copyright (c) 2022 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef LWIP_H
#define LWIP_H

#include <stdio.h>
#include <string.h>

// Ensure these are included in platformio.ini
// lib_extra_dirs = 
//  	Src/remora-core/drivers/Networking	; additional libraries have been added here for portability


#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/apps/lwiperf.h"
#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "ioLibrary_Driver/Ethernet/socket.h"

#define ETHERNET_MTU 1500

class lwip_handle {
    private:
        static lwip_handle* active_instance;  // there are a few functions in here that need to be static so they can be passed as callback functions. Problem is they rely on resources allocated at run time, this is how we check if these resources are ready.

        static uint8_t tx_frame[1542];
        static const uint8_t mac[6];  

        static const uint32_t ethernet_polynomial_le = 0xedb88320U;

    public:
        lwip_handle();
        ~lwip_handle();
        /**
         * ----------------------------------------------------------------------------------------------------
         * Functions
         * ----------------------------------------------------------------------------------------------------
         */
        /*! \brief send an ethernet packet
        *  \ingroup w5x00_lwip
        *
        *  It is used to send outgoing data to the socket.
        *
        *  \param sn socket number
        *  \param buf a pointer to the data to send
        *  \param len the length of data in packet
        *  \return he sent data size
        */
        int32_t send_lwip(uint8_t sn, uint8_t *buf, uint16_t len);

        /*! \brief read an ethernet packet
        *  \ingroup w5x00_lwip
        *
        *  It is used to read incoming data from the socket.
        *
        *  \param sn socket number
        *  \param buf a pointer buffer to read incoming data
        *  \param len the length of the data in the packet
        *  \return the real received data size
        */
        int32_t recv_lwip(uint8_t sn, uint8_t *buf, uint16_t len);

        /*! \brief callback function
        *  \ingroup w5x00_lwip
        *
        *  This function is called by ethernet_output() when it wants
        *  to send a packet on the interface. This function outputs
        *  the pbuf as-is on the link medium.
        *
        *  \param netif a pre-allocated netif structure
        *  \param p main packet buffer struct
        *  \return ERR_OK if data was sent.
        */
        static err_t netif_output(struct netif *netif, struct pbuf *p);

        /*! \brief callback function
        *  \ingroup w5x00_lwip
        *
        *  Callback function for link.
        *
        *  \param netif a pre-allocated netif structure
        */
        void netif_link_callback(struct netif *netif);

        /*! \brief callback function
        *  \ingroup w5x00_lwip
        *
        *   Callback function for status.
        *
        *   \param netif a pre-allocated netif structure
        */
        void netif_status_callback(struct netif *netif);

        /*! \brief callback function
        *  \ingroup w5x00_lwip
        *
        *  Callback function that initializes the interface.
        *
        *  \param netif a pre-allocated netif structure
        *  \return ERR_OK if Network interface initialized
        */
        err_t netif_initialize(struct netif *netif);

        /*! \brief ethernet frame cyclic redundancy check
        *  \ingroup w5x00_lwip
        *
        *  Perform cyclic redundancy check on ethernet frame
        *
        *  \param data a pointer to the ethernet frame
        *  \param length the total length of ethernet frame
        *  \return an ethernet frame cyclic redundancy check result value
        */
        static uint32_t ethernet_frame_crc(const uint8_t *data, int length);    
};

#endif