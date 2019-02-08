#include <stdio.h>
#include <errno.h>
#include <i2c_lora.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <json-c/json.h>

#define IRQ_PIN     7   /* wiringPi 7 = BCM GPIO 4 */
#define SLAVE_ADDRESS       0x50

bool svcRadio;

unsigned serviceRate;
int tx_dbm;
unsigned rssiSamples;
unsigned lora_sf;
unsigned bw_khz;
unsigned lora_cr;
unsigned preamble_symbs;
unsigned cf_hz;
unsigned beacon_size;

unsigned rand_max_ns;
unsigned rand_min_ns;

void init_radio(unsigned cfhz, unsigned bwkhz, unsigned sf)
{
    cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg_t));

    cfg.fields.flags.rxOnTxDone = 1;
    cfg.fields.n_rssi_samples = rssiSamples;
    Radio::Config(&cfg);

    Radio::Standby( );

    Radio::set_tx_dbm(tx_dbm);

    Radio::LoRaModemConfig(bwkhz, sf, lora_cr);
    printf("using sf%u, %uKHz\r\n", sf, bwkhz);
    Radio::SetPublicNetwork(true);

    Radio::SetChannel(cfhz);

	fflush(stdout);
    Radio::SetRxMaxPayloadLength(255);
}

int cmd_fsk_packet(char* argv[])
{
    unsigned preambleLen;
    bool fixLen, crcOn;
    if (Radio::GetGFSKPacketConfig(&preambleLen, &fixLen, &crcOn) < 0)
        return -1;

    printf("fsk preableLen%u  %s   %s\r\n", preambleLen,
        fixLen ? "fixed" : "variable",
        crcOn ? "crcOn" : "crc-off"
    );

    return 0;
}

int cmd_fsk_modem(char* argv[])
{
    unsigned bps, bwKHz, fdev_hz;
    if (Radio::GetGFSKModemConfig(&bps, &bwKHz, &fdev_hz) < 0)
        return -1;

    printf("%ubps  bw:%uKHz  fdev:%u\r\n", bps, bwKHz, fdev_hz);

    return 0;
}

int cmd_fsk_sync(char* argv[])
{
    uint8_t i, len;
    uint8_t sync[8];
    if (Radio::GetFSKSync(&len, sync) < 0)
        return -1;

    printf("fsk sync: ");
    for (i = 0; i < len; i++)
        printf("%02x ", sync[i]);

    printf("\r\n");
    return 0;
}

int cmd_loraModemConfigGet(char* argv[])
{
    unsigned khz, sprfact, coderate;
    if (Radio::GetLoRaModemConfig(&khz, &sprfact, &coderate) < 0)
        return -1;

    printf("%uKHz sf%u cr%u\r\n", khz, sprfact, coderate);
    return 0;
}

int cmd_loraPacketConfigGet(char* argv[])
{
    unsigned pl;
    bool fixed, crc_on, inv_iq;
    if (Radio::GetLoRaPacketConfig(&pl, &fixed, &crc_on, &inv_iq) < 0)
        return -1;

    printf("preambleLength:%u  %s   %s   %s\r\n", pl,
            fixed ? "implicit" : "explicit",
            crc_on ? "crcOn" : "crc-off",
            inv_iq ? "IQ-invert" : "normal-iq"
    );
    return 0;
}

