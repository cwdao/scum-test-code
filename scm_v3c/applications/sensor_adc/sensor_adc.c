#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adc.h"
#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"

// ADC configuration.
static const adc_config_t g_adc_config = {
    .reset_source = ADC_RESET_SOURCE_FSM,
    .convert_source = ADC_CONVERT_SOURCE_FSM,
    .pga_amplify_source = ADC_PGA_AMPLIFY_SOURCE_FSM,
    .pga_gain = 0,
    .settling_time = 0,
    .bandgap_reference_tuning_code = 1,
    .const_gm_tuning_code = 0xFF,
    .vbat_div_4_enabled = false,
    .ldo_enabled = true,
    .input_mux_select = ADC_INPUT_MUX_SELECT_EXTERNAL_SIGNAL,
    .pga_bypass = true,
};

static inline void read_adc_output(void) {
    // Read 10 times and keep the last read.
    for (uint32_t i = 0; i < 10; ++i) {
        adc_trigger();
        while (!g_adc_output.valid) {
        }
        // Wait a bit.
        for (uint32_t j = 0; j < 100; ++j) {
        }
    }
}

int main(void) {
    initialize_mote();

    // Configure the ADC.
    printf("Configuring the ADC.\n");
    adc_config(&g_adc_config);
    adc_enable_interrupt();

    analog_scan_chain_write();
    analog_scan_chain_load();

    crc_check();
    perform_calibration();

    // GPIO 0 is the strobe, and GPIO 1, 2, and 3 control A, B, and C,
    // respectively.
    GPO_control(6, 6, 6, 6);
    GPI_enables(0x0000);
    GPO_enables(0xFFFF);

    analog_scan_chain_write();
    analog_scan_chain_load();

    // Lower B and C.
    GPIO_REG__OUTPUT &= ~0x000C;

    while (true) {
        // Lower the strobe.
        GPIO_REG__OUTPUT &= ~0x0001;

        // Lower A.
        GPIO_REG__OUTPUT &= ~0x0002;

        // Wait a bit.
        for (uint32_t i = 0; i < 1000; ++i) {
        }

        // Trigger an ADC read.
        printf("Triggering ADC on D0.\n");
        read_adc_output();
        if (!g_adc_output.valid) {
            printf("ADC output should be valid.\n");
        }
        printf("ADC output: %u\n", g_adc_output.data);

        // Raise A.
        GPIO_REG__OUTPUT |= 0x0002;

        // Wait a bit.
        for (uint32_t i = 0; i < 1000; ++i) {
        }

        // Trigger an ADC read.
        printf("Triggering ADC on D1.\n");
        read_adc_output();
        if (!g_adc_output.valid) {
            printf("ADC output should be valid.\n");
        }
        printf("ADC output: %u\n", g_adc_output.data);

        // Raise the strobe.
        GPIO_REG__OUTPUT |= 0x0001;

        // Wait for around 1 second.
        for (uint32_t i = 0; i < 700000; ++i) {
        }
    }
}
