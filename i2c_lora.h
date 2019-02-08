#ifndef SWIG
#include <stdint.h>
#include <float.h>
#include <linux/i2c-dev.h>
#include "cmds.h"
#endif /* SWIG */

typedef struct {
    uint8_t solicited     : 7;   // 0,1,2,3,4,5,6
    uint8_t res           : 1;   // 7
} i2c_flags_t;

extern int fd;

class Radio {
    public:
        static int i2c_lora_setup(uint8_t sa, unsigned irqPin);
        static void i2c_lora_close(void);
        static float GetAmbient(uint8_t);
        static bool service(void);  // return true if irq was asserted
        static int set_tx_dbm(int8_t);
        static int get_tx_dbm(int8_t*);
        static int SetChannel(unsigned hz);
        static int LoRaModemConfig(unsigned KHz, unsigned sf, unsigned cr);
        static int GetLoRaModemConfig(unsigned* KHz, unsigned* sf, unsigned* cr);
        static int LoRaPacketConfig(unsigned preambleLen, bool fixLen, bool crcOn, bool invIQ);
        static int GetLoRaPacketConfig(unsigned* preambleLen, bool* fixLen, bool* crcOn, bool* invIQ);
        static int Rx(unsigned timeout); // timeout 0 for continuous rx
        static int BeaconCfg(unsigned secs, unsigned preambleSize, uint8_t beaconSize);
		static int Config(const cfg_t* cfg);
		static int Standby(void);
		static int SetPublicNetwork(bool pub);
		static int SetRxMaxPayloadLength(uint8_t paylen);
		static int Send(void);
        static int SetBeaconSkip(uint8_t cnt);
        static int GetBeaconSkip(void);
        static int GetCurrentSlot(void);
        static float GetCF(void);   // return MHz
		static unsigned GetRandom(void);
        static int LoadTxPacket(const uint8_t* txbuf, uint8_t txlen);

		static int SetLoRaSymbolTimeout(uint8_t symbs);
        static int GetFSKSync(uint8_t* length, uint8_t* syncbuf);
        static int GetGFSKModemConfig(unsigned* bps, unsigned* bwKHz, unsigned* fdev_hz);
		static int GFSKModemConfig(unsigned bps, unsigned bwKHz, unsigned fdev_hz);
		static int GFSKPacketConfig(unsigned preambleLen, bool fixLen, bool crcOn);
		static int GetGFSKPacketConfig(unsigned* preambleLen, bool* fixLen, bool* crcOn);
        static int get_opmode(void);
		static int reset(void);

        static uint8_t irq_pin;
        static void (*OnRadioRxDone)(uint16_t slot, uint8_t size, float rssi, float snr, const uint8_t*);
        static void (*background_rssi)(float);

    private:
        static irq_t irq;
        static irq_type_e waitingFor;
        static i2c_flags_t flags;
};