int cmd_status(char* argv[])
{
    unsigned pl;
    unsigned khz, sprfact, coderate;
    bool fixed, crc_on, inv_iq;
    int op = Radio::get_opmode();
    if (op < 0) {
        printf("get_opmode failed\r\n");
        return -1;
    }

    printf("OPMODE_");
	switch (op) {
		case OPMODE_SLEEP: printf("SLEEP"); break;
		case OPMODE_STANDBY: printf("STANDBY"); break;
		case OPMODE_FS: printf("FS"); break;
		case OPMODE_RX: printf("RX"); break;
		case OPMODE_TX: printf("TX"); break;
		case OPMODE_FAIL: printf("FAIL"); break;
		default: printf("?%d?", op); break;
	}

	printf(" ");
    float MHz = Radio::GetCF();
    if (MHz == FLT_MIN) {
        return -1;
	}
	printf("%.3fMHz ", MHz);

    if (Radio::GetLoRaModemConfig(&khz, &sprfact, &coderate) < 0)
        return -1;

    printf("%uKHz sf%u cr%u ", khz, sprfact, coderate);

    if (Radio::GetLoRaPacketConfig(&pl, &fixed, &crc_on, &inv_iq) < 0)
        return -1;

    printf("preambleLength:%u  %s   %s   %s\r\n", pl,
            fixed ? "implicit" : "explicit",
            crc_on ? "crcOn" : "crc-off",
            inv_iq ? "IQ-invert" : "normal-iq"
    );

    return 0;
}

int cmd_tx(char* argv[])
{
    unsigned tx_buf_len;
    uint8_t tx_buf[256];
    if (argv[0] == NULL) {
        printf("give packet length\r\n");
        return -1;
    }
    sscanf(argv[0], "%u", &tx_buf_len);

    init_radio(cf_hz, bw_khz, lora_sf);

    if (tx_buf_len > sizeof(tx_buf)) {
        tx_buf_len = sizeof(tx_buf);
	}

	printf("loading..\r\n");
	fflush(stdout);
    if (Radio::LoadTxPacket(tx_buf, tx_buf_len) < 0) {
        printf("\e[33mLoadTxPacket fail\e[0\r\n");
        return -1;
    }

    if (Radio::Send() < 0) {
        printf("\e[33mSend fail\e[0\r\n");
        return -1;
    }

    return 0;
}

int cmd_test_read(char* argv[])
{
    unsigned reps = 1;
    int result;
    uint8_t buf[32];
    unsigned i, len;
    uint8_t cmd;
    if (argv[0] == NULL) {
        printf("give length 3 or 32\r\n");
        return -1;
    }
    sscanf(argv[0], "%u", &len);
    if (len == 3)
        cmd = CMD_TEST;
    else if (len == 32)
        cmd = CMD_TEST32;
    else {
        printf("dont have length %u\r\n", len);
        return -1;
    }

    if (argv[1]) {
        sscanf(argv[1], "%u", &reps);
    }

    while (reps > 0) {
        result = i2c_smbus_read_i2c_block_data(fd, cmd, len, buf);
        if (result < 0) {
            perror("test_read");
            return -1;
        }
        printf("test read %u: ", len);
        for (i = 0; i < len; i++)
            printf("%02x ", buf[i]);
        printf("\n");
        reps--;
    }

    return 0;
}

int cmd_test_write(char* argv[])
{
    unsigned reps = 1;
    int result = -1;
    uint8_t buf[32];
    unsigned i, len;
    uint8_t cmd;
    if (argv[0] == NULL) {
        printf("give length 3 or 32\r\n");
        return -1;
    }
    sscanf(argv[0], "%u", &len);
    if (len == 3) {
        printf("tw3 ");
        cmd = CMD_TEST;
    } else if (len == 32) {
        printf("tw32 ");
        cmd = CMD_TEST32;
    } else {
        printf("dont have length %u\r\n", len);
        return -1;
    }

    for (i = 0; i < len; i++)
        buf[i] = i;

    if (argv[1]) {
        sscanf(argv[1], "%u", &reps);
    }

    printf("cmd:%02x len%u\r\n", cmd, cmd_to_length[cmd]);
    while (reps > 0) {
        result = i2c_smbus_write_i2c_block_data(fd, cmd, cmd_to_length[cmd], buf);
        if (result < 0) {
            perror("test write");
            return -1;
        }
        printf("result:%d\r\n", result);
        reps--;
    }
    return result;
}


int cmd_rx(char* argv[])
{
    if (Radio::Rx(0) < 0)
        return -1;

    printf("rxStart\n");
    return 0;
}

