#include <stdint.h>

void gen_ble_packet(uint8_t *packet, uint8_t *AdvA, uint8_t channel, uint16_t LC_freq);
void radio_init_tx_BLE(void);
void gen_test_ble_packet(uint8_t *packet);
void load_tx_arb_fifo(uint8_t *packet);
void transmit_tx_arb_fifo();
void radio_init_rx_ZCC_BLE(void);
void initialize_mote_ble(void);
