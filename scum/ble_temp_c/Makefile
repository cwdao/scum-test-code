CC=gcc
CFLAGS=-ggdb -Wall -std=c99

packet: generate_packet_ble_temp.c
	$(CC) $(CFLAGS) -o generate_packet_ble_temp generate_packet_ble_temp.c && ./generate_packet_ble_temp

test: generate_packet_test.c
	$(CC) $(CFLAGS) -o generate_packet_test generate_packet_test.c && ./generate_packet_test