int cmd_rssi(char* argv[])
{
    unsigned samps = 1;
    float dbm;
    printf("rxing at %.3fMHz\r\n", cf_hz / 1000000.0);
    Radio::SetChannel(cf_hz);
    Radio::Rx(0);
    if (argv[0])
        sscanf(argv[0], "%u", &samps);

    dbm = Radio::GetAmbient(samps);
    printf("dbm:%.1f\n", dbm);
    if (dbm == FLT_MIN)
        return -1;
    else
        return 0;
}

int cmd_bs(char* argv[])
{
    int cnt = Radio::GetBeaconSkip();
    if (cnt < 0) {
        printf("GetBeaconSkip failed\r\n");
        return -1;
    }
    printf("GetBeaconSkip: %d\r\n", cnt);
    return 0;
}

struct _sr_ {
    long nsec_next;
    bool enable;
    struct timespec lastReqAt;
} svcReq;

int cmd_bt(char* argv[])
{
    if (argv[0]) {
        int ret;
        unsigned secs, beaconDur;
        printf("beacon test\n");
        init_radio(cf_hz, bw_khz, lora_sf);
        if (sscanf(argv[0], "%u", &secs) == 0) {
            printf("give seconds\n");
            return -1;
        }
        printf("%useconds ", secs);
        ret = Radio::BeaconCfg(secs, preamble_symbs, beacon_size);
        if (ret < 0)
            return -1;
        beaconDur = ret;
        printf("beaconDur:%u, 0x%x\r\n", beaconDur, beaconDur);
        svcRadio = true;

        printf("argv[1]:%s\r\n", argv[1]);
        if (argv[1] && argv[1][0] == 'r') {
            /* random cmds during beacon */
            svcReq.enable = true;
            svcReq.nsec_next = (rand() % rand_max_ns) + rand_min_ns;
            svcReq.nsec_next = (rand() % 100000000) + 100000000;
            printf("nsec_next:%ld\r\n", svcReq.nsec_next);
            if (clock_gettime(CLOCK_MONOTONIC, &svcReq.lastReqAt) < 0)
                perror("clock_gettime");
        }
        return 0;
    } else {
        printf("give seconds\n");
        return -1;
    }
}

int cmd_lmc(char* argv[])
{
    int result;
    if (argv[0] && argv[1] && argv[2]) {
        /* set */
        unsigned khz, sf, cr;
        sscanf(argv[0], "%u", &khz);
        sscanf(argv[1], "%u", &sf);
        sscanf(argv[2], "%u", &cr);
        result = Radio::LoRaModemConfig(khz, sf, cr);
        printf("%d = LoRaModemConfig(%u, %u, %u)\r\n", result, khz, sf, cr);
        return result;
    } else {
        /* get */
        printf("TODO get lora modem config\n");
        return -1;
    }
}

int cmd_rand(char* argv[])
{
    unsigned r = Radio::GetRandom();
    printf("random %08x\r\n", r);
    if (r == 0)
        return -1;
    else
        return 0;
}

int get_cf_test()
{
    float MHz = Radio::GetCF();
	if (MHz == FLT_MIN) {
		printf("\e[41mGetCF failed\e[0m\r\n");
        return -1;
	}
    if (MHz < 0.1) {
        printf("\e[41m%.2fMHz\e[0m\n", MHz);
        return -1;
    }
    printf("%.2fMHz\n", MHz);
    return 0;
}

int cmd_cf_write(char* argv[])
{
    int result;
    float MHz;
    sscanf(argv[0], "%f", &MHz);
    result = Radio::SetChannel(MHz * 1000000);
    printf("%d = SetChannel(%f)\n", result, MHz);
    return result;
}

int cmd_cf_read(char* argv[])
{
    unsigned rep, reps = 1;
    if (argv[0]) {
        if (sscanf(argv[0], "%u", &reps) == 0) {
            printf("reps integer\r\n");
            return -1;
        }
    }
    for (rep = 0; rep < reps; rep++) {
        if (get_cf_test() < 0)
            return -1;
    }
    return 0;
}

int cmd_dbm(char* argv[])
{
    int result, dbm;
    if (argv[0]) {
        sscanf(argv[0], "%d", &dbm);
        printf("sending %ddBm\n", dbm);
        result = Radio::set_tx_dbm(dbm);
        printf("%d = set_tx_dbm()\n", result);
        return result;
    } else {
        int8_t dbm8;
        if (Radio::get_tx_dbm(&dbm8) < 0)
            return -1;
        printf("dbm:%d\n", dbm8);
        return 0;
    }
}

typedef struct {
    const char* const cmd;
    int (*handler)(char* argv[]);
    const char* const arg_descr;
    const char* const description;
} menu_item_t;

const menu_item_t menu_items[] = 
{
    { "fp", cmd_fsk_packet, NULL, "request fsk packet config" },
    { "fm", cmd_fsk_modem, NULL, "request fsk modem config" },
    { "fsksync", cmd_fsk_sync, NULL, "request fsk sync" },
    { "lmr", cmd_loraModemConfigGet, NULL, "request lora modem config" },
    { "lpr", cmd_loraPacketConfigGet, NULL, "request lora packet config" },
    { "status", cmd_status, NULL, "read chip config" },
    { "tx", cmd_tx, "nbytes", "test rf tx" },
    { "tr", cmd_test_read, "nbytes", "test i2c read" },
    { "tw", cmd_test_write, "nbytes", "test i2c write" },
    { "rx", cmd_rx, NULL, "start radio rx" },
    { "rssi", cmd_rssi, NULL, "get ambient rssi" },
    { "bt", cmd_bt, "seconds", "run beacon test" },
    { "bs", cmd_bs, NULL, "read beacon skip" },
    { "lmc", cmd_lmc, "khz sf cr", "get/set lora modem config" },
    { "rand", cmd_rand, NULL, "get random number" },
    { "cfw", cmd_cf_write, "MHz", "set center frequency" },
    { "cfr", cmd_cf_read, "reps", "get center frequency" },
    { "dbm", cmd_dbm, "dBm", "get/set tx power" },
    { NULL, NULL, NULL, NULL }
};

void cmd_help(uint8_t args_at)
{
    int i;
    
    printf("usage:\n");
    for (i = 0; menu_items[i].cmd != NULL ; i++) {
        printf("%s ", menu_items[i].cmd);
        if (menu_items[i].arg_descr)
            printf("<%s>", menu_items[i].arg_descr);
        else
            printf("\t");

        printf("\t%s\r\n", menu_items[i].description);
    }
    
}

int load_json_config(void)
{
    int len, ret = -1;
    json_object *top, *obj;
    enum json_tokener_error jerr;
    struct json_tokener *tok = json_tokener_new();
    FILE *file;
    const char* confFile ="../test_conf.json";

    file = fopen(confFile, "r");

    if (file == NULL) {
        perror(confFile);
        return -1;
    }

    do {
        char line[96];
        if (fgets(line, sizeof(line), file) == NULL) {
            fprintf(stderr, "NULL == fgets()\n");
            goto pEnd;
        }
        len = strlen(line);
        top = json_tokener_parse_ex(tok, line, len);
        //printf("jobj:%p, len%u\n", top, len);
    } while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);

    if (jerr != json_tokener_success) {
        printf("parse_server_config() json error: %s\n", json_tokener_error_desc(jerr));
        goto pEnd;
    }

    if (json_object_object_get_ex(top, "serviceRate", &obj)) {
        serviceRate = json_object_get_int(obj);
    } else {
        printf("need serviceRate\n");
        goto pEnd;
    }

/*    if (json_object_object_get_ex(top, "beaconInterval", &obj)) {
        beacon_interval = json_object_get_int(obj);
    } else {
        printf("need serviceRate\n");
        goto pEnd;
    }*/

    if (json_object_object_get_ex(top, "tx_dbm", &obj)) {
        tx_dbm = json_object_get_int(obj);
    } else {
        printf("need tx_dbm\n");
        goto pEnd;
    }

    if (json_object_object_get_ex(top, "rssiSamples", &obj)) {
        rssiSamples = json_object_get_int(obj);
    } else {
        printf("need rssiSamples\n");
        goto pEnd;
    }

    if (json_object_object_get_ex(top, "lora_sf", &obj)) {
        lora_sf = json_object_get_int(obj);
    } else {
        printf("need lora_sf\n");
        goto pEnd;
    }

    if (json_object_object_get_ex(top, "bw_khz", &obj)) {
        bw_khz = json_object_get_int(obj);
    } else {
        printf("need bw_khz\n");
        goto pEnd;
    }

    if (json_object_object_get_ex(top, "lora_cr", &obj)) {
        lora_cr = json_object_get_int(obj);
    } else {
        printf("need lora_cr\n");
        goto pEnd;
    }

    if (json_object_object_get_ex(top, "preamble_symbs", &obj)) {
        preamble_symbs = json_object_get_int(obj);
    } else {
        printf("need preamble_symbs\n");
        goto pEnd;
    }

    if (json_object_object_get_ex(top, "cfMHz", &obj)) {
        cf_hz = json_object_get_double(obj) * 1000000;
    } else {
        printf("need cfMHz\n");
        goto pEnd;
    }

    if (json_object_object_get_ex(top, "beacon_size", &obj)) {
        beacon_size = json_object_get_int(obj);
    } else {
        printf("need beacon_size\n");
        goto pEnd;
    }

    if (json_object_object_get_ex(top, "rand_min_ms", &obj)) {
        rand_min_ns = json_object_get_int(obj) * 1000000;
    } else {
        printf("need rand_min_ms\n");
        goto pEnd;
    }

    if (json_object_object_get_ex(top, "rand_max_ms", &obj)) {
        rand_max_ns = json_object_get_int(obj) * 1000000;
    } else {
        printf("need rand_max_ms\n");
        goto pEnd;
    }

    ret = 0;

pEnd:
    if (tok)
        json_tokener_free(tok);
    fclose(file);
    return ret;

}

void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

int main(int argc, char* argv[])
{
    unsigned i;
    int ret = -1;

    if (load_json_config() < 0) {
        printf("load_json_config() failed\r\n");
        return -1;
    }

    if (Radio::i2c_lora_setup(SLAVE_ADDRESS, IRQ_PIN) < 0) {
        printf("i2c_lora_setup fail\n");
        return -1;
    }

    if (argc < 2) {
        cmd_help(0);
        return -1;
    }

    svcRadio = false;

    for (i = 0; menu_items[i].cmd != NULL ; i++) {
        const menu_item_t* mip = menu_items + i;
        if (mip->handler && (strcmp(argv[1], mip->cmd) == 0)) {
            ret = mip->handler(argv+2);
            break;
        }
    }

    if (!svcRadio)
        return ret;

    printf("svcRadio\n");
    for (;;) {
        struct timespec now;
        struct timespec diff;
        Radio::service();
        if (svcReq.enable) {
            if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
                perror("clock_gettime");
            timespec_diff(&svcReq.lastReqAt, &now, &diff);
            if (diff.tv_nsec > svcReq.nsec_next) {
                svcReq.nsec_next = (rand() % rand_max_ns) + rand_min_ns;
                //printf("%lu.%09lu     %ld\r\n", now.tv_sec, now.tv_nsec, svcReq.nsec_next);
                get_cf_test();
                svcReq.lastReqAt.tv_sec = now.tv_sec;
                svcReq.lastReqAt.tv_nsec = now.tv_nsec;
            }

        }
        usleep(serviceRate);
    } // ..for (;;)

    return ret;
}

