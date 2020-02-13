clearvars

value_seq=[
    0 0 0 0 ... % ADV_IND
    0 0 ... % RFU
    0 ... % txadd
    0 ... % rxadd
    0 0 1 0 0 1 ... % length 9
    0 0 ... % RFU again
    0 0 1 1 1 1 0 0 0 0 1 0 1 1 0 1 1 0 1 1 0 1 1 1 1 0 0 0 0 1 0 0 1 1 1 0 1 1 1 1 0 0 0 0 1 1 0 1 ... % dec2bin(hex2dec('3c2db784EF0D'),48)
    0 0 0 0 0 0 1 0 ... % AdvData length 2
    0 0 0 0 0 0 0 1 ... % AdvData GAP code 1
    0 0 0 0 0 1 0 1 ... % AdvData data 5
    ];

%%

% (d6 be 89 8e) Access Address for Advertising 
% (40 24) Header 
% (05 a2 17 6e 3d 71) Mac address 
% (02 01 1a 1a ff 4c 00 02 15 e2 c5 6d b5 df fb 48 d2 b0 60 d0 f5 a7 10 96 e0 00 00 00 00 c5) Bluetooth Advertisement
% 
% The checksum of the above packet is 0x52AB8D

value_seq=['11010110101111101000100110001110' ...
    '0100000000100100' ...
    '000001011010001000010111011011100011110101110001' ...
    '000000100000000100011010000110101111111101001100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000'
    ];

value_seq=dec2bin(hex2dec('d6be898e402405a2176e3d7102011a1aff4c000215e2c56db5dffb48d2b060d0f5a71096e000000000c5'),8*42);

value = zeros(1,numel(value_seq));
for ii=1:numel(value_seq);
    if(value_seq(ii)=='1')
        value(ii)=1;
    elseif(value_seq(ii)=='0')
        value(ii)=0;
    else
        value(ii)=-1; % oops
    end
end

value_seq=value;

%% forward

lfsr = [1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 ]; % Initialize the LFSR with 0x555555 per BLE

ble_input=value_seq;

lfsr_next=zeros(1,numel(lfsr));
for ii = 1:numel(ble_input)
    
    common=xor(lfsr(24),ble_input(ii));
    
    lfsr_next(1) = common;                        % position 0
    lfsr_next(2) = xor(common, lfsr(1));
    lfsr_next(3) = lfsr(2);
    lfsr_next(4) = xor(common, lfsr(3));
    lfsr_next(5) = xor(common, lfsr(4));
    lfsr_next(6) = lfsr(5);
    lfsr_next(7) = xor(common, lfsr(6));
    lfsr_next(8) = lfsr(7);
    lfsr_next(9) = lfsr(8);
    lfsr_next(10)= xor(common, lfsr(9));
    lfsr_next(11)= xor(common, lfsr(10));
    lfsr_next(12) = lfsr(11);
    lfsr_next(13) = lfsr(12);
    lfsr_next(14) = lfsr(13);
    lfsr_next(15) = lfsr(14);
    lfsr_next(16) = lfsr(15);
    lfsr_next(17) = lfsr(16);
    lfsr_next(18) = lfsr(17);
    lfsr_next(19) = lfsr(18);
    lfsr_next(20) = lfsr(19);
    lfsr_next(21) = lfsr(20);
    lfsr_next(22) = lfsr(21);
    lfsr_next(23) = lfsr(22);
    lfsr_next(24) = lfsr(23);                       % position 23
    
    lfsr=lfsr_next;
end

lfsr_dec=0;
for jj=1:numel(lfsr)
    lfsr_dec=lfsr_dec+lfsr(jj)*2^(jj-1);
end
dec2hex(lfsr_dec)

lfsr_dec=0;
for jj=1:numel(lfsr)
    lfsr_dec=lfsr_dec+lfsr(end-jj+1)*2^(jj-1);
end
dec2hex(lfsr_dec)
% [crc,crc_dec]=LFSR_BLE_CRC(value_seq); dec2hex(crc_dec)




%% backward

value_seq = value_seq(numel(value_seq):-1:1);
% [crc,crc_dec]=LFSR_BLE_CRC(value_seq); dec2hex(crc_dec)

lfsr = [1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0]; % Initialize the LFSR with 0x555555 per BLE

ble_input=value_seq;

lfsr_next=zeros(1,numel(lfsr));
for ii = 1:numel(ble_input)
    
    common=xor(lfsr(24),ble_input(ii));
    
    lfsr_next(1) = common;                        % position 0
    lfsr_next(2) = xor(common, lfsr(1));
    lfsr_next(3) = lfsr(2);
    lfsr_next(4) = xor(common, lfsr(3));
    lfsr_next(5) = xor(common, lfsr(4));
    lfsr_next(6) = lfsr(5);
    lfsr_next(7) = xor(common, lfsr(6));
    lfsr_next(8) = lfsr(7);
    lfsr_next(9) = lfsr(8);
    lfsr_next(10)= xor(common, lfsr(9));
    lfsr_next(11)= xor(common, lfsr(10));
    lfsr_next(12) = lfsr(11);
    lfsr_next(13) = lfsr(12);
    lfsr_next(14) = lfsr(13);
    lfsr_next(15) = lfsr(14);
    lfsr_next(16) = lfsr(15);
    lfsr_next(17) = lfsr(16);
    lfsr_next(18) = lfsr(17);
    lfsr_next(19) = lfsr(18);
    lfsr_next(20) = lfsr(19);
    lfsr_next(21) = lfsr(20);
    lfsr_next(22) = lfsr(21);
    lfsr_next(23) = lfsr(22);
    lfsr_next(24) = lfsr(23);                       % position 23
    
    lfsr=lfsr_next;
end

lfsr_dec=0;
for jj=1:numel(lfsr)
    lfsr_dec=lfsr_dec+lfsr(jj)*2^(jj-1);
end
dec2hex(lfsr_dec)

lfsr_dec=0;
for jj=1:numel(lfsr)
    lfsr_dec=lfsr_dec+lfsr(end-jj+1)*2^(jj-1);
end
dec2hex(lfsr_dec)