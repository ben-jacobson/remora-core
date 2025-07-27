#include "W5500_Networking.h"

/**
 * Copyright (c) 2022 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

namespace network 
{
    std::shared_ptr<CommsInterface> ptr_eth_comms;

    auto ptrRxData = network::ptr_eth_comms->ptrRxData;
    auto ptrTxData = network::ptr_eth_comms->ptrTxData;

    Pin *ptr_csPin = nullptr;
    Pin *ptr_rstPin = nullptr;

    volatile bool new_pru_request = false; 

    static ip_addr_t g_ip;
    static ip_addr_t g_mask;
    static ip_addr_t g_gateway;

    //void EthernetInit(volatile rxData_t* _ptrRxData, volatile txData_t* _ptrTxData, std::shared_ptr<STM32F4_EthComms> ptr_eth_comms, Pin *ptr_csPin, Pin *ptr_rstPin)
    void EthernetInit(std::shared_ptr<CommsInterface> ptr_eth_comms, Pin *ptr_csPin, Pin *ptr_rstPin)
    {
//        network::ptrRxData = _ptrRxData;
//        network::ptrTxData = _ptrTxData;

        wiznet::wizchip_cris_initialize();

        wiznet::wizchip_reset();
        wiznet::wizchip_initialize();
        wiznet::wizchip_check();

        // Set ethernet chip MAC address
        setSHAR(lwip::mac);
        ctlwizchip(CW_RESET_PHY, 0);

        // Initialize LWIP in NO_SYS mode
        lwip_init();

        netif_add(
                &lwip::g_netif, 
                &network::g_ip, 
                &network::g_mask, 
                &network::g_gateway, 
                NULL, 
                lwip::netif_initialize, 
                netif_input);

        lwip::g_netif.name[0] = 'e';
        lwip::g_netif.name[1] = '0';

        // Assign callbacks for link and status
        netif_set_link_callback(&lwip::g_netif, lwip::netif_link_callback);
        netif_set_status_callback(&lwip::g_netif, lwip::netif_status_callback);

        // MACRAW socket open
        lwip::retval = socket(SOCKET_MACRAW, Sn_MR_MACRAW, PORT_LWIPERF, 0x00);

        if (lwip::retval < 0)
        {
            printf(" MACRAW socket open failed\n");
        }

        // Set the default interface and bring it up
        netif_set_link_up(&lwip::g_netif);
        netif_set_up(&lwip::g_netif);
    }


    void EthernetTasks()
    {
        getsockopt(SOCKET_MACRAW, SO_RECVBUF, &lwip::pack_len);

        if (lwip::pack_len > 0)
        {
            lwip::pack_len = lwip::recv_lwip(SOCKET_MACRAW, (uint8_t *)lwip::pack, lwip::pack_len);

            if (lwip::pack_len)
            {
                lwip::p = pbuf_alloc(PBUF_RAW, lwip::pack_len, PBUF_POOL);
                pbuf_take(lwip::p, lwip::pack, lwip::pack_len);
                free(lwip::pack);

                lwip::pack = static_cast<uint8_t *>(malloc(ETHERNET_MTU));
            }
            else
            {
                printf(" No packet received\n");
            }

            if (lwip::pack_len && lwip::p != NULL)
            {
                LINK_STATS_INC(link.recv);

                if (lwip::g_netif.input(lwip::p, &lwip::g_netif) != ERR_OK)
                {
                    pbuf_free(lwip::p);
                }
            }
        }
    }

    void udpServerInit(void)
    {
    struct udp_pcb *upcb;
    err_t err;

    // UDP control block for data
    upcb = udp_new();
    err = udp_bind(upcb, &g_ip, 27181);  // 27181 is the server UDP port

    /* 3. Set a receive callback for the upcb */
    if(err == ERR_OK)
    {
        udp_recv(upcb, udp_data_callback, NULL);
    }
    else
    {
        udp_remove(upcb);
    }
    }

    void udp_data_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
    {
        int txlen = 0;
        //int n;
        struct pbuf *txBuf;
        //uint32_t status;

        memcpy(&network::ptrRxData, p->payload, p->len);

        //received a PRU request, need to copy data and then change pointer assignments.
        if (network::ptrRxData->header == Config::pruRead || network::ptrRxData->header == Config::pruWrite) {
            if (network::ptrRxData->header == Config::pruRead)
            {                        
                network::ptrTxData->header = Config::pruData;
                txlen = Config::dataBuffSize + 4; // to check, why the extra 4 bytes? 
                //network::new_pru_request = true;    //  do we some how trigger the reg_wizchip_spiburst_cbfunc from here?
            }
            else if (network::ptrRxData->header == Config::pruWrite)
            {
                network::ptrTxData->header = Config::pruAcknowledge;
                txlen = Config::dataBuffSize + 4;   
                //network::new_pru_request = true;    // do we some how trigger the reg_wizchip_spiburst_cbfunc from here?
            }	
        }
    
        // allocate pbuf from RAM
        txBuf = pbuf_alloc(PBUF_TRANSPORT, txlen, PBUF_RAM);

        // copy the data into the buffer
        pbuf_take(txBuf, (char*)&network::ptrTxData->txBuffer, txlen);

        // Connect to the remote client
        udp_connect(upcb, addr, port);

        // Send a Reply to the Client
        udp_send(upcb, txBuf);

        // free the UDP connection, so we can accept new clients
        udp_disconnect(upcb);

        // Free the p_tx buffer
        pbuf_free(txBuf);

        // Free the p buffer
        pbuf_free(p);
    }

    void network_initialize(wiz_NetInfo net_info)
    {
        ctlnetwork(CN_SET_NETINFO, (void *)&net_info);
    }

    void SPI_DMA_read(uint8_t*, uint16_t)
    {
        //TODO - trigger virtual datareceived() (SPI data transfer via DMA)
    }

    void SPI_DMA_write(uint8_t*, uint16_t) 
    {
        // TODO - somehow trigger base class data to write (SPI data transfer via DMA)
    }    

    void print_network_information(wiz_NetInfo net_info)
    {
        uint8_t tmp_str[8] = {
            0,
        };

        ctlnetwork(CN_GET_NETINFO, (void *)&net_info);
        ctlwizchip(CW_GET_ID, (void *)tmp_str);

        if (net_info.dhcp == NETINFO_DHCP)
        {
            printf("====================================================================================================\n");
            printf(" %s network configuration : DHCP\n\n", (char *)tmp_str);
        }
        else
        {
            printf("====================================================================================================\n");
            printf(" %s network configuration : static\n\n", (char *)tmp_str);
        }

        printf(" MAC         : %02X:%02X:%02X:%02X:%02X:%02X\n", net_info.mac[0], net_info.mac[1], net_info.mac[2], net_info.mac[3], net_info.mac[4], net_info.mac[5]);
        printf(" IP          : %d.%d.%d.%d\n", net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
        printf(" Subnet Mask : %d.%d.%d.%d\n", net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3]);
        printf(" Gateway     : %d.%d.%d.%d\n", net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3]);
        printf(" DNS         : %d.%d.%d.%d\n", net_info.dns[0], net_info.dns[1], net_info.dns[2], net_info.dns[3]);
        printf("====================================================================================================\n\n");
    }
}

