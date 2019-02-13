//#include "esp_eth.h"
#include "hal_eth.h"
#include "soc/io_mux_reg.h"
#include "eth_phy/phy_lan8720.h"
#include "tcpip_adapter.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "soc/emac_ex_reg.h"

#define PIN_SMI_MDC GPIO_NUM_23
#define PIN_SMI_MDIO GPIO_NUM_18

extern void rtc_plla_ena(bool ena, uint32_t sdm0, uint32_t sdm1, uint32_t sdm2, uint32_t o_div);

static void eth_gpio_config_rmii()
{
	// RMII data pins are fixed:
    // TXD0 = GPIO19
    // TXD1 = GPIO22
    // TX_EN = GPIO21
    // RXD0 = GPIO25
    // RXD1 = GPIO26
    // CLK == GPIO0
    //phy_rmii_configure_data_interface_pins();
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO27_U, FUNC_GPIO27_EMAC_RX_DV);

    // TXD0 to GPIO19
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO19_U, FUNC_GPIO19_EMAC_TXD0);
    // TX_EN to GPIO21
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO21_U, FUNC_GPIO21_EMAC_TX_EN);
    // TXD1 to GPIO22
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO22_U, FUNC_GPIO22_EMAC_TXD1);
    //clk out to GPIO17
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO16_U, FUNC_GPIO16_EMAC_CLK_OUT);  
    // for rev0 chip: f_out = f_xtal * (sdm2 + 4) / (2 * (o_div + 2))
    // so for 40MHz XTAL, sdm2 = 1 and o_div = 1 will give 50MHz //output

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO16_U, FUNC_GPIO16_EMAC_CLK_OUT);
    // REG_SET_FIELD(EMAC_EX_CLKOUT_CONF_REG, EMAC_EX_CLK_OUT_H_DIV_NUM, 0);
    // REG_SET_FIELD(EMAC_EX_CLKOUT_CONF_REG, EMAC_EX_CLK_OUT_DIV_NUM, 0);
    // REG_CLR_BIT(EMAC_EX_CLK_CTRL_REG, EMAC_EX_EXT_OSC_EN);
    // REG_SET_BIT(EMAC_EX_CLK_CTRL_REG, EMAC_EX_INT_OSC_EN);
    // rtc_plla_ena(1, 0, 0, 1, 0);
    printf("ok!!!!!");
    // RXD0 to GPIO25
    gpio_set_direction(25, GPIO_MODE_INPUT);
    // RXD1 to GPIO26
    gpio_set_direction(26, GPIO_MODE_INPUT);
    phy_rmii_smi_configure_pins(PIN_SMI_MDC, PIN_SMI_MDIO);
}
// static void tcpip_adapter_init(void)
// {
//     static bool tcpip_inited = false;
//     int ret;

//     if (tcpip_inited == false) {
//         tcpip_inited = true;

//         tcpip_init(NULL, NULL);

//         IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].ip, 192, 168 , 31, 100);
//         IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].gw, 192, 168 , 31, 1);
//         IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].netmask, 255, 255 , 255, 0);
//         ret = sys_sem_new(&api_sync_sem, 0);
//         if (ERR_OK != ret) {
//             ESP_LOGD( "tcpip adatper api sync sem init fail");
//         }
//     }
// }
esp_err_t hal_eht_init()
{
	esp_err_t ret = ESP_OK;
    tcpip_adapter_init();
    
    eth_config_t config = phy_lan8720_default_ethernet_config;
    //config.mac_mode=ETH_MODE_RMII_INT_50MHZ_CLK;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;

    ret = esp_eth_init(&config);
    return ret;
}
