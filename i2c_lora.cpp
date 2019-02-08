#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include <i2c_lora.h>


uint8_t Radio::irq_pin;
i2c_flags_t Radio::flags;
void (*Radio::OnRadioRxDone)(uint16_t, uint8_t, float, float, const uint8_t*) = NULL;
void (*Radio::background_rssi)(float) = NULL;
irq_type_e Radio::waitingFor;
irq_t Radio::irq;

int fd;

int Radio::i2c_lora_setup(uint8_t sa, unsigned irqPin)
{
    if (wiringPiSetup () < 0) {
        fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno)) ;
        return -1;
    }
    pinMode(irqPin, INPUT);
    irq_pin = irqPin;

    fd = wiringPiI2CSetup(sa);
    if (fd < 0) {
        printf("i2c setup fail\n");
        return -1;
    }
    return 0;
}

void Radio::i2c_lora_close()
{
    close(fd);
}

int Radio::get_tx_dbm(int8_t* s8)
{
    uint8_t dummy[8];
    int result;
    time_t start;

    while (service())
        printf("stale irq\n");

    waitingFor = IRQ_TYPE_TXDBM;
    start = time(NULL);
    result =  i2c_smbus_write_i2c_block_data(fd, CMD_TXDBM_REQ, cmd_to_length[CMD_TXDBM_REQ], dummy);
    if (result < 0) {
        perror("CMD_TXDBM_REQ");
        return -1;
    }
    while (waitingFor == IRQ_TYPE_TXDBM) {
        service();
        if (time(NULL) > start+2) {
            printf("get_tx_dbm timeout\n");
            return -1;
        }
    }

    *s8 = irq.buf[1];

    return 0;
}

int Radio::set_tx_dbm(int8_t s8)
{
    unsigned len;
    len = cmd_to_length[CMD_TXDBM_WRITE];
    return i2c_smbus_write_i2c_block_data(fd, CMD_TXDBM_WRITE, len, (uint8_t*)&s8);
}

int Radio::SetChannel(unsigned hz)
{
    uint8_t buf[4];
    buf[0] = hz & 0xff;
    hz >>= 8;
    buf[1] = hz & 0xff;
    hz >>= 8;
    buf[2] = hz & 0xff;
    hz >>= 8;
    buf[3] = hz & 0xff;

	service();	// channel cannot be set during pending interrupt

    return i2c_smbus_write_i2c_block_data(fd, CMD_CFHZ_WRITE, cmd_to_length[CMD_CFHZ_WRITE], buf);
}

int Radio::Rx(unsigned timeout)
{
    uint8_t buf[4];
    buf[0] = timeout & 0xff;
    timeout >>= 8;
    buf[1] = timeout & 0xff;
    timeout >>= 8;
    buf[2] = timeout & 0xff;
    timeout >>= 8;
    buf[3] = timeout & 0xff;
    return i2c_smbus_write_i2c_block_data(fd, CMD_RX_START, cmd_to_length[CMD_RX_START], buf);
}

int Radio::SetLoRaSymbolTimeout(uint8_t symbs)
{
    return i2c_smbus_write_i2c_block_data(fd, CMD_LORA_SYMBTO_WRITE, cmd_to_length[CMD_LORA_SYMBTO_WRITE], &symbs);
}

int Radio::GetFSKSync(uint8_t* sync_len, uint8_t* sync_value)
{
    uint8_t dummy[8];
    int result;
    time_t start;

    while (service())
        printf("stale irq\n");

    waitingFor = IRQ_TYPE_FSK_SYNC;
    start = time(NULL);
    result =  i2c_smbus_write_i2c_block_data(fd, CMD_FSK_SYNC_REQ, cmd_to_length[CMD_FSK_SYNC_REQ], dummy);
    if (result < 0) {
        perror("CMD_FSK_SYNC_REQ");
        return -1;
    }
    while (waitingFor == IRQ_TYPE_FSK_SYNC) {
        service();
        if (time(NULL) > start+2) {
            printf("getfsksync timeout\n");
            return -1;
        }
    }

    *sync_len = irq.buf[1];
    memcpy(sync_value, irq.buf+2, *sync_len);

    return 0;
}

int Radio::GetGFSKModemConfig(unsigned* bps, unsigned* bwKHz, unsigned* fdev_hz)
{
    uint8_t dummy[8];
    int result;
    time_t start;

    while (service())
        printf("stale irq\n");

    waitingFor = IRQ_TYPE_FSK_MODEM;
    start = time(NULL);
    result =  i2c_smbus_write_i2c_block_data(fd, CMD_FSK_MODEM_CFG_REQ, cmd_to_length[CMD_FSK_MODEM_CFG_REQ], dummy);
    if (result < 0) {
        perror("CMD_FSK_MODEM_CFG_REQ");
        return -1;
    }
    while (waitingFor == IRQ_TYPE_FSK_MODEM) {
        service();
        if (time(NULL) > start+2) {
            printf("getfskmodem timeout\n");
            return -1;
        }
    }

    *bps = irq.buf[4];
    *bps <<= 8;
    *bps |= irq.buf[3];
    *bps <<= 8;
    *bps |= irq.buf[2];
    *bps <<= 8;
    *bps |= irq.buf[1];

    *bwKHz = irq.buf[6];
    *bwKHz <<= 8;
    *bwKHz |= irq.buf[5];

    *fdev_hz = irq.buf[10];
    *fdev_hz <<= 8;
    *fdev_hz |= irq.buf[9];
    *fdev_hz <<= 8;
    *fdev_hz |= irq.buf[8];
    *fdev_hz <<= 8;
    *fdev_hz |= irq.buf[7];

    return 0;
}

int Radio::GFSKModemConfig(unsigned bps, unsigned bwKHz, unsigned fdev_hz)
{
    uint8_t buf[cmd_to_length[CMD_FSK_MODEM_CFG_WRITE]];

    buf[0] = bps & 0xff;
    bps >>= 8;
    buf[1] = bps & 0xff;
    bps >>= 8;
    buf[2] = bps & 0xff;
    bps >>= 8;
    buf[3] = bps & 0xff;

    buf[4] = bwKHz & 0xff;
    bwKHz >>= 8;
    buf[5] = bwKHz & 0xff;

    buf[6] = fdev_hz & 0xff;
    fdev_hz >>= 8;
    buf[7] = fdev_hz & 0xff;
    fdev_hz >>= 8;
    buf[8] = fdev_hz & 0xff;
    fdev_hz >>= 8;
    buf[9] = fdev_hz & 0xff;

    return i2c_smbus_write_i2c_block_data(fd, CMD_FSK_MODEM_CFG_WRITE, cmd_to_length[CMD_FSK_MODEM_CFG_WRITE], buf);
}

int Radio::GetGFSKPacketConfig(unsigned* preambleLen, bool* fixLen, bool* crcOn)
{
    uint8_t dummy[8];
    int result;
    time_t start;

    while (service())
        printf("stale irq\n");

    waitingFor = IRQ_TYPE_FSK_PKT;
    start = time(NULL);
    result =  i2c_smbus_write_i2c_block_data(fd, CMD_FSK_PKT_CFG_REQ, cmd_to_length[CMD_FSK_PKT_CFG_REQ], dummy);
    if (result < 0) {
        perror("CMD_FSK_PKT_CFG_REQ");
        return -1;
    }
    while (waitingFor == IRQ_TYPE_FSK_PKT) {
        service();
        if (time(NULL) > start+2) {
            printf("getfskpkt timeout\n");
            return -1;
        }
    }

    *preambleLen = irq.buf[2];
    *preambleLen <<= 8;
    *preambleLen |= irq.buf[1];

    *fixLen = irq.buf[3] ? true : false;
    *crcOn = irq.buf[4] ? true : false;

    return 0;
}

