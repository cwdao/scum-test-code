#include <stdint.h>

// Set aside sections of address space for the packet
uint8_t send_packet[127] __attribute__ ((aligned (4)));
uint8_t recv_packet[130] __attribute__ ((aligned (4)));
