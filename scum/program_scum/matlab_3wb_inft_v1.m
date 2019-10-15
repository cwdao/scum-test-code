
close all
clear all
clc

% 12/21/17: Verified working with command_interpreter_v2 for bootloading Nexys4
    
% run('E:\Box Sync\Group\VAPR\SCM\SCMv3_Testing\writeable_IMEM\embed_payload')


delete(instrfind)


tic
% Open COM port 
s = serial('COM9','BaudRate',19200);
s.OutputBufferSize = 70000; 

s.InputBufferSize = 2000; 
fopen(s);



% Open binary file and pad out to 64kB with zero
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\check_raw_chips\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\Mesri_scm_digital_demo_files\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\simplest_gpio_demo\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\SCM3_MF_rx_loop\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\SCM3_MF_rx_loop_v2\code.bin');

% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\SCM3_MF_rx_loop_v3\code.bin');

% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\SCM3_MF_rx_loop_v4\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\SCM3_MF_rx_loop_v5\code.bin');
% binFile = fopen('E:\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\check_imem_write_v2\code.bin');
% binFile = fopen('E:\Box Sync\Group\VAPR\SCM\SCMv3_Testing\writeable_IMEM\combined.bin');

binFile = fopen('C:\Users\Titan Yuan\Documents\scum\scm_digital\proj\keil\uRobotDigitalController\code.bin');

% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\tb_coldboot_v1\code.bin');

% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\check_imem_write_v2\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\tb_gain_change_transients\code.bin');


% binFile = fopen('C:\Users\Brad\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\SCM3_MF_rx_loop_v2\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\check_if_estimate\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\SCM3_FW_v8\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\agc_read_test\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\check_raw_chips\code.bin');
% binFile = fopen('C:\Research\Box Sync\Group\VAPR\SCM\scm-digital\proj\keil\SCM3_FW_v9_PONtest\code.bin');

% binFile = fopen('C:\Users\Brad\Box Sync\Home_Sync\Box Sync\Home_Sync2\code.bin');

bindata = fread(binFile);
bindata = [bindata' zeros(1,65536-length(bindata))]';

% Tell uC to prepare to receive 64kB of sram data
fprintf(s,'transfersram');
disp(fgets(s))
fwrite(s,bindata);
disp(fgets(s))

% Execute the 3wb bootloader on uC

fprintf(s,'boot3wb');
disp(fgets(s))

% Cleanup
fclose(s);
fclose(binFile);
toc
