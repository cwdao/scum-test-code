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

    // Output a constant 1.8 V over GPIO 2 and a square wave over GPIO 3.
    GPO_control(6, 6, 6, 6);
    GPI_enables(0x0000);
    GPO_enables(0xFFFF);

    // GPIO 2 is always at 1.8 V.
    GPIO_REG__OUTPUT |= 0x0004;
    // GPIO 3 is initially at 0 V.
    GPIO_REG__OUTPUT &= ~0x0008;

    uint32_t num_samples = 0;
    bool gpio_high = false;
    while (true) {
        // Trigger an ADC read.
        // printf("Triggering ADC.\n");
        adc_trigger();
        while (!g_adc_output.valid) {
        }
        if (!g_adc_output.valid) {
            printf("ADC output should be valid.\n");
        }
        printf("%u\n", g_adc_output.data);

        // Wait a bit.
        for (uint32_t i = 0; i < 30000; i++) {}

        // Toggle GPIO 3 every minute.
        ++num_samples;
        if (num_samples == 10 * 60) {
            gpio_high = !gpio_high;
            printf("Toggling GPIO 3 to %d.\n", gpio_high);
            if (gpio_high) {
                GPIO_REG__OUTPUT |= 0x0008;
            } else {
                GPIO_REG__OUTPUT &= ~0x0008;
            }
            num_samples = 0;
        }
    }
}
