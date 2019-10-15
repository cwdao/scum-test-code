#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

const unsigned char PRE_PREAMBLE = 0b00000111; // padded with two extra zeroes at the beginning;
const unsigned char BPREAMBLE = 0b01010101;
const unsigned char BACCESS_ADDRESS1 = 0b01101011; // need to split BACCESS_ADDRESS into bytes to avoid big-/little-endianness issue
const unsigned char BACCESS_ADDRESS2 = 0b01111101;
const unsigned char BACCESS_ADDRESS3 = 0b10010001;
const unsigned char BACCESS_ADDRESS4 = 0b01110001;
const unsigned char SUFFIX = 0b00011100; // padded with two extra zeroes at the end

const unsigned char PDU_HEADER1 = 0b01000000;
const unsigned char PDU_HEADER2 = 0b00001000;
const unsigned char PAYLOAD11 = 0b01000000;
const unsigned char PAYLOAD12 = 0b10000000;
const unsigned char PAYLOAD13 = 0b10100000;
const unsigned char PAYLOAD2_HEADER1 = 0b01100000;
const unsigned char PAYLOAD2_HEADER2 = 0b00010000;

const unsigned int PACKET_LENGTH = 28; // packet is 25 bytes long
const unsigned int PDU_LENGTH = 18; // pdu is 15 bytes long
const unsigned int ADVA_LENGTH = 6; // adv address is 6 bytes long
const unsigned int DATA_LENGTH = 5; // data (i.e. temperature) is 5 bytes long
const unsigned int CRC_LENGTH = 3; // crc is 3 bytes long

unsigned char flip(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

unsigned char hex_char_to_binary(unsigned char ch) {
	ch = tolower(ch);
	if (isdigit(ch)) {
		return (ch - '0');
	} else if (ch >= 'a' && ch <= 'f') {
		return (ch - 'a') + 10;
	}
	return 0;
}

void hex_str_to_binary(char *str, unsigned int length, unsigned char *res) {
	for (unsigned int i = 0; i < length; i += 2) {
		unsigned char high = hex_char_to_binary(str[i]);
		unsigned char low = hex_char_to_binary(str[i + 1]);
		res[i / 2] = (high << 4) | low;
	}
}

void gen_packet(unsigned char *packet, unsigned char *AdvA, unsigned char channel, double tempC) {
	// temperature in Kelvin rounded to nearest hundredth
	// temperature multiplied by 100 to store as integer
	unsigned int temp = (unsigned int) (tempC * 100 + 27315);

	// pre_preamble
	*packet = PRE_PREAMBLE;
	packet++;

	// bpreamble
	*packet = BPREAMBLE;
	packet++;

	// baccess_address
	*packet = BACCESS_ADDRESS1;
	packet++;
	*packet = BACCESS_ADDRESS2;
	packet++;
	*packet = BACCESS_ADDRESS3;
	packet++;
	*packet = BACCESS_ADDRESS4;
	packet++;

	// pdu_crc_whitened
	unsigned char pdu_crc[PDU_LENGTH + CRC_LENGTH];
	unsigned char *pdu_pointer = pdu_crc;

	*pdu_pointer = PDU_HEADER1;
	pdu_pointer++;
	*pdu_pointer = PDU_HEADER2;
	pdu_pointer++;

	for (int byte = ADVA_LENGTH - 1; byte >= 0; byte --) {
		*pdu_pointer = flip(AdvA[byte]);
		*pdu_pointer++;
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

	*pdu_pointer = flip((unsigned char) (temp & 0xFF));
	pdu_pointer++;
	*pdu_pointer = flip((unsigned char) ((temp & 0xFF00) >> 8));
	pdu_pointer++;
	*pdu_pointer = flip((unsigned char) ((temp & 0xFF0000) >> 16));
	pdu_pointer++;
	*pdu_pointer = flip((unsigned char) ((temp & 0xFF000000) >> 24));
	pdu_pointer++;
	*pdu_pointer = 0x00;
	pdu_pointer++;

	// crc
	unsigned char *crc_pointer = pdu_pointer;
	unsigned char crc3 = 0b10101010;
	unsigned char crc2 = 0b10101010;
	unsigned char crc1 = 0b10101010;

	for (unsigned char *byte_addr = pdu_crc, i = 0; i < PDU_LENGTH; byte_addr++, i++) {
		for (int bit = 7; bit >= 0; bit--) {
			unsigned char common = (crc1 & 1) ^ ((*byte_addr & (1 << bit)) >> bit);
			crc1 = (crc1 >> 1) | ((crc2 & 1) << 7);
			crc2 = ((crc2 >> 1) | ((crc3 & 1) << 7)) ^ (common << 6) ^ (common << 5);
			crc3 = ((crc3 >> 1) | (common << 7)) ^ (common << 6) ^ (common << 4) ^ (common << 3) ^ (common << 1);
		}
	}

	crc1 = flip(crc1);
	crc2 = flip(crc2);
	crc3 = flip(crc3);

	*(crc_pointer) = crc1;
	*(crc_pointer + 1) = crc2;
	*(crc_pointer + 2) = crc3;

	// whiten
	unsigned char lfsr = channel | 0x40; // [1 37]
	for (unsigned char *byte_addr = pdu_crc, i = 0; i < PDU_LENGTH + CRC_LENGTH; byte_addr++, i++) {
		for (int bit = 7; bit >= 0; bit--) {
			*byte_addr = (*byte_addr & ~(1 << bit)) | ((*byte_addr & (1 << bit)) ^ ((lfsr & 1) << bit));
			lfsr = ((lfsr >> 1) | ((lfsr & 1) << 6)) ^ ((lfsr & 1) << 2);
		}
	}

	for (unsigned char *byte_addr = pdu_crc, i = 0; i < PDU_LENGTH + CRC_LENGTH; byte_addr++, i++) {
		*packet = *byte_addr;
		packet++;
	}

	// suffix
	*packet = SUFFIX;
}

int main() {
	unsigned char packet[PACKET_LENGTH];
	unsigned char AdvA[ADVA_LENGTH];
	hex_str_to_binary("0002723280C6", ADVA_LENGTH * 2, AdvA);
	gen_packet(packet, AdvA, 37, 0);

	for (unsigned char *byte_addr = packet, i = 0; i < PACKET_LENGTH; byte_addr++, i++) {
		printf("%02x", *byte_addr);
	}
	printf("\n");
	return 0;
}