int Radio::GFSKPacketConfig(unsigned preambleLen, bool fixLen, bool crcOn)
{
    uint8_t buf[cmd_to_length[CMD_FSK_PKT_CFG_WRITE]];

    buf[0] = preambleLen & 0xff;
    preambleLen >>= 8;
    buf[1] = preambleLen & 0xff;

	buf[2] = fixLen;
	buf[3] = crcOn;

    return i2c_smbus_write_i2c_block_data(fd, CMD_FSK_PKT_CFG_WRITE, cmd_to_length[CMD_FSK_PKT_CFG_WRITE], buf);
}

int Radio::GetLoRaModemConfig(unsigned* KHz, unsigned* sf, unsigned* cr)
{
    uint8_t dummy[8];
    int result;
    time_t start;

    while (service())
        printf("stale irq\n");

    waitingFor = IRQ_TYPE_LORA_MODEM;
    start = time(NULL);
    result = i2c_smbus_write_i2c_block_data(fd, CMD_LORA_MODEM_CFG_REQ, cmd_to_length[CMD_LORA_MODEM_CFG_REQ], dummy);
    if (result < 0) {
        perror("CMD_LORA_PKT_CFG_REQ");
        return -1;
    }
    while (waitingFor == IRQ_TYPE_LORA_MODEM) {
        service();
        if (time(NULL) > start+2) {
            printf("GetLoRaModem timeout\n");
            return -1;
        }
    }

    *KHz = irq.buf[2];
    *KHz <<= 8;
    *KHz |= irq.buf[1];

    *sf = irq.buf[3];
    *cr = irq.buf[4];

    return 0;
}

int Radio::LoRaModemConfig(unsigned KHz, unsigned sf, unsigned cr)
{
    uint8_t buf[4];
    buf[0] = KHz & 0xff;
    KHz >>= 8;
    buf[1] = KHz & 0xff;
    buf[2] = sf;
    buf[3] = cr;

    return i2c_smbus_write_i2c_block_data(fd, CMD_LORA_MODEM_CFG_WRITE, cmd_to_length[CMD_LORA_MODEM_CFG_WRITE], buf);
}

int Radio::GetLoRaPacketConfig(unsigned* preambleLen, bool* fixLen, bool* crcOn, bool* invIQ)
{
    uint8_t dummy[8];
    int result;
    time_t start;

    while (service())
        printf("stale irq\n");

    waitingFor = IRQ_TYPE_LORA_PKT;
    start = time(NULL);
    result =  i2c_smbus_write_i2c_block_data(fd, CMD_LORA_PKT_CFG_REQ, cmd_to_length[CMD_LORA_PKT_CFG_REQ], dummy);
    if (result < 0) {
        perror("CMD_LORA_PKT_CFG_REQ");
        return -1;
    }
    while (waitingFor == IRQ_TYPE_LORA_PKT) {
        service();
        if (time(NULL) > start+2) {
            printf("getlorapacket timeout\n");
            return -1;
        }
    }

    *preambleLen = irq.buf[2];
    *preambleLen <<= 8;
    *preambleLen |= irq.buf[1];

    *fixLen = irq.buf[3] ? true : false;
    *invIQ = irq.buf[4] ? true : false;
    *crcOn = irq.buf[5] ? true : false;

    return 0;
}

int Radio::LoRaPacketConfig(unsigned preambleLen, bool fixLen, bool crcOn, bool invIQ)
{
    uint8_t buf[4];
    pktcfg_t pktcfg;

    buf[0] = preambleLen & 0xff;
    preambleLen >>= 8;
    buf[1] = preambleLen & 0xff;

    pktcfg.bits.fixLen = fixLen;
    pktcfg.bits.crcOn = crcOn;
    pktcfg.bits.invIQ = invIQ;
    buf[2] = pktcfg.octet;

    return i2c_smbus_write_i2c_block_data(fd, CMD_LORA_PKT_CFG_WRITE, cmd_to_length[CMD_LORA_PKT_CFG_WRITE], buf);
}

int Radio::Config(const cfg_t* cfg)
{
    return i2c_smbus_write_i2c_block_data(fd, CMD_CFG, cmd_to_length[CMD_CFG], cfg->buf);
}

int Radio::Standby()
{
    uint8_t o = 0;
    return i2c_smbus_write_i2c_block_data(fd, CMD_STANDBY, cmd_to_length[CMD_STANDBY], &o);
}

int Radio::SetPublicNetwork(bool pub)
{
    uint8_t o = pub ? true : false;
    return i2c_smbus_write_i2c_block_data(fd, CMD_PUBLIC_NET, cmd_to_length[CMD_PUBLIC_NET], &o);
}

int Radio::SetRxMaxPayloadLength(uint8_t paylen)
{
    return i2c_smbus_write_i2c_block_data(fd, CMD_MAX_PAYLEN_WRITE, cmd_to_length[CMD_MAX_PAYLEN_WRITE], &paylen);
}

int Radio::BeaconCfg(unsigned secs, unsigned preambleSize, uint8_t beaconSize)
{
    uint32_t u32;
    int result;
    unsigned i;
    uint8_t buf[8];
    time_t start;
    printf("secs%u ps%u: ", secs, preambleSize);
    buf[0] = secs & 0xff;
    secs >>= 8;
    buf[1] = secs & 0xff;

    buf[2] = preambleSize & 0xff;
    preambleSize >>= 8;
    buf[3] = preambleSize & 0xff;

    buf[4] = beaconSize;

    printf("{%u} ", cmd_to_length[CMD_BEACON_CFG_WRITE]);
    for (i = 0; i < cmd_to_length[CMD_BEACON_CFG_WRITE]; i++)
        printf("%02x ", buf[i]);
    printf("\n");

    while (service())
        printf("stale irq\n");

    waitingFor = IRQ_TYPE_BEACON_DUR;
    start = time(NULL);

    result = i2c_smbus_write_i2c_block_data(fd, CMD_BEACON_CFG_WRITE, cmd_to_length[CMD_BEACON_CFG_WRITE], buf);
    if (result < 0) {
        perror("CMD_BEACON_CFG_WRITE");
        return -1;
    }

    while (waitingFor == IRQ_TYPE_BEACON_DUR) {
        service();
        if (time(NULL) > start+4) {
            printf("get beaconDur timeout\n");
            return -1;
        }
    }

    u32 = irq.buf[4];
    u32<<= 8;
    u32 |= irq.buf[3];
    u32<<= 8;
    u32 |= irq.buf[2];
    u32<<= 8;
    u32 |= irq.buf[1];

    return u32;
}


int Radio::get_opmode()
{
    uint8_t dummy[8];
    int result;
    time_t start;

    while (service())
        printf("stale irq\n");

    waitingFor = IRQ_TYPE_OPMODE;
    start = time(NULL);
    result = i2c_smbus_write_i2c_block_data(fd, CMD_OPMODE_REQ, cmd_to_length[CMD_OPMODE_REQ], dummy);
    if (result < 0) {
        perror("IRQ_TYPE_OPMODE");
        return -1;
    }

    while (waitingFor == IRQ_TYPE_OPMODE) {
        service();
        if (time(NULL) > start+2) {
            printf("get_opmode timeout\n");
            return -1;
        }
    }

    return irq.buf[1];
}

unsigned Radio::GetRandom()
{
    uint32_t u32;
    uint8_t dummy[cmd_to_length[CMD_REQ_RANDOM]];
    int result;
    time_t start;

    while (service())
        printf("stale irq\n");

    waitingFor = IRQ_TYPE_RANDOM;
    start = time(NULL);
    result = i2c_smbus_read_i2c_block_data(fd, CMD_REQ_RANDOM, cmd_to_length[CMD_REQ_RANDOM], dummy);
    if (result < 0) {
        perror("CMD_REQ_RANDOM");
        return 0;
    }

    while (waitingFor == IRQ_TYPE_RANDOM) {
        service();
        if (time(NULL) > start+2) {
            printf("GetRandom timeout\n");
            return 0;
        }
    }

    u32 = irq.buf[4];
    u32 <<= 8;
    u32 |= irq.buf[3];
    u32 <<= 8;
    u32 |= irq.buf[2];
    u32 <<= 8;
    u32 |= irq.buf[1];
    return u32;
}

