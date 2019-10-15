close all
clear all
clc

delete(instrfind)
tic

NUM_MEASUREMENTS = 5;

if not(exist('temp_sweep.mat', 'file'))
    temperature = [];
    counts_2M = [];
    counts_32k = [];
    ratios_2M_32k = [];
    save('temp_sweep.mat', 'temperature', 'counts_2M', 'counts_32k', 'ratios_2M_32k');
end

temperature = zeros(1, NUM_MEASUREMENTS);
counts_2M = zeros(1, NUM_MEASUREMENTS);
counts_32k = zeros(1, NUM_MEASUREMENTS);
% counts_LC = zeros(1, NUM_MEASUREMENTS);
ratios_2M_32k = zeros(1, NUM_MEASUREMENTS);

temp_port = serial('COM16', 'BaudRate', 15200);
fopen(temp_port);

counter_port = serial('COM8', 'BaudRate', 19200);
fopen(counter_port);

% LOOP
for i = 1:NUM_MEASUREMENTS + 1
    fprintf("%d ", i);
    
    % Read temperature
    fprintf(temp_port, 'temp');
    T0 = '';
    while isempty(T0)
        T0 = fgets(temp_port);
    end

    % Read LC tank count
    % fprintf(counter_port, 'lct');
    % fgets(counter_port); % Count executed
    % count = split(fgets(counter_port));
    % count_LC = str2double(count{2});
    % fgets(counter_port); % DONE

    % Read 2MHz and 32kHz counts
    fprintf(counter_port, 'lcv');
    fgets(counter_port); % Count executed
    count = split(fgets(counter_port));
    count_2M = str2double(count{2});
    count = split(fgets(counter_port));
    count_32k = str2double(count{2});
    fgets(counter_port); % DONE

    % Add to data arrays
    temperature(i) = str2double(T0);
    counts_2M(i) = count_2M;
    counts_32k(i) = count_32k;
    % counts_LC(i) = count_LC;
    ratios_2M_32k(i) = count_2M / count_32k;
    fprintf("temp: %.2f count_2M: %d count_32k: %d\n", temperature(i), counts_2M(i), counts_32k(i));
end

fclose(temp_port);
fclose(counter_port);

m = matfile('temp_sweep.mat', 'Writable', true);
m.temperature = [m.temperature temperature(2:end)];
m.counts_2M = [m.counts_2M counts_2M(2:end)];
m.counts_32k = [m.counts_32k counts_32k(2:end)];
m.ratios_2M_32k = [m.ratios_2M_32k ratios_2M_32k(2:end)];

toc