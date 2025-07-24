#include "W5500_Networking.h"

/**
 * Copyright (c) 2022 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * ----------------------------------------------------------------------------------------------------
 * Includes
 * ----------------------------------------------------------------------------------------------------
 */
#include <stdio.h>

void application_layer::EthernetInit()
{
    physical_layer::wizchip_spi_initialize();
    physical_layer::wizchip_cris_initialize();

    physical_layer::wizchip_reset();
    physical_layer::wizchip_initialize();
    physical_layer::wizchip_check();

    // Set ethernet chip MAC address
    setSHAR(mac);
    ctlwizchip(CW_RESET_PHY, 0);

    // Initialize LWIP in NO_SYS mode
    lwip_init();

    netif_add(&g_netif, &g_ip, &g_mask, &g_gateway, NULL, netif_initialize, netif_input);
    g_netif.name[0] = 'e';
    g_netif.name[1] = '0';

    // Assign callbacks for link and status
    netif_set_link_callback(&g_netif, netif_link_callback);
    netif_set_status_callback(&g_netif, netif_status_callback);

    // MACRAW socket open
    retval = socket(SOCKET_MACRAW, Sn_MR_MACRAW, PORT_LWIPERF, 0x00);

    if (retval < 0)
    {
        printf(" MACRAW socket open failed\n");
    }

    // Set the default interface and bring it up
    netif_set_link_up(&g_netif);
    netif_set_up(&g_netif);
}


void application_layer::EthernetTasks()
{
    getsockopt(SOCKET_MACRAW, SO_RECVBUF, &pack_len);

    if (pack_len > 0)
    {
        pack_len = recv_lwip(SOCKET_MACRAW, (uint8_t *)pack, pack_len);

        if (pack_len)
        {
            p = pbuf_alloc(PBUF_RAW, pack_len, PBUF_POOL);
            pbuf_take(p, pack, pack_len);
            free(pack);

            pack = static_cast<uint8_t *>(malloc(ETHERNET_MTU));
        }
        else
        {
            printf(" No packet received\n");
        }

        if (pack_len && p != NULL)
        {
            LINK_STATS_INC(link.recv);

            if (g_netif.input(p, &g_netif) != ERR_OK)
            {
                pbuf_free(p);
            }
        }
    }
}


void application_layer::udpServerInit(void)
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


void application_layer::udp_data_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	int txlen = 0;
    int n;
	struct pbuf *txBuf;
    uint32_t status;

    //received data from host needs to go into the inactive buffer
    rxData_t* rxBuffer = getAltRxBuffer(&rxPingPongBuffer);
    //data sent to host needs to come from the active buffer
    txData_t* txBuffer = getCurrentTxBuffer(&txPingPongBuffer);

	memcpy(&rxBuffer->rxBuffer, p->payload, p->len);

    //received a PRU request, need to copy data and then change pointer assignments.
    if (rxBuffer->header == PRU_READ || rxBuffer->header == PRU_WRITE) {


        if (rxBuffer->header == PRU_READ)
        {        
            //if it is a read, need to swap the TX buffer over but the RX buffer needs to remain unchanged.
            //feedback data will now go into the alternate buffer
            while (baseThread->semaphore);
                baseThread->semaphore = true;
            //don't need to wait for the servo thread.

            swapTxBuffers(&txPingPongBuffer);

            baseThread->semaphore = false;            
            
            //txBuffer pointer is now directed at the 'old' data for transmission
            txBuffer->header = PRU_DATA;
            txlen = BUFFER_SIZE;
            comms->dataReceived();
        }
        else if (rxBuffer->header == PRU_WRITE)
        {
            //if it is a write, then both the RX and TX buffers need to be changed.
            while (baseThread->semaphore);
                baseThread->semaphore = true;
            //don't need to wait for the servo thread.
            //feedback data will now go into the alternate buffer
            swapTxBuffers(&txPingPongBuffer);
            //frequency command will now come from the new data
            swapRxBuffers(&rxPingPongBuffer);
            baseThread->semaphore = false;               
            
            //txBuffer pointer is now directed at the 'old' data for transmission
            txBuffer->header = PRU_ACKNOWLEDGE;
            txlen = BUFFER_SIZE;
            comms->dataReceived();
        }	
    }
   
	// allocate pbuf from RAM
	txBuf = pbuf_alloc(PBUF_TRANSPORT, txlen, PBUF_RAM);

	// copy the data into the buffer
	pbuf_take(txBuf, (char*)&txBuffer->txBuffer, txlen);

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

void application_layer::network_initialize(wiz_NetInfo net_info)
{
    ctlnetwork(CN_SET_NETINFO, (void *)&net_info);
}

void application_layer::print_network_information(wiz_NetInfo net_info)
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