float Radio::GetCF()
{
    uint32_t u32;
    uint8_t dummy[8];
    int result;
    time_t start;

    while (service())
        printf("stale irq\n");

    waitingFor = IRQ_TYPE_CF;
    start = time(NULL);
    result =  i2c_smbus_write_i2c_block_data(fd, CMD_CFHZ_REQ, cmd_to_length[CMD_CFHZ_REQ], dummy);
    if (result < 0) {
        perror("CMD_CFHZ_REQ");
        return FLT_MIN;
    }
    while (waitingFor == IRQ_TYPE_CF) {
        service();
        if (time(NULL) > start+2) {
            printf("GetCF timeout\n");
            return FLT_MIN;
        }
    }

    u32 = irq.buf[4];
    u32 <<= 8;
    u32 |= irq.buf[3];
    u32 <<= 8;
    u32 |= irq.buf[2];
    u32 <<= 8;
    u32 |= irq.buf[1];
    return u32 / 1000000.0;
}

float Radio::GetAmbient(uint8_t n_samp)
{
    time_t start = time(NULL);
    int result;

    waitingFor = IRQ_TYPE_RSSI;
    flags.solicited = 1;
    result = i2c_smbus_write_i2c_block_data(fd, CMD_RSSI_REQ, cmd_to_length[CMD_RSSI_REQ], &n_samp);
    if (result < 0) {
        perror("CMD_REQ_RSSI");
        return FLT_MIN;
    }

    while (waitingFor == IRQ_TYPE_RSSI) {
        service();
        if (time(NULL) > start+2) {
            printf("rssi timeout\n");
            return FLT_MIN;
        }
    }

    return irq.fields.rssi / 10.0;
}

int Radio::Send()
{
    uint8_t dummy[32];
    return i2c_smbus_write_i2c_block_data(fd, CMD_SEND, cmd_to_length[CMD_SEND], dummy);
}

int Radio::reset()
{
    uint8_t dummy[32];
    return i2c_smbus_write_i2c_block_data(fd, CMD_RADIO_RESET, cmd_to_length[CMD_RADIO_RESET], dummy);
}

int Radio::GetCurrentSlot()
{
    uint16_t slot;
    int result = i2c_smbus_read_i2c_block_data(fd, CMD_CURRENT_SLOT, cmd_to_length[CMD_CURRENT_SLOT], (uint8_t*)&slot);
    if (result < 0) {
        perror("read-CMD_CURRENT_SLOT");
        return result;
    }
    return slot;
}

int Radio::GetBeaconSkip()
{
    uint8_t cnt;
    int result = i2c_smbus_read_i2c_block_data(fd, CMD_SKIP_BEACONS, cmd_to_length[CMD_SKIP_BEACONS], &cnt);
    if (result < 0) {
        perror("read-CMD_SKIP_BEACONS");
        return result;
    }
    return cnt;
}

int Radio::SetBeaconSkip(uint8_t cnt)
{
    return i2c_smbus_write_i2c_block_data(fd, CMD_SKIP_BEACONS, cmd_to_length[CMD_SKIP_BEACONS], &cnt);
}

int Radio::LoadTxPacket(const uint8_t* txbuf, uint8_t txlen)
{
    int result;
    uint8_t i2cbuf[cmd_to_length[CMD_TX_BUF_START]];
    unsigned txbuf_idx;

    i2cbuf[0] = txlen;
    memcpy(i2cbuf+1, txbuf, sizeof(i2cbuf)-1);
    txbuf_idx = sizeof(i2cbuf)-1;
    result = i2c_smbus_write_i2c_block_data(fd, CMD_TX_BUF_START, cmd_to_length[CMD_TX_BUF_START], &i2cbuf[0]);
    if (result < 0) {
        perror("CMD_TX_BUF_START");
        return -1;
    }

    while (txbuf_idx < txlen) {
        memcpy(i2cbuf, txbuf+txbuf_idx, cmd_to_length[CMD_TX_BUF]);
        result = i2c_smbus_write_i2c_block_data(fd, CMD_TX_BUF, cmd_to_length[CMD_TX_BUF], i2cbuf);
        if (result < 0) {
            perror("CMD_TX_BUF");
            return -1;
        }
        txbuf_idx += cmd_to_length[CMD_TX_BUF];
    }

    return 0;
}

