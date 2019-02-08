#include <stdint.h>
#include "cmds.h"

const uint8_t cmd_to_length[] =
{
    /* CMD_UNUSED              */ 0,
    /* CMD_TEST                */ 3,
    /* CMD_TEST32              */ 32,
    /* CMD_BEACON_PAYLOAD      */ 4,
    /* CMD_CFG                 */ sizeof(cfg_t),
    /* CMD_IRQ                 */ sizeof(irq_t),
    /* CMD_SKIP_BEACONS        */ 1,
    /* CMD_CURRENT_SLOT        */ 2,

    /* CMD_FSK_MODEM_CFG_WRITE   */ 10,
    /* CMD_FSK_MODEM_CFG_REQ     */ 1,
    /* CMD_FSK_PKT_CFG_WRITE     */ 4,
    /* CMD_FSK_PKT_CFG_REQ       */ 1,
    /* CMD_FSK_SYNC_WRITE        */ 6,
    /* CMD_FSK_SYNC_REQ          */ 1,
    /* CMD_LORA_SYMBTO_WRITE     */ 1,
    /* CMD_REQ_RANDOM            */ 4,
    /* CMD_CFHZ_WRITE            */ 4,
    /* CMD_CFHZ_REQ              */ 1,
    /* CMD_TXDBM_WRITE           */ 1,
    /* CMD_TXDBM_REQ             */ 1,
    /* CMD_LORA_MODEM_CFG_WRITE  */ 4,
    /* CMD_LORA_MODEM_CFG_REQ    */ 1,
    /* CMD_RX_START              */ 4,
    /* CMD_LORA_PKT_CFG_WRITE    */ 3,
    /* CMD_LORA_PKT_CFG_REQ      */ 1,
    /* CMD_BEACON_CFG_WRITE      */ 6,
    /* CMD_STANDBY               */ 1,
    /* CMD_OPMODE_REQ            */ 1,
    /* CMD_PUBLIC_NET            */ 1,
    /* CMD_MAX_PAYLEN_WRITE      */ 1,
    /* CMD_RX_PAYLOAD            */ 32,
    /* CMD_TX_BUF_START          */ 32,
    /* CMD_TX_BUF                */ 32,
    /* CMD_SEND                  */ 0,
    /* CMD_RSSI_REQ              */ 1,
    /* CMD_RADIO_RESET           */ 1
};