namespace lwip 
{
    struct netif g_netif;

    uint8_t mac[6] = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56};
    int8_t retval = 0;
    uint8_t *pack = static_cast<uint8_t *>(malloc(ETHERNET_MTU));
    uint16_t pack_len = 0;
    struct pbuf *p = NULL;    

    static uint8_t tx_frame[1542];
    static const uint32_t ethernet_polynomial_le = 0xedb88320U;

    int32_t send_lwip(uint8_t sn, uint8_t *buf, uint16_t len)
    {
        uint16_t freesize = 0;
        //uint8_t tmp = getSn_SR(sn);
        getSn_SR(sn);

        freesize = getSn_TxMAX(sn);
        if (len > freesize)
            len = freesize; // check size not to exceed MAX size.

        wiz_send_data(sn, buf, len);
        setSn_CR(sn, Sn_CR_SEND);
        while (getSn_CR(sn))
            ;

        while (1)
        {
            uint8_t IRtemp = getSn_IR(sn);
            if (IRtemp & Sn_IR_SENDOK)
            {
                setSn_IR(sn, Sn_IR_SENDOK);
                // printf("Packet sent ok\n");
                break;
            }
            else if (IRtemp & Sn_IR_TIMEOUT)
            {
                setSn_IR(sn, Sn_IR_TIMEOUT);
                // printf("Socket is closed\n");
                //  There was a timeout
                return -1;
            }
        }

        return (int32_t)len;
    }

    int32_t recv_lwip(uint8_t sn, uint8_t *buf, uint16_t len)
    {
        uint8_t head[2];
        uint16_t pack_len = 0;

        pack_len = getSn_RX_RSR(sn);

        if (pack_len > 0)
        {
            wiz_recv_data(sn, head, 2);
            setSn_CR(sn, Sn_CR_RECV);

            // byte size of data packet (2byte)
            pack_len = head[0];
            pack_len = (pack_len << 8) + head[1];
            pack_len -= 2;

            if (pack_len > len)
            {
                // Packet is bigger than buffer - drop the packet
                wiz_recv_ignore(sn, pack_len);
                setSn_CR(sn, Sn_CR_RECV);
                return 0;
            }

            wiz_recv_data(sn, buf, pack_len); // data copy
            setSn_CR(sn, Sn_CR_RECV);
        }

        return (int32_t)pack_len;
    }

    err_t netif_output(struct netif *netif, struct pbuf *p)
    {
        uint32_t tot_len = 0;

        memset(tx_frame, 0x00, sizeof(tx_frame));

        for (struct pbuf *q = p; q != NULL; q = q->next)
        {
            memcpy(tx_frame + tot_len, q->payload, q->len);

            tot_len += q->len;

            if (q->len == q->tot_len)
            {
                break;
            }
        }

        if (tot_len < 60)
        {
            // pad
            tot_len = 60;
        }

        //uint32_t crc = ethernet_frame_crc(tx_frame, tot_len); // unused? 

        //uint32_t send_len = send_lwip(0, tx_frame, tot_len);
        send_lwip(0, tx_frame, tot_len);

        return ERR_OK;
    }

    void netif_link_callback(struct netif *netif)
    {
        printf("netif link status changed %s\n", netif_is_link_up(netif) ? "up" : "down");
    }

    void netif_status_callback(struct netif *netif)
    {
        printf("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
    }

    err_t netif_initialize(struct netif *netif)
    {
        netif->linkoutput = netif_output;
        netif->output = etharp_output;
        netif->mtu = ETHERNET_MTU;
        netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
        SMEMCPY(netif->hwaddr, lwip::mac, sizeof(netif->hwaddr));
        netif->hwaddr_len = sizeof(netif->hwaddr);
        return ERR_OK;
    }

    uint32_t ethernet_frame_crc(const uint8_t *data, int length)
    {
        uint32_t crc = 0xffffffff; /* Initial value. */

        while (--length >= 0)
        {
            uint8_t current_octet = *data++;

            for (int bit = 8; --bit >= 0; current_octet >>= 1)
            {
                if ((crc ^ current_octet) & 1)
                {
                    crc >>= 1;
                    crc ^= lwip::ethernet_polynomial_le;
                }
                else
                    crc >>= 1;
            }
        }

        return ~crc;
    }
}