int32_t transport_layer::send_lwip(uint8_t sn, uint8_t *buf, uint16_t len)
{
    uint8_t tmp = 0;
    uint16_t freesize = 0;

    tmp = getSn_SR(sn);

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

int32_t transport_layer::recv_lwip(uint8_t sn, uint8_t *buf, uint16_t len)
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

err_t transport_layer::netif_output(struct netif *netif, struct pbuf *p)
{
    uint32_t send_len = 0;
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

    uint32_t crc = ethernet_frame_crc(tx_frame, tot_len);

    send_len = send_lwip(0, tx_frame, tot_len);

    return ERR_OK;
}

void transport_layer::netif_link_callback(struct netif *netif)
{
    printf("netif link status changed %s\n", netif_is_link_up(netif) ? "up" : "down");
}

void transport_layer::netif_status_callback(struct netif *netif)
{
    printf("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
}

err_t transport_layer::netif_initialize(struct netif *netif)
{
    netif->linkoutput = netif_output;
    netif->output = etharp_output;
    netif->mtu = ETHERNET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
    SMEMCPY(netif->hwaddr, mac, sizeof(netif->hwaddr));
    netif->hwaddr_len = sizeof(netif->hwaddr);
    return ERR_OK;
}

transport_layer::uint32_t ethernet_frame_crc(const uint8_t *data, int length)
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
                crc ^= ethernet_polynomial_le;
            }
            else
                crc >>= 1;
        }
    }

    return ~crc;
}

inline void physical_layer::wizchip_select(void)
{
    //gpio_put(PIN_CS, 0);
}

inline void physical_layer::wizchip_deselect(void)
{
    //gpio_put(PIN_CS, 1);
}

void physical_layer::wizchip_reset()
{
    // gpio_set_dir(PIN_RST, GPIO_OUT);

    // gpio_put(PIN_RST, 0);
    // sleep_ms(100);

    // gpio_put(PIN_RST, 1);
    // sleep_ms(100);

    // bi_decl(bi_1pin_with_name(PIN_RST, "W5x00 RESET"));
}

uint8_t physical_layer::wizchip_read(void)
{
    uint8_t rx_data = 0;
    uint8_t tx_data = 0xFF;

    //spi_read_blocking(SPI_PORT, tx_data, &rx_data, 1);

    return rx_data;
}

void physical_layer::wizchip_write(uint8_t tx_data)
{
    //spi_write_blocking(SPI_PORT, &tx_data, 1);
}

static void wizchip_read_burst(uint8_t *pBuf, uint16_t len)
{
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
}

void physical_layer::wizchip_write_burst(uint8_t *pBuf, uint16_t len)
// {
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
}

void physical_layer::wizchip_critical_section_lock(void)
{
    critical_section_enter_blocking(&g_wizchip_cri_sec);
}

physical_layer::void wizchip_critical_section_unlock(void)
{
    critical_section_exit(&g_wizchip_cri_sec);
}

void physical_layer::wizchip_spi_initialize(void)
{
    // // this example will use SPI0 at 50MHz
    // spi_init(SPI_PORT, 50000 * 1000);

    // gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    // // make the SPI pins available to picotool
    // bi_decl(bi_3pins_with_func(PIN_MISO, PIN_MOSI, PIN_SCK, GPIO_FUNC_SPI));

    // // chip select is active-low, so we'll initialise it to a driven-high state
    // gpio_init(PIN_CS);
    // gpio_set_dir(PIN_CS, GPIO_OUT);
    // gpio_put(PIN_CS, 1);

    // // make the SPI pins available to picotool
    // bi_decl(bi_1pin_with_name(PIN_CS, "W5x00 CHIP SELECT"));

    // #ifdef USE_SPI_DMA
    //     dma_tx = dma_claim_unused_channel(true);
    //     dma_rx = dma_claim_unused_channel(true);

    //     dma_channel_config_tx = dma_channel_get_default_config(dma_tx);
    //     channel_config_set_transfer_data_size(&dma_channel_config_tx, DMA_SIZE_8);
    //     channel_config_set_dreq(&dma_channel_config_tx, DREQ_SPI0_TX);

    //     // We set the inbound DMA to transfer from the SPI receive FIFO to a memory buffer paced by the SPI RX FIFO DREQ
    //     // We coinfigure the read address to remain unchanged for each element, but the write
    //     // address to increment (so data is written throughout the buffer)
    //     dma_channel_config_rx = dma_channel_get_default_config(dma_rx);
    //     channel_config_set_transfer_data_size(&dma_channel_config_rx, DMA_SIZE_8);
    //     channel_config_set_dreq(&dma_channel_config_rx, DREQ_SPI0_RX);
    //     channel_config_set_read_increment(&dma_channel_config_rx, false);
    //     channel_config_set_write_increment(&dma_channel_config_rx, true);
    // #endif
}

void physical_layer::wizchip_cris_initialize(void)
{
    critical_section_init(&g_wizchip_cri_sec);
    reg_wizchip_cris_cbfunc(wizchip_critical_section_lock, wizchip_critical_section_unlock);
}

void physical_layer::wizchip_initialize(void)
{
    /* Deselect the FLASH : chip select high */
    wizchip_deselect();

    /* CS function register */
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);

    /* SPI function register */
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
    #ifdef USE_SPI_DMA
        reg_wizchip_spiburst_cbfunc(wizchip_read_burst, wizchip_write_burst);
    #endif

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

void physical_layer::wizchip_check(void)
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


