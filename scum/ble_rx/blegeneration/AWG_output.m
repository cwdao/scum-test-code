function [] = AWG_output(fgen,I,Q)


% I/Q data, should be column vectors, normalized to 1, single precision
I = single(I');
Q = single(Q');

%% Return to default settings
fprintf(fgen,'*RST');

%Clear volatile memory
fprintf(fgen, 'SOURce1:DATA:VOLatile:CLEar');
fprintf(fgen, 'SOURce2:DATA:VOLatile:CLEar');
fprintf(fgen, 'FORM:BORD SWAP');  %configure the box to correctly accept the binary arb points

%% Set up Channel 1 with I data
% Number of bytes
iBytes=num2str(length(I) * 4); 
% Create header
header= ['SOURce1:DATA:ARBitrary IDATA, #' num2str(length(iBytes)) iBytes]; 
% Convert datapoints to binary before sending
binblockBytes = typecast(I, 'uint8');  
% Combine header and datapoints then send to instrument
fwrite(fgen, [header binblockBytes'], 'uint8'); 
% Make sure no other commands are exectued until arb is done downloading
fprintf(fgen, '*WAI');   

% Set current arb waveform to defined arb 
fprintf(fgen,'SOUR1:FUNCtion:ARB IDATA'); 
% Store arb in intermal NV memory
fprintf(fgen,'MMEM:STOR:DATA1 "INT:\IDATA.arb"'); 
% Set peak-peak voltage
fprintf(fgen, 'SOUR1:FUNC:ARB:PTPeak 0.5');
% Set offset to 0 V
fprintf(fgen,'SOUR1:VOLT:OFFSET 0'); 
% Set output load to 50 ohms
fprintf(fgen,'OUTPUT1:LOAD 50'); 
% Set sample rate
fprintf(fgen,'SOUR1:FUNC:ARB:SRAT 250e6');
% Advance at sample rate
fprintf(fgen, 'SOUR1:FUNC:ARB:ADV SRAT');
% Turn on arb function
fprintf(fgen,'SOUR1:FUNC ARB'); 


%% Set up Channel 2 with Q data
% Number of bytes
qBytes=num2str(length(Q) * 4); 
% Create header
header= ['SOUR2:DATA:ARB QDATA, #' num2str(length(qBytes)) qBytes]; 
% Convert datapoints to binary before sending
binblockBytes = typecast(Q, 'uint8');  
% Combine header and datapoints then send to instrument
fwrite(fgen, [header binblockBytes'], 'uint8'); 
% Make sure no other commands are exectued until arb is done downloading
fprintf(fgen, '*WAI');   

% Set current arb waveform to defined arb 
fprintf(fgen,'SOUR2:FUNC:ARB QDATA'); 
% Store arb in intermal NV memory
fprintf(fgen,'MMEM:STOR:DATA2 "INT:\QDATA.arb"'); 
% Set peak-peak voltage
fprintf(fgen, 'SOUR2:FUNC:ARB:PTPeak 0.5');
% Set offset to 0 V
fprintf(fgen,'SOUR2:VOLT:OFFSET 0'); 
% Set output load to 50 ohms
fprintf(fgen,'OUTPUT2:LOAD 50'); 
% Set sample rate
fprintf(fgen,'SOUR2:FUNC:ARB:SRAT 250e6');
% Advance at sample rate
fprintf(fgen, 'SOUR2:FUNC:ARB:ADV SRAT');
% Turn on arb function
fprintf(fgen,'SOUR2:FUNC ARB'); 


% Synchronize the two output channels
fprintf(fgen, 'SOUR1:FUNC:ARB:SYNC');

% Turn the output on for both channels
fprintf(fgen,'OUTP1 1');
fprintf(fgen,'OUTP2 1');

% Pause here to do scope capture
pause

err = (query(fgen, 'SYSTem:ERRor?'));
err = (query(fgen, 'SYSTem:ERRor?'));

% Turn the outputs off
fprintf(fgen,'OUTP1 0');
fprintf(fgen,'OUTP2 0');

end

