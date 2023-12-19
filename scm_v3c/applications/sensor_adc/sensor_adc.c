#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adc.h"
#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"

// Number of pulses per PWM ramp.
#define NUM_PULSES_PER_RAMP 3500

// Number of cycles per pulse.
#define NUM_CYCLES_PER_PULSE 800

// Fraction increment per pulse.
#define FRACTION_INCREMENT_PER_PULSE 8

// If true, take measurements with the ADC during the ramp.
#define ADC_ENABLED false

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

#if ADC_ENABLED
// ADC data for a single PWM ramp.
static uint16_t g_adc_data_for_single_ramp[NUM_PULSES_PER_RAMP][2];

static inline void read_adc_output(void) {
    adc_trigger();
    while (!g_adc_output.valid) {
    }
}
#endif  // ADC_ENABLED

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

    // GPIO 0 is the PWM signal.
    GPO_control(6, 6, 6, 6);
    GPI_enables(0x0000);
    GPO_enables(0xFFFF);

    analog_scan_chain_write();
    analog_scan_chain_load();

    // Lower the pulse.
    GPIO_REG__OUTPUT &= ~0x0001;

    while (true) {
        printf("Starting a new PWM ramp.\n");
        // Generate a PWM ramp.
        for (uint32_t i = 0; i < NUM_PULSES_PER_RAMP; ++i) {
#if ADC_ENABLED
            // Read the ADC.
            read_adc_output();
            g_adc_data_for_single_ramp[i][0] = g_adc_output.data;
#endif  // ADC_ENABLED

            // Start the pulse.
            GPIO_REG__OUTPUT |= 0x0001;

            // Wait for the pulse width.
            for (uint32_t j = 0; j < i / FRACTION_INCREMENT_PER_PULSE; ++j) {
            }

#if ADC_ENABLED
            // Read the ADC.
            read_adc_output();
            g_adc_data_for_single_ramp[i][1] = g_adc_output.data;
#endif  // ADC_ENABLED

            // Lower the pulse.
            GPIO_REG__OUTPUT &= ~0x0001;

            // Wait until the next pulse.
            for (uint32_t j = 0;
                 j < NUM_CYCLES_PER_PULSE - i / FRACTION_INCREMENT_PER_PULSE;
                 ++j) {
            }
        }

#if ADC_ENABLED
        // Print the ADC data for the latest PWM ramp.
        printf("ADC data for PWM ramp:\n");
        for (uint32_t i = 0; i < NUM_PULSES_PER_RAMP; ++i) {
            printf("%d %d\n", g_adc_data_for_single_ramp[i][0],
                   g_adc_data_for_single_ramp[i][1]);
        }
#endif  // ADC_ENABLED

        // Wait a bit.
        for (uint32_t i = 0; i < 10000000; ++i) {
        }
    }
}
