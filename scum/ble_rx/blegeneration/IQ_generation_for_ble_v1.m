close all 
clearvars
% clc

% Set parameters
% T/F_sample needs to be an integer

% The output rateof 33522B is 250MHz
Fs = 250e6;
dT = 1/Fs;

% Packet parameters
num_preambles = 0;
num_payload_bytes = 0;
% Packet time duration  = (payload_bytes + preambles/2 + 2) * 64 * T

%% Generate I/Q data for BLE packet
[I,Q] = generate_packet_ble_scum3_v4(Fs,num_payload_bytes,num_preambles);
% [I,Q] = generate_packet_ble_shortest(Fs,num_payload_bytes,num_preambles);
% [I,Q] = generate_packet_ble_ee194(Fs);
%load blepacket.mat
% set rohde to channel 37 aka 2.402GHz

% %% Open GPIB connection to Agilent 33522B AWG
% %% ------- 
% % Kill open instruments
% % this script closes all open instruments and deletes all instruments
openInst = instrfindall;
if ~isempty(openInst)
    fclose(openInst);
    delete(openInst)
end
clear openInst Instruments

fgen = visa('agilent','GPIB1::10::INSTR');

% Set up buffer
buffer = 10e6;
set (fgen,'OutputBufferSize',(buffer+125));
set (fgen,'TimeOut',20);

fopen(fgen);

fgen_identity = query(fgen, '*IDN?');
if isempty(strfind(fgen_identity,'Agilent Technologies,33522B'))
    error('Not connected to Keysight Function Generator! Check GPIB connection');
end
%% -------

%% Send data to Agilent 33522B for output
AWG_output(fgen,I(10:end),Q(10:end));


% save 'tx_payload_symbols.mat' tx_symbols

%% Cleanup
fclose(fgen);