namespace wiznet 
{
    static volatile bool spin_lock = false;

    void wizchip_select(void)
    {
        network::ptr_csPin->set(false);
    }

    void wizchip_deselect(void)
    {
        network::ptr_csPin->set(true);

    }

    void wizchip_reset()
    {
        network::ptr_rstPin->set(false);
        delay_ms(100);
        network::ptr_rstPin->set(true);
        delay_ms(100);
    }

    // uint8_t physical_layer::wizchip_read(void)
    // {
    //     uint8_t rx_data = 0;
    //     uint8_t tx_data = 0xFF;

    //     //spi_read_blocking(SPI_PORT, tx_data, &rx_data, 1);

    //     return rx_data;
    // }

    // void physical_layer::wizchip_write(uint8_t tx_data)
    // {
    //     //spi_write_blocking(SPI_PORT, &tx_data, 1);
    // }

    //static void wizchip_read_burst(uint8_t *pBuf, uint16_t len)
    //{
    //     uint8_t dummy_data = 0xFF;

    //     channel_config_set_read_increment(&dma_channel_config_tx, false);
    //     channel_config_set_write_increment(&dma_channel_config_tx, false);
    //     dma_channel_configure(dma_tx, &dma_channel_config_tx,
    //                           &spi_get_hw(SPI_PORT)->dr, // write address
    //                           &dummy_data,               // read address
    //                           len,                       // element count (each element is of size transfer_data_size)
    //                           false);                    // don't start yet

