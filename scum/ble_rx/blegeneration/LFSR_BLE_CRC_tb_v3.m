clearvars

%% j2abro example -- OK BUT CRC BACKWARDS?
% http://j2abro.blogspot.com/2014/06/understanding-bluetooth-advertising.html
% 
% payload_hex='02 01 06 11 06 ba 56 89 a6 fa bf a2 bd 01 46 7d 6e ca 36 ab ad 05 16 0a 18 07 04 ';
% payload_str=[];
% for ii=1:numel(payload_hex)/3
%     position=1+(ii-1)*3;
%     payload_str=[payload_str fliplr(dec2bin(hex2dec(payload_hex(position:position+1)),8))];
% end
% 
% 
% payload_bin = zeros(1,numel(payload_str));
% for ii=1:numel(payload_str);
%     if(payload_str(ii)=='1')
%         payload_bin(ii)=1;
%     elseif(payload_str(ii)=='0')
%         payload_bin(ii)=0;
%     else
%         payload_bin(ii)=-1; % oops
%     end
% end
% 
% 
% value_seq = [
%     0 0 0 0 ... % ADV_IND
%     0 0 ... % RFU
%     1 ... % txadd
%     0 ... % rxadd
%     1 0 0 0 0 1 0 0 ... % length 6+3+18+6, LSB first
%     1 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 0 1 1 0 0 1 1 1 1 0 1 1 0 1 0 0 1 1 0 1 1 1 0 1 1 0 1 0 1 1 1 1 ... % fliplr(dec2bin(hex2dec('f5bb2de612eb'),48))
%     payload_bin
%     ];% CRC should be 69 6e 34  
%     
    

%% TI stuff w/ bitwise reversal -- OK
% http://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/538/4353.btproblem2.png

% value_seq=[
%     0 0 0 0 ... % ADV_IND
%     0 0 ... % RFU
%     0 ... % txadd
%     0 ... % rxadd
%     1 0 0 1 0 0 0 0 ... % length 9
%     1 0 1 1 0 0 0 0 1 1 1 1 0 1 1 1 0 0 1 0 0 0 0 1 1 1 1 0 1 1 0 1 1 0 1 1 0 1 0 0 0 0 1 1 1 1 0 0 ...% fliplr(dec2bin(hex2dec('3c2db784EF0D'),48))
%     0 1 0 0 0 0 0 0 ... % AdvData length 2, LSB first
%     1 0 0 0 0 0 0 0 ... % AdvData GAP code 1, LSB first
%     1 0 1 0 0 0 0 0 ... % AdvData data 5, LSB first
%     ]; % CRC should be 0xA4E2C2

%% TI stuff w/ bitwise reversal -- OK
% http://processors.wiki.ti.com/index.php/BLE_sniffer_guide

payload_hex='02 01 05 07 02 03 18 02 18 04 18 ';
payload_str=[];
for ii=1:numel(payload_hex)/3
    position=1+(ii-1)*3;
    payload_str=[payload_str fliplr(dec2bin(hex2dec(payload_hex(position:position+1)),8))];
end


payload_bin = zeros(1,numel(payload_str));
for ii=1:numel(payload_str);
    if(payload_str(ii)=='1')
        payload_bin(ii)=1;
    elseif(payload_str(ii)=='0')
        payload_bin(ii)=0;
    else
        payload_bin(ii)=-1; % oops
    end
end

%     0 0 0 0 1 0 0 1 1 1 1 0 1 0 1 1 1 1 0 1 0 1 1 1 1 0 0 0 1 1 0 1 0 1 0 0 1 0 0 1 1 0 0 1 1 0 0 1 ... % fliplr(dec2bin(hex2dec('9992b1ebd790'),48))
%     1 0 0 1 1 0 0 1 0 1 0 0 1 0 0 1 1 0 0 0 1 1 0 1 1 1 0 1 0 1 1 1 1 1 1 0 1 0 1 1 0 0 0 0 1 0 0 1 ... % fliplr(dec2bin(hex2dec('90d7ebb19299'),48)) -- there's a fliplr so LSB is first

value_seq=[    
    0 0 0 0 ... % ADV_IND
    0 0 ... % RFU
    0 ... % txadd
    0 ... % rxadd
    1 0 0 0 1 0 ... % length 17, LSB first
    0 0 ... % RFU again
    1 0 0 1 1 0 0 1 0 1 0 0 1 0 0 1 1 0 0 0 1 1 0 1 1 1 0 1 0 1 1 1 1 1 1 0 1 0 1 1 0 0 0 0 1 0 0 1 ... % fliplr(dec2bin(hex2dec('90d7ebb19299'),48)) -- there's a fliplr so LSB is first
    payload_bin
    ]; % CRC should be 0xEF5DA8

%% found online somewhere; stackexchange post? -- OK BUT CRC BACKWARDS

% (d6 be 89 8e) Access Address for Advertising 
% (40 24) Header 
% (05 a2 17 6e 3d 71) Mac address 
% (02 01 1a 1a ff 4c 00 02 15 e2 c5 6d b5 df fb 48 d2 b0 60 d0 f5 a7 10 96 e0 00 00 00 00 c5) Bluetooth Advertisement
% 
% The checksum of the above packet is 0x52AB8D

