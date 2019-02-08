#define CMD_UNUSED              0x00
#define CMD_TEST                0x01
#define CMD_TEST32              0x02
#define CMD_BEACON_PAYLOAD      0x03
#define CMD_CFG                 0x04    // firmware config
#define CMD_IRQ                 0x05    // read when irq pin is asserted
#define CMD_SKIP_BEACONS        0x06
#define CMD_CURRENT_SLOT        0x07

#define CMD_RADIO                 0x08
#define CMD_FSK_MODEM_CFG_WRITE   (CMD_RADIO+0x00)   // 08
#define CMD_FSK_MODEM_CFG_REQ     (CMD_RADIO+0x01)   // 09
#define CMD_FSK_PKT_CFG_WRITE     (CMD_RADIO+0x02)   // 0a 
#define CMD_FSK_PKT_CFG_REQ       (CMD_RADIO+0x03)   // 0b
#define CMD_FSK_SYNC_WRITE        (CMD_RADIO+0x04)   // 0c
#define CMD_FSK_SYNC_REQ          (CMD_RADIO+0x05)   // 0d
#define CMD_LORA_SYMBTO_WRITE     (CMD_RADIO+0x06)   // 0e
#define CMD_REQ_RANDOM            (CMD_RADIO+0x07)   // 0f
#define CMD_CFHZ_WRITE            (CMD_RADIO+0x08)   // 10
#define CMD_CFHZ_REQ              (CMD_RADIO+0x09)   // 11
#define CMD_TXDBM_WRITE           (CMD_RADIO+0x0a)   // 12
#define CMD_TXDBM_REQ             (CMD_RADIO+0x0b)   // 13
#define CMD_LORA_MODEM_CFG_WRITE  (CMD_RADIO+0x0c)   // 14
#define CMD_LORA_MODEM_CFG_REQ    (CMD_RADIO+0x0d)   // 15
#define CMD_RX_START              (CMD_RADIO+0x0e)   // 16
#define CMD_LORA_PKT_CFG_WRITE    (CMD_RADIO+0x0f)   // 17
#define CMD_LORA_PKT_CFG_REQ      (CMD_RADIO+0x10)   // 18
#define CMD_BEACON_CFG_WRITE      (CMD_RADIO+0x11)   // 19
#define CMD_STANDBY               (CMD_RADIO+0x12)   // 1a
#define CMD_OPMODE_REQ            (CMD_RADIO+0x13)   // 1b
#define CMD_PUBLIC_NET            (CMD_RADIO+0x14)   // 1c
#define CMD_MAX_PAYLEN_WRITE      (CMD_RADIO+0x15)   // 1d
#define CMD_RX_PAYLOAD            (CMD_RADIO+0x16)   // 1e read this when irq.fields.flags.rx_pkt == 1
#define CMD_TX_BUF_START          (CMD_RADIO+0x17)   // 1f length and begin of payload
#define CMD_TX_BUF                (CMD_RADIO+0x18)   // 20 continuing payload
#define CMD_SEND                  (CMD_RADIO+0x19)   // 21 transmit packet, immediate
#define CMD_RSSI_REQ              (CMD_RADIO+0x1a)   // 22 returned via IRQ
#define CMD_RADIO_RESET           (CMD_RADIO+0x1b)   // 23


#ifdef DEBUG_SMBUS
#define CMD_ISR                 0xf8
#define CMD_STOPF               0xf9
#define CMD_ADDR_HOST_READ      0xfa
#define CMD_ADDR_HOST_WRITE     0xfb
#define CMD_AF                  0xfc    // debug for tx buf on host read
#endif /* DEBUG_SMBUS */
#define CMD_TIMEOUT             0xfd    // indication of smbus timeout
#define CMD_ARLO                0xfe    // indication of arbitration lost
#define CMD_BUSERR              0xff    // indication of start or stop during transfer

typedef union {
    struct {
        uint8_t fixLen : 1;    // 0
        uint8_t crcOn  : 1;    // 1
        uint8_t invIQ  : 1;    // 2
    } bits;
    uint8_t octet;
} pktcfg_t;

typedef union {
    struct __attribute__((packed)) {
        struct {
            uint8_t rxOnTxDone : 1;    // 0      enables gateway operation
            uint8_t res        : 7;    // 1,2,3,4,5,6,7
        } flags;
        uint32_t maxListenTime; // LBT
        uint32_t channelFreeTime;   // LBT
        int8_t rssiThresh;  // LBT
        uint8_t n_rssi_samples;     // rssi averaging
        uint16_t downlinkOffset;    // uplink end to downlink in ms, if rxOnTxDone==1
    } fields;
    uint8_t buf[13];
} cfg_t;

enum {
    OPMODE_SLEEP = 0,
    OPMODE_STANDBY,
    OPMODE_FS,
    OPMODE_RX,
    OPMODE_TX,
    OPMODE_FAIL
};

typedef enum {
    /* 0 */ IRQ_TYPE_PKT = 0,
    /* 1 */ IRQ_TYPE_RSSI,
    /* 2 */ IRQ_TYPE_CF,
    /* 3 */ IRQ_TYPE_BEACON_DUR,
    /* 4 */ IRQ_TYPE_RANDOM,
    /* 5 */ IRQ_TYPE_FSK_MODEM,
    /* 6 */ IRQ_TYPE_FSK_PKT,
    /* 7 */ IRQ_TYPE_FSK_SYNC,
    /* 8 */ IRQ_TYPE_TXDBM,
    /* 9 */ IRQ_TYPE_LORA_MODEM,
    /*10 */ IRQ_TYPE_LORA_PKT,
    /*11 */ IRQ_TYPE_OPMODE
} irq_type_e;

typedef union {
    struct __attribute__((packed)) {
        struct {
            uint8_t rx_pkt          : 1;    // 0
            uint8_t rx_pkt_overrun  : 1;    // 1
            uint8_t unused          : 2;    // 2,3
            uint8_t irq_type        : 4;    // 4,5,6,7
        } flags;
        uint8_t pkt_len;    // if flags.rx_pkt
        int8_t pkt_snr;     // if flags.rx_pkt
        uint16_t rx_slot;
        int16_t rssi;    // dBm * 10: if flags.rx_pkt or flags.bg_rssi
    } fields;
    uint8_t buf[11];
} irq_t;

extern const uint8_t cmd_to_length[];

