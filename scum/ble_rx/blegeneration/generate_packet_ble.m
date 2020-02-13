function [I,Q,packet01] = generate_packet_ble(Fs,num_payload_bytes,num_preambles)

% Chip period
T = 1/1e6;
dT = 1/Fs;

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
TxAdd=1; % 0 means legit address, 1 means random address
RxAdd=0; % not sure what these are for
length_PDU=[1 0 1 0 0 1 0 0]; % 8 bits in BLE 5, assume max payload length 37 bytes. fliplr(dec2bin(37,8))

pdu_header= [PDU_type RFU TxAdd RxAdd length_PDU];


%% payload

% Since we used 0010 PDU type, payload is divided into two parts:
% 6 bits: advertiser address
% 0-31 bytes: optional advertiser data

AdvA = [1 0 0 1 1 0 0 1 0 1 0 0 1 0 0 1 1 0 0 0 1 1 0 1 1 1 0 1 0 1 1 1 1 1 1 0 1 0 1 1 0 0 0 0 1 0 0 1]; 
% fliplr(dec2bin(hex2dec('90d7ebb19299'),48)) -- there's a fliplr so LSB is first
% advertiser MAC address, 6 bits. here set to 0x90d7ebb19299, random copied from http://processors.wiki.ti.com/index.php/BLE_sniffer_guide


%% data payload, first part: length, GAP value, data. 3 bytes/octets used here; 1 for data

payload1=[
    0 1 0 0 0 0 0 0 ... % AdvData length 2, LSB first
    1 0 0 0 0 0 0 0 ... % AdvData GAP code 0x01 ("flags"), LSB first, from https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile
    1 0 1 0 0 0 0 0 ... % AdvData data 5, LSB first. for why 5, see Core Specification Supplement part A section 1.3 at https://www.bluetooth.com/specifications/bluetooth-core-specification
];


%% data payload, second part: length, GAP value, data. 28 bytes/octets used here; 26 for data.

payload2_header=[
    1 0 1 0 0 1 0 0 ... % AdvData length 28, LSB first
    1 0 0 1 0 0 0 0 ... % AdvData GAP code 0x09 ("complete local name"), LSB first
   % 26 bytes of ASCII data to be appended next   
];

% dec2bin(int8('A'),8) dec2bin(int8('B'),8) etc. up to 26-byte limit: A to Z
value_seq = fliplr(dec2bin(int8('A'):int8('A')+25,8));

payload2_data = zeros(1,numel(value_seq));
for ii=1:numel(value_seq);
    if(value_seq(ii)=='1')
        payload2_data(ii)=1;
    elseif(value_seq(ii)=='0')
        payload2_data(ii)=0;
    else
        payload2_data(ii)=-1; % oops
    end
end

pdu=[pdu_header AdvA payload1 payload2_header payload2_data];

% then whiten payload and CRC, pg 2601 
% then loop every 100ms

crc=LFSR_BLE_CRC(pdu); % 3 bytes

adv_channel=37;
pdu_crc_whitened=LFSR_BLE_WHITEN([pdu crc],adv_channel);

packet01=[bpreamble baccess_address pdu_crc_whitened]; % LSB-first

% Convert packet to +/- 1
packet = 2 * packet01 - 1;

%% Begin OQPSK-HSS Modulation

% Separate data into I and Q channels
I_data =  packet(1:2:end);
Q_data = -packet(2:2:end);      %% Need to invert for use with R&S

% Packet length, in seconds
sim_length = (length(I_data)+length(Q_data))*T;

% Setup time vectors
% Q is delayed by T 
tI = 0:dT:sim_length;
tI = tI(1:end-1);
tQ = [zeros(1, int32(T/dT)) 0:dT:(sim_length-T)];
tQ = tQ(1:end-1);

% Apply half sin symbol shaping
I = I_data(floor(tI/(2*T))+1) .* sin(mod(pi*tI/(2*T),pi));
Q = Q_data(floor(tQ/(2*T))+1) .* sin(mod(pi*tQ/(2*T),pi));

end