bool Radio::service()
{
    static uint8_t prev_rx_buf[256];
    static uint8_t prev_pkt_len = 0;
    char str[64];
    bool ret = false;

    while (digitalRead(irq_pin)) {
        int result = i2c_smbus_read_i2c_block_data(fd, CMD_IRQ, cmd_to_length[CMD_IRQ], irq.buf);
        ret = true;
        if (result < 0) {
            sprintf(str, "i2c_read_CMD_IRQ");
            perror(str);
            usleep(100000);
            return ret;
        }

        if (irq.buf[0] == 0) {
            /* irq pin raised, but no pending irq: disconnected wire? */
            printf("\e[31mno irq\e[0m\n");
            usleep(100000);
            return ret;
        }

        if (irq.fields.flags.rx_pkt || irq.fields.flags.rx_pkt_overrun) {
            uint8_t rx_buf[256];
            unsigned nread;
            if (irq.fields.flags.rx_pkt_overrun)
                printf("\e[31moverrun\e[0m ");

            if (prev_pkt_len == irq.fields.pkt_len)
                printf("rx pkt_len=%u ", irq.fields.pkt_len);
            else
                printf("\e43mrx pkt_len=%u\e[0m ", irq.fields.pkt_len);

            printf("snr=%d, rssi=%.1f rxSlot:%u ", irq.fields.pkt_snr, irq.fields.rssi / 10.0, irq.fields.rx_slot);
            for (nread = 0; nread < irq.fields.pkt_len; ) {
                int n;
                result = i2c_smbus_read_i2c_block_data(fd, CMD_RX_PAYLOAD, cmd_to_length[CMD_RX_PAYLOAD], rx_buf+nread);
                if (result < 0) {
                    perror("CMD_RX_PAYLOAD");
                    printf("\e[33mpayload-read-fail at %u\e[0m ", nread);
                    usleep(50000);
                    continue;
                }
                if (result != cmd_to_length[CMD_RX_PAYLOAD])
                    printf("\e[33mnread=%u rx payload result %d\e[0m ", nread, result);

                for (n = 0; n < result; n++) {
                    if ((nread + n) == irq.fields.pkt_len)
                        printf("\e[34m");
                    if (rx_buf[nread+n] == 0xff)
                        printf("\e[7m%02x\e[0m", rx_buf[nread+n]);
                    else if (rx_buf[nread+n] == prev_rx_buf[nread+n])
                        printf("%02x", rx_buf[nread+n]);
                    else
                        printf("\e[41m%02x\e[0m", rx_buf[nread+n]);
                }
                printf(" ");
                nread += result;
            }
            printf("\e[0m");

            if (OnRadioRxDone)
               OnRadioRxDone(irq.fields.rx_slot, irq.fields.pkt_len, irq.fields.rssi / 10.0, irq.fields.pkt_snr, rx_buf);

            memcpy(prev_rx_buf, rx_buf, irq.fields.pkt_len);
            prev_pkt_len = irq.fields.pkt_len;
        } else {
            if (irq.fields.flags.irq_type == IRQ_TYPE_RSSI) {
                float rssi = irq.fields.rssi / 10.0;
                if (!flags.solicited) {
                    /* ambient energy was measured */
                    if (background_rssi)
                        background_rssi(rssi);
                } else
                    flags.solicited = 0;
            }

            if (waitingFor == irq.fields.flags.irq_type) {
                /* indictate received that which was expected */
                waitingFor = IRQ_TYPE_PKT;
            }
        }

    } // ..while irq pin asserted

    return ret;
}

