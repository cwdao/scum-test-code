function [packet01] = generate_packet_ble_temp(tempC)


%% Temperature conversion
temp = round(tempC, 2) + 273.15; % round tempC to nearest hundredth and convert to Kelvin
temp100 = temp * 100; % multiply temp by 100 to transmit integer
temp_payload = fliplr(dec2bin(uint16(temp100), 16)) - '0'; % 2 bytes for temp, LSB first

%% Begin Packet Formation

bpreamble = [0 1 0 1 0 1 0 1]; % since the access address ends with a 0 (AA LSB = 0)

% http://j2abro.blogspot.com/2014/06/understanding-bluetooth-advertising.html
% standard AA for broadcast packets is 0x8E89BED6

baccess_address = [0 1 1 0 1 0 1 1 0 1 1 1 1 1 0 1 1 0 0 1 0 0 0 1 0 1 1 1 0 0 0 1];
% fliplr(dec2bin(hex2dec('8E89BED6'),32)) -- there's a fliplr so LSB is first

% transmit on BLE channels 37, 38, 39: 2.402GHz, 2.426GHz, and 2.480GHz,
% respectively
% http://www.argenox.com/a-ble-advertising-primer/

%% PDU header

PDU_type = [0 1 0 0]; % 4 bytes. 0010b is "ADV_NONCONN_IND"; tx only mode
% must be flipped wrt table below so LSB comes first

%  PDU Types
%  b3b2b1b0 Packet Name
%  0000 ADV_IND? connectable undirected advertising event
%  0001 ADV_DIRECT_IND?connectable directed advertising event
%  0010 ADV_NONCONN_IND?non-connectable undirected advertising event
%  0011 SCAN_REQ?scan request
%  0100 SCAN_RSP: scan response
%  0101 CONNECT_REQ?connection request
%  0110 ADV_SCAN_IND?scannable undirected advertising event
%  0111-1111 Reserved

RFU=[0 0]; % reserved for future use
TxAdd=0; % 0 means legit address, 1 means random address
RxAdd=0; % not sure what these are for
% length_PDU=[1 1 1 1 0 0 0 0]; % 8 bits in BLE 5. 6 for address, 3 for headers, 6 for name payload. fliplr(dec2bin(15,8))
% length_PDU=[1 0 1 0 0 1 0 0]; % 8 bits in BLE 5. max typical length 37. fliplr(dec2bin(37,8))
% length_PDU=[0 0 0 0 1 0 0 0]; % 8 bits in BLE 5. fliplr(dec2bin(16,8))
length_PDU=[0 1 1 0 0 0 0 0]; % 8 bits in BLE 5. fliplr(dec2bin(6,8)) (only AdvA)

pdu_header= [PDU_type RFU TxAdd RxAdd length_PDU];

%% payload

% Since we used 0010 PDU type, payload is divided into two parts:
% 6 bits: advertiser address
% 0-31 bytes: optional advertiser data

% AdvA = [1 0 0 1 1 0 0 1 0 1 0 0 1 0 0 1 1 0 0 0 1 1 0 1 1 1 0 1 0 1 1 1 1 1 1 0 1 0 1 1 0 0 0 0 1 0 0 1]; 
% fliplr(dec2bin(hex2dec('90d7ebb19299'),48)) -- there's a fliplr so LSB is first
% advertiser MAC address, 6 bits. here set to 0x90d7ebb19299, random copied from http://processors.wiki.ti.com/index.php/BLE_sniffer_guide

% AdvA = [0 1 1 0 0 0 1 1 0 0 0 0 0 0 0 1 0 1 0 0 1 1 0 0 0 1 0 0 1 1 1 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0];
% AdvA = 0002723280C6, LSB first

AdvA = [temp_payload 0 1 0 0 1 1 0 0 0 1 0 0 1 1 1 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0];
% AdvA = 00027232 + temp payload, LSB first

pdu=[pdu_header AdvA]; % 2 bytes, 6 bytes

% then whiten payload and CRC, pg 2601 
crc=fliplr(lfsr_ble_crc(pdu)); % 3 bytes

adv_channel=37;
pdu_crc_whitened=lfsr_ble_whiten([pdu crc],adv_channel);

packet01=[bpreamble baccess_address pdu_crc_whitened]; % LSB-first

fprintf("%02x", bin2dec(num2str(packet01(1:48))));
fprintf("%02x", bin2dec(num2str(packet01(49:96))));
fprintf("%02x", bin2dec(num2str(packet01(97:128))));

end
