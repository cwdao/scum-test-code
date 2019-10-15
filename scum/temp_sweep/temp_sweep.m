close all
clear all
clc

delete(instrfind)
tic

NUM_MEASUREMENTS = 3;

if not(exist('temp.mat', 'file'))
    temperature = [];
    measured_temp = [];
    LC_counter = [];
    save('temp.mat', 'temperature', 'measured_temp', 'LC_counter');
end

temperature = zeros(1, NUM_MEASUREMENTS);
measured_temp = zeros(1, NUM_MEASUREMENTS);
LC_counter = zeros(1, NUM_MEASUREMENTS);

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
    
    % Read measured temperature
    fprintf(counter_port, 'lcv');
    fgets(counter_port); % Count executed
    temp = split(fgets(counter_port));
    measured = str2double(temp{2});
    fgets(counter_port); % DONE
    
    pause(0.5)
    
    % Read LC counter
    fprintf(counter_port, 'lct');
    fgets(counter_port); % Count executed
    counter = split(fgets(counter_port));
    count = str2double(counter{2});
    fgets(counter_port); % DONE

    % Add to data arrays
    temperature(i) = str2double(T0);
    measured_temp(i) = measured;
    LC_counter(i) = count;
    fprintf("temp: %.2f measured: %.2f LC: %d\n", temperature(i), measured_temp(i), LC_counter(i));
end

fclose(temp_port);
fclose(counter_port);

m = matfile('temp.mat', 'Writable', true);
m.temperature = [m.temperature temperature(2:end)];
m.measured_temp = [m.measured_temp measured_temp(2:end)];
m.LC_counter = [m.LC_counter LC_counter(2:end)];

toc