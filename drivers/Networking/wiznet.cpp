/**
 * Copyright (c) 2022 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "wiznet.h"

wiznet_handle* wiznet_handle::active_instance = nullptr;
volatile bool wiznet_handle::spin_lock = false;

wiznet_handle::wiznet_handle() {
    active_instance = this;  // set the active instance for checking that non-static members can be accessed by static members.
}

wiznet_handle::~wiznet_handle() {

}

inline void wiznet_handle::wizchip_select(void)
{
    if (active_instance && active_instance->wiz_cs_pin)
        active_instance->wiz_cs_pin->set(GPIO_PIN_RESET);
}

inline void wiznet_handle::wizchip_deselect(void)
{
    if (active_instance && active_instance->wiz_cs_pin)
        active_instance->wiz_cs_pin->set(GPIO_PIN_SET);
}

void wiznet_handle::wizchip_reset()
{
    // not sure if needed, doesnt seem to effect using spi1 pa5 sck, unsure what to do. -cakeslob
    wiz_cs_pin->set(GPIO_PIN_RESET);
    wiz_rst_pin->set(GPIO_PIN_RESET);
    delay_ms(250);
    wiz_cs_pin->set(GPIO_PIN_SET);
    wiz_rst_pin->set(GPIO_PIN_SET);
    delay_ms(250);
}


void wiznet_handle::wizchip_critical_section_lock(void)
{
    while(spin_lock);

    spin_lock = true;
}

void wiznet_handle::wizchip_critical_section_unlock(void)
{
    spin_lock = false;
}

void wiznet_handle::wizchip_spi_initialize(void)
{   
    wiz_cs_pin = std::make_unique<Pin>(WIZ_CS_PORT_AND_PIN, OUTPUT, NONE); // TODO - set pointers back to the pins initliased at HAL layer
    wiz_rst_pin = std::make_unique<Pin>(WIZ_RST_PORT_AND_PIN, OUTPUT, NONE);

    // reset the device pre-emptively
    wizchip_reset();

    // initialise the SPI bus
    wizchip_select();
    spi_init(); // TODO - set pointers back to the SPI bus initliased at HAL layer
    wizchip_deselect();
    spi_set_speed(WIZCHIP_SPI_PRESCALER);
}

void wiznet_handle::wizchip_cris_initialize(void)
{
    spin_lock = false;
    reg_wizchip_cris_cbfunc(wizchip_critical_section_lock, wizchip_critical_section_unlock);
}

void wiznet_handle::wizchip_initialize(void)
{
    wizchip_reset();

    reg_wizchip_cris_cbfunc(wizchip_critical_section_lock, wizchip_critical_section_unlock);
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
    reg_wizchip_spi_cbfunc(spi_get_byte, spi_put_byte); // TODO - ensure our SPI bus can be statically accessed..
    reg_wizchip_spiburst_cbfunc(spi_read, spi_write);       // and that SPI read/write is done via DMA

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

void wiznet_handle::wizchip_check(void)
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

/* Network */
void wiznet_handle::network_initialize(wiz_NetInfo net_info)
{
    ctlnetwork(CN_SET_NETINFO, (void *)&net_info);
}

void wiznet_handle::print_network_information(wiz_NetInfo net_info)
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
