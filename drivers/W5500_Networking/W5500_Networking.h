/*
Simple namespaces for Wiznet, LWIP and TFTP functionality used in variations of Remora that rely on ethernet connectivity

This is arranged a bit like the TCP/IP model, except there's only three layers. Also there's UDP in there so no need to be too strict with the analogy! 
*/

#ifndef W5500_NETWORKING_H
#define W5500_NETWORKING_H

#include <stdio.h>
#include <string.h>
#include <memory>

/* Application layer includes and defines*/
#include "remora-hal/STM32F4_EthComms.h"
#include "remora-hal/pin/pin.h"

class STM32F4_EthComms; // forward declaration.

/* Transport layer includes and defines */
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/apps/lwiperf.h"
#include "lwip/etharp.h"
#include "tftpserver.h"

#define ETHERNET_MTU 1500

/* Prevent having to link sys_arch.c (we don't test the API layers in unit tests) */
#define NO_SYS 1
#define MEM_ALIGNMENT 4
#define LWIP_RAW 1
#define LWIP_NETCONN 0
#define LWIP_SOCKET 0
#define LWIP_DHCP 1
#define LWIP_DNS 1
#define LWIP_ICMP 1
#define LWIP_UDP 1
#define LWIP_TCP 1
#define MEM_SIZE 2048
#define SOCKET_MACRAW 0
#define PORT_LWIPERF 5001

// disable ACD to avoid build errors
// http://lwip.100.n7.nabble.com/Build-issue-if-LWIP-DHCP-is-set-to-0-td33280.html
#define LWIP_DHCP_DOES_ACD_CHECK 0

#define ETH_PAD_SIZE 0
#define LWIP_IP_ACCEPT_UDP_PORT(p) ((p) == PP_NTOHS(67))

#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_NETIF_STATUS_CALLBACK 1

#define TCP_MSS (1500 /*mtu*/ - 20 /*iphdr*/ - 20 /*tcphhr*/)
#define TCP_SND_BUF (2 * TCP_MSS)

#define LWIP_HTTPD_CGI 0
#define LWIP_HTTPD_SSI 0
#define LWIP_HTTPD_SSI_INCLUDE_TAG 0

#define LWIP_RAND_WIZ() ((u32_t)rand())

#if 1
#define LWIP_DEBUG 1
#define TCP_DEBUG LWIP_DBG_OFF
#define ETHARP_DEBUG LWIP_DBG_OFF
#define PBUF_DEBUG LWIP_DBG_OFF
#define IP_DEBUG LWIP_DBG_OFF
#define TCPIP_DEBUG LWIP_DBG_OFF
#define DHCP_DEBUG LWIP_DBG_OFF
#define UDP_DEBUG LWIP_DBG_OFF
#endif

/* Physical layer includes and definitions */
#include "socket.h"


// Application layer namespace
namespace application_layer {
    std::shared_ptr<STM32F4_EthComms> ptr_eth_comms;

    Pin *ptr_csPin;
    Pin *ptr_rstPin;
        
    extern uint8_t mac[6];
    static ip_addr_t g_ip;
    static ip_addr_t g_mask;
    static ip_addr_t g_gateway;

    void EthernetInit(std::shared_ptr<STM32F4_EthComms>, Pin*, Pin*);
    void udpServerInit();
    void EthernetTasks();
    void udp_data_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
    void network_initialize(wiz_NetInfo net_info);

    /*! \brief Print network information
    *  \ingroup w5x00_spi
    *
    *  Print network information about MAC address, IP address, Subnet mask, Gateway, DHCP and DNS address.
    *
    *  \param net_info network information.
    */
    void print_network_information(wiz_NetInfo net_info);
}

// Transport layer namespace
namespace transport_layer {
    uint8_t mac[6] = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56};

    static uint8_t tx_frame[1542];
    static const uint32_t ethernet_polynomial_le = 0xedb88320U;

    struct netif g_netif;

    int8_t retval = 0;
    uint8_t *pack = static_cast<uint8_t *>(malloc(ETHERNET_MTU));
    uint16_t pack_len = 0;
    struct pbuf *p = NULL;    

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
    err_t netif_output(struct netif *netif, struct pbuf *p);

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
}