    //     channel_config_set_read_increment(&dma_channel_config_rx, false);
    //     channel_config_set_write_increment(&dma_channel_config_rx, true);
    //     dma_channel_configure(dma_rx, &dma_channel_config_rx,
    //                           pBuf,                      // write address
    //                           &spi_get_hw(SPI_PORT)->dr, // read address
    //                           len,                       // element count (each element is of size transfer_data_size)
    //                           false);                    // don't start yet

    //     dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
    //     dma_channel_wait_for_finish_blocking(dma_rx);
    //}

    //void physical_layer::wizchip_write_burst(uint8_t *pBuf, uint16_t len)
    //{
    //     uint8_t dummy_data;

    //     channel_config_set_read_increment(&dma_channel_config_tx, true);
    //     channel_config_set_write_increment(&dma_channel_config_tx, false);
    //     dma_channel_configure(dma_tx, &dma_channel_config_tx,
    //                           &spi_get_hw(SPI_PORT)->dr, // write address
    //                           pBuf,                      // read address
    //                           len,                       // element count (each element is of size transfer_data_size)
    //                           false);                    // don't start yet

    //     channel_config_set_read_increment(&dma_channel_config_rx, false);
    //     channel_config_set_write_increment(&dma_channel_config_rx, false);
    //     dma_channel_configure(dma_rx, &dma_channel_config_rx,
    //                           &dummy_data,               // write address
    //                           &spi_get_hw(SPI_PORT)->dr, // read address
    //                           len,                       // element count (each element is of size transfer_data_size)
    //                           false);                    // don't start yet

    //     dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
    //     dma_channel_wait_for_finish_blocking(dma_rx);
    //}

    void wizchip_critical_section_lock(void)
    {
        while(wiznet::spin_lock);

        wiznet::spin_lock = true;
    }

    void wizchip_critical_section_unlock(void)
    {
        wiznet::spin_lock = false;
    }

    void wizchip_cris_initialize(void)
    {
        wiznet::spin_lock = false;
        reg_wizchip_cris_cbfunc(wizchip_critical_section_lock, wizchip_critical_section_unlock);
    }

    void wizchip_initialize(void)
    {
        /* Deselect the FLASH : chip select high */
        wizchip_deselect();

        /* Call back function register for wizchip CS pin */
        reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);

        // Set up our callback functions for the Wiznet to trigger SPI read and write from the HAL class
        // reg_wizchip_spi_cbfunc(application_layer::ptr_eth_comms->spi_get_byte, application_layer::ptr_eth_comms->spi_put_byte); // for individual SPI bytes only
        reg_wizchip_spiburst_cbfunc(network::SPI_DMA_read, network::SPI_DMA_write); // seems the burst function plays better with DMA?

        /* W5x00 initialize */
        uint8_t temp;
        #if (_WIZCHIP_ == W5100S)
            uint8_t memsize[2][4] = {{8, 0, 0, 0}, {8, 0, 0, 0}};
        #elif (_WIZCHIP_ == W5500)
            uint8_t memsize[2][8] = {{8, 0, 0, 0, 0, 0, 0, 0}, {8, 0, 0, 0, 0, 0, 0, 0}};
        #endif

        if (ctlwizchip(CW_INIT_WIZCHIP, (void *)memsize) == -1)
        {
            printf(" W5x00 initialized fail\n");

            return;
        }

        /* Check PHY link status */
        do
        {
            if (ctlwizchip(CW_GET_PHYLINK, (void *)&temp) == -1)
            {
                printf(" Unknown PHY link status\n");

                return;
            }
        } while (temp == PHY_LINK_OFF);
    }

    void wizchip_check(void)
    {
    #if (_WIZCHIP_ == W5100S)
        /* Read version register */
        if (getVER() != 0x51)
        {
            printf(" ACCESS ERR : VERSION != 0x51, read value = 0x%02x\n", getVER());

            while (1)
                ;
        }
    #elif (_WIZCHIP_ == W5500)
        /* Read version register */
        if (getVERSIONR() != 0x04)
        {
            printf(" ACCESS ERR : VERSION != 0x04, read value = 0x%02x\n", getVERSIONR());

            while (1)
                ;
        }
    #endif
    }
}