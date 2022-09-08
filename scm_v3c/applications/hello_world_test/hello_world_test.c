#include <string.h>
#include <stdio.h>
#include <stdint.h>

//=========================== defines =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== main ============================================

void rawchips_32_isr(void) {}
void rawchips_startval_isr(void) {}
void rftimer_isr(void) {}
void optical_32_isr(void) {}
void optical_sfd_isr(void) {}
void radio_isr(void) {}
void uart_rx_isr(void) {}
void adc_isr(void) {}
void ext_gpio3_activehigh_debounced_isr(void) {}
void ext_gpio8_activehigh_isr(void) {}
void ext_gpio9_activelow_isr(void) {}
void ext_gpio10_activelow_isr(void) {}

int main(void) {
    uint32_t i;

    printf("Initializing...\n");

    while(1){
        printf("Hello World!\n");

        for (i = 0; i < 1000000; i++);
    }
}