% payload_hex='05 a2 17 6e 3d 71 02 01 1a 1a ff 4c 00 02 15 e2 c5 6d b5 df fb 48 d2 b0 60 d0 f5 a7 10 96 e0 00 00 00 00 c5 ';
% payload_str=[];
% for ii=1:numel(payload_hex)/3
%     position=1+(ii-1)*3;
%     payload_str=[payload_str fliplr(dec2bin(hex2dec(payload_hex(position:position+1)),8))];
% end
% 
% 
% payload_bin = zeros(1,numel(payload_str));
% for ii=1:numel(payload_str);
%     if(payload_str(ii)=='1')
%         payload_bin(ii)=1;
%     elseif(payload_str(ii)=='0')
%         payload_bin(ii)=0;
%     else
%         payload_bin(ii)=-1; % oops
%     end
% end
% 
% %     101000000100010111101000011101101011110010001110 ... % fliplr(dec2bin(hex2dec('713d6e17a205'),48)) -- there's a fliplr so LSB is first
% 
% value_seq=[    
%     0 0 0 0 ... % ADV_IND
%     0 0 ... % RFU
%     1 ... % txadd
%     0 ... % rxadd
%     0 0 1 0 0 1 ... % length 36, LSB first
%     0 0 ... % RFU again
%     payload_bin % incl mac address here
%     ]; % CRC should be 0x52AB8D

% value_seq=['11010110101111101000100110001110' ...
%     '0100000000100100' ...
%     '000001011010001000010111011011100011110101110001' ...
%     '000000100000000100011010000110101111111101001100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000'
%     ];

% value_seq=dec2bin(hex2dec('d6be898e402405a2176e3d7102011a1aff4c000215e2c56db5dffb48d2b060d0f5a71096e000000000c5'),8*42); % crc should be 0x52AB8D

% value = zeros(1,numel(value_seq));
% for ii=1:numel(value_seq);
%     if(value_seq(ii)=='1')
%         value(ii)=1;
%     elseif(value_seq(ii)=='0')
%         value(ii)=0;
%     else
%         value(ii)=-1; % oops
%     end
% end
% 
% value_seq=value;
% 
% bits=8;
% for jj=1:numel(value_seq)/bits;
%     for kk=1:bits
%         value_temp(bits*(jj-1)+kk)=value(bits*(jj-1)+(bits+1-kk));
%     end
% end
% 
% value_seq=value_temp;


%% as yet unchecked
% sample TI data ADV_IND 0 0 0 9 0x3c2db78ef0d 02 01 05 -- crc 0xa4e2c2


%% forward lfsr 1

% lfsr = [1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 ]; % Initialize the LFSR with 0x555555 per BLE
% 
% ble_input=value_seq;
% 
% lfsr_next=zeros(1,numel(lfsr));
% for ii = 1:numel(ble_input)
%     
%     common=xor(lfsr(24),ble_input(ii));
%     
%     lfsr_next(1) = common;                        % position 0
%     lfsr_next(2) = xor(common, lfsr(1));
%     lfsr_next(3) = lfsr(2);
%     lfsr_next(4) = xor(common, lfsr(3));
%     lfsr_next(5) = xor(common, lfsr(4));
%     lfsr_next(6) = lfsr(5);
%     lfsr_next(7) = xor(common, lfsr(6));
%     lfsr_next(8) = lfsr(7);
%     lfsr_next(9) = lfsr(8);
%     lfsr_next(10)= xor(common, lfsr(9));
%     lfsr_next(11)= xor(common, lfsr(10));
%     lfsr_next(12) = lfsr(11);
%     lfsr_next(13) = lfsr(12);
%     lfsr_next(14) = lfsr(13);
%     lfsr_next(15) = lfsr(14);
%     lfsr_next(16) = lfsr(15);
%     lfsr_next(17) = lfsr(16);
%     lfsr_next(18) = lfsr(17);
%     lfsr_next(19) = lfsr(18);
%     lfsr_next(20) = lfsr(19);
%     lfsr_next(21) = lfsr(20);
%     lfsr_next(22) = lfsr(21);
%     lfsr_next(23) = lfsr(22);
%     lfsr_next(24) = lfsr(23);                       % position 23
%     
%     lfsr=lfsr_next;
% end

% lfsr_dec=0;
% for jj=1:numel(lfsr)
%     lfsr_dec=lfsr_dec+lfsr(jj)*2^(jj-1);
% end
% dec2hex(lfsr_dec)
% 
% lfsr_dec=0;
% for jj=1:numel(lfsr)
%     lfsr_dec=lfsr_dec+lfsr(end-jj+1)*2^(jj-1);
% end
% dec2hex(lfsr_dec)
% % [crc,crc_dec]=LFSR_BLE_CRC(value_seq); dec2hex(crc_dec)

crc=LFSR_BLE_CRC(value_seq);

% lfsr=crc;

for ii=1:3
    lfsr_dec(ii)=0;
    for jj=1:8
%         jj+8*(ii-1)
        lfsr_dec(ii)=lfsr_dec(ii)+crc(jj+8*(ii-1))*2^(8-jj);
    end
end
[dec2hex(lfsr_dec(1),2) ' ' dec2hex(lfsr_dec(2),2) ' ' dec2hex(lfsr_dec(3),2)]
