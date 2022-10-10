#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adc.h"
#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"

// Number of ADC samples.
#define NUM_ADC_SAMPLES 60000

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

// ADC samples.
static uint8_t g_adc_samples[NUM_ADC_SAMPLES];

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

    // Warm up the ADC reads.
    for (uint32_t i = 0; i < 5; ++i) {
        adc_trigger();
        while (!g_adc_output.valid)
            ;
    }

    // Output the ADC convert signal over GPIO 2 in bank 9.
    GPO_control(6, 6, 6, 6);
    GPI_enables(0x0000);
    GPO_enables(0xFFFF);

    for (uint32_t i = 0; i < NUM_ADC_SAMPLES; ++i) {
        // Trigger an ADC read.
        adc_trigger();
        GPIO_REG__OUTPUT |= 0x0004;
        while (!g_adc_output.valid) {
        }
        if (!g_adc_output.valid) {
            printf("ADC output should be valid.\n");
        }
        GPIO_REG__OUTPUT &= ~0x0004;

        g_adc_samples[i] = (uint8_t)g_adc_output.data;
    }

    printf("Done with %u ADC samples.\n", NUM_ADC_SAMPLES);
    for (uint32_t i = 0; i < NUM_ADC_SAMPLES; ++i) {
        printf("%u\n", g_adc_samples[i]);
    }
    printf("Done.\n");
}