// Physical layer namespace
namespace physical_layer {
    static volatile bool spin_lock = false;

    /**
     * ----------------------------------------------------------------------------------------------------
     * Functions
     * ----------------------------------------------------------------------------------------------------
     */
    /* W5x00 */
    /*! \brief Set CS pin
    *  \ingroup w5x00_spi
    *
    *  Set chip select pin of spi0 to low(Active low).
    *
    *  \param none
    */
    static inline void wizchip_select(void);

    /*! \brief Set CS pin
    *  \ingroup w5x00_spi
    *
    *  Set chip select pin of spi0 to high(Inactive high).
    *
    *  \param none
    */
    static inline void wizchip_deselect(void);

    /*! \brief Read from an SPI device, blocking
    *  \ingroup w5x00_spi
    *
    *  Set spi_read_blocking function.
    *  Read byte from SPI to rx_data buffer.
    *  Blocks until all data is transferred. No timeout, as SPI hardware always transfers at a known data rate.
    *
    *  \param none
    */
    static uint8_t wizchip_read(void);

    /*! \brief Write to an SPI device, blocking
    *  \ingroup w5x00_spi
    *
    *  Set spi_write_blocking function.
    *  Write byte from tx_data buffer to SPI device.
    *  Blocks until all data is transferred. No timeout, as SPI hardware always transfers at a known data rate.
    *
    *  \param tx_data Buffer of data to write
    */
    static void wizchip_write(uint8_t tx_data);

    /*! \brief Configure all DMA parameters and optionally start transfer
    *  \ingroup w5x00_spi
    *
    *  Configure all DMA parameters and read from DMA
    *
    *  \param pBuf Buffer of data to read
    *  \param len element count (each element is of size transfer_data_size)
    */
    static void wizchip_read_burst(uint8_t *pBuf, uint16_t len);

    /*! \brief Configure all DMA parameters and optionally start transfer
    *  \ingroup w5x00_spi
    *
    *  Configure all DMA parameters and write to DMA
    *
    *  \param pBuf Buffer of data to write
    *  \param len element count (each element is of size transfer_data_size)
    */
    static void wizchip_write_burst(uint8_t *pBuf, uint16_t len);

    /*! \brief Enter a critical section
    *  \ingroup w5x00_spi
    *
    *  Set ciritical section enter blocking function.
    *  If the spin lock associated with this critical section is in use, then this
    *  method will block until it is released.
    *
    *  \param none
    */
    static void wizchip_critical_section_lock(void);

    /*! \brief Release a critical section
    *  \ingroup w5x00_spi
    *
    *  Set ciritical section exit function.
    *  Release a critical section.
    *
    *  \param none
    */
    static void wizchip_critical_section_unlock(void);

    /*! \brief Initialize SPI instances and Set DMA channel
    *  \ingroup w5x00_spi
    *
    *  Set GPIO to spi0.
    *  Puts the SPI into a known state, and enable it.
    *  Set DMA channel completion channel.
    *
    *  \param none
    */
    void wizchip_cris_initialize(void);

    /*! \brief W5x00 chip reset
    *  \ingroup w5x00_spi
    *
    *  Set a reset pin and reset.
    *
    *  \param none
    */
    void wizchip_reset(void);

    /*! \brief Initialize WIZchip
    *  \ingroup w5x00_spi
    *
    *  Set callback function to read/write byte using SPI.
    *  Set callback function for WIZchip select/deselect.
    *  Set memory size of W5x00 chip and monitor PHY link status.
    *
    *  \param none
    */
    void wizchip_initialize(void);

    /*! \brief Check chip version
    *  \ingroup w5x00_spi
    *
    *  Get version information.
    *
    *  \param none
    */
    void wizchip_check(void);

    /* Network */
    /*! \brief Initialize network
    *  \ingroup w5x00_spi
    *
    *  Set network information.
    *
    *  \param net_info network information.
    */

}

#endif
