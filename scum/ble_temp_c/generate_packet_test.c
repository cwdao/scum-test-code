#include <stdio.h>
#include <stdlib.h>
#include "Memory_Map.h"

const unsigned char PRE_PREAMBLE = 0x07; // padded with two extra zeroes at the beginning;
const unsigned char BPREAMBLE = 0x55;
const unsigned char BACCESS_ADDRESS1 = 0x6B; // need to split BACCESS_ADDRESS into bytes to avoid big-/little-endianness issue
const unsigned char BACCESS_ADDRESS2 = 0x7D;
const unsigned char BACCESS_ADDRESS3 = 0x91;
const unsigned char BACCESS_ADDRESS4 = 0x71;
const unsigned char SUFFIX = 0x1C; // padded with two extra zeroes at the end

const unsigned char PDU_HEADER1 = 0x40;
const unsigned char PDU_HEADER2 = 0x08;
const unsigned char PAYLOAD11 = 0x40;
const unsigned char PAYLOAD12 = 0x80;
const unsigned char PAYLOAD13 = 0xA0;
const unsigned char PAYLOAD2_HEADER1 = 0x60;
const unsigned char PAYLOAD2_HEADER2 = 0x10;

const unsigned int PACKET_LENGTH = 28; // packet is 25 bytes long
const unsigned int PDU_LENGTH = 18; // pdu is 15 bytes long
const unsigned int ADVA_LENGTH = 6; // adv address is 6 bytes long
const unsigned int DATA_LENGTH = 5; // data (i.e. temperature) is 5 bytes long
const unsigned int CRC_LENGTH = 3; // crc is 3 bytes long


unsigned char flipChar(unsigned char b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}


void gen_ble_packet(unsigned char *packet, unsigned char *AdvA, unsigned char channel, double tempC) {
	// bullshit declarations that I don't care where they go but the compiler is being a little ass about
	
	int byte1;
	int i;
	int bit2;
	unsigned char common;
	unsigned char crc3 = 0xAA;
	unsigned char crc2 = 0xAA;
	unsigned char crc1 = 0xAA;
	unsigned char lfsr = channel | 0x40; // [1 channel[6]]
	
	unsigned int temp = (unsigned int) (tempC * 100 + 27315);
	unsigned char pdu_crc[PDU_LENGTH + CRC_LENGTH];
	unsigned char *pdu_pointer = pdu_crc;
	unsigned char *crc_pointer = pdu_pointer + ADVA_LENGTH + 11;
	unsigned char *byte_addr = pdu_crc;
	
	*packet = PRE_PREAMBLE;
	packet++;
	
	*packet = BPREAMBLE;
	packet++;
	
	*packet = BACCESS_ADDRESS1;
	packet++;
	*packet = BACCESS_ADDRESS2;
	packet++;
	*packet = BACCESS_ADDRESS3;
	packet++;
	*packet = BACCESS_ADDRESS4;
	packet++;
	
	*pdu_pointer = PDU_HEADER1;
	pdu_pointer++;
	*pdu_pointer = PDU_HEADER2;
	pdu_pointer++;
	
	for (byte1 = ADVA_LENGTH - 1; byte1 >= 0; byte1--) {
		*pdu_pointer = flipChar(AdvA[byte1]);
		pdu_pointer++;
	}
	
	*pdu_pointer = PAYLOAD11;
	pdu_pointer++;
	*pdu_pointer = PAYLOAD12;
	pdu_pointer++;
	*pdu_pointer = PAYLOAD13;
	pdu_pointer++;
	*pdu_pointer = PAYLOAD2_HEADER1;
	pdu_pointer++;
	*pdu_pointer = PAYLOAD2_HEADER2;
	pdu_pointer++;
	
	*pdu_pointer = flipChar((unsigned char) (temp & 0xFF));
	pdu_pointer++;
	*pdu_pointer = flipChar((unsigned char) ((temp & 0xFF00) >> 8));
	pdu_pointer++;
	*pdu_pointer = flipChar((unsigned char) ((temp & 0xFF0000) >> 16));
	pdu_pointer++;
	*pdu_pointer = flipChar((unsigned char) ((temp & 0xFF000000) >> 24));
	pdu_pointer++;
	*pdu_pointer = 0x00;
	pdu_pointer++;
	
	crc_pointer = pdu_pointer;
	byte_addr = pdu_crc;
	
	for (i = 0; i < PDU_LENGTH; byte_addr++, i++) {
		for (bit2 = 7; bit2 >= 0; bit2--) {
			common = (crc1 & 1) ^ ((*byte_addr & (1 << bit2)) >> bit2);
			crc1 = (crc1 >> 1) | ((crc2 & 1) << 7);
			crc2 = ((crc2 >> 1) | ((crc3 & 1) << 7)) ^ (common << 6) ^ (common << 5);
			crc3 = ((crc3 >> 1) | (common << 7)) ^ (common << 6) ^ (common << 4) ^ (common << 3) ^ (common << 1);
		}
	}
	
	crc1 = flipChar(crc1);
	crc2 = flipChar(crc2);
	crc3 = flipChar(crc3);
	
	*(crc_pointer) = crc1;
	*(crc_pointer + 1) = crc2;
	*(crc_pointer + 2) = crc3;
	
	byte_addr = pdu_crc;
	
	for (i = 0; i < PDU_LENGTH + CRC_LENGTH; byte_addr++, i++) {
		for (bit2 = 7; bit2 >= 0; bit2--) {
			*byte_addr = (*byte_addr & ~(1 << bit2)) | ((*byte_addr & (1 << bit2)) ^ ((lfsr & 1) << bit2));
			lfsr = ((lfsr >> 1) | ((lfsr & 1) << 6)) ^ ((lfsr & 1) << 2);
		}
	}

	byte_addr = pdu_crc;
	for (i = 0; i < PDU_LENGTH + CRC_LENGTH; byte_addr++, i++) {
		*packet = *byte_addr;
		packet++;
	}

	// suffix
	*packet = SUFFIX;

}

int test_ble_packet() {
	unsigned char packet[PACKET_LENGTH];
	unsigned char *byte_addr = packet;
	unsigned char AdvA[ADVA_LENGTH];
	int i;
	AdvA[0] = 0x00;
	AdvA[1] = 0x02;
	AdvA[2] = 0x72;
	AdvA[3] = 0x32;
	AdvA[4] = 0x80;
	AdvA[5] = 0xC6;
	gen_ble_packet(packet, AdvA, 37, 0);
	
	for (i = 0; i < PACKET_LENGTH; byte_addr++, i++) {
  	printf("%02x",*byte_addr);
	}
	printf("\n");
	return 0;
}