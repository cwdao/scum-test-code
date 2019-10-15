function [outvalid] = scm3_ASC_func_v2_fil(lo_enable,...
                                        pa_enable,...
                                        div_enable,...
                                        lo_fine_tune,...
                                        lo_mid_tune,...
                                        lo_coarse_tune,...
                                        lo_current,...
                                        pa_panic,...
                                        pa_ldo,...
                                        lo_panic,...
                                        lo_ldo,...
                                        div_panic,...
                                        div_ldo,...
                                        sel_12,...
                                        en_backup_2,...
                                        en_backup_5,...
                                        pre_dyn_sel, ...
                                        en_64mhz,...
                                        en_20mhz,...
                                        static_code,...
                                        static_en,...
                                        bg_panic,...
                                        bg)

%% PCB Configuration (LDOs, gpio direction)

% Configure voltage for pcb analog and digital LDOs
% 0.8V + [50m 100m 200m 400m]
vdda_pcb_tune = [0 0 0 0];
vddd_pcb_tune = [0 0 0 1];

% SCM GPIO direction control
% 0 = input to SCM, 1 = output from SCM
% [0:15]
gpio_direction = [1 1 1 1  1 1 1 1  1 1 1 1  1 1 1 1];

% Enables for other pcb LDOs
vbat_ldo_enable = 1;
gpio_ldo_enable = 1;
debug_ldo_enable = 0;
vdda_pcb_ldo_enable = 0;
vddd_pcb_ldo_enable = 1;
ldo_enables = [0 0 0 vbat_ldo_enable gpio_ldo_enable debug_ldo_enable vdda_pcb_ldo_enable vddd_pcb_ldo_enable];

%%-----------
% Begin ASC

ASC=zeros(1,1200); % initialize analog scanchain

%% internal section, uses ASD 0:270. copied from scum-3 tapeout google doc, "scan registers" tab.

% zeroth ASC bit is shifted in at the end to prevent more off-by-one errors
% than there will be in this file already

ASC(1)=1;	%  sel_data     , Recovered data , 0 is for LC and 1 is for Ring 
ASC(2)=1;	%  sel_counter0_reset, sel signal for counter 0 reset, 0 from ASC[8] and 1 from M0 analog_cfg[0]
            %  sel signal for counter 0 enable, 0 from ASC[15] and 1 from M0 analog_cfg[7]
ASC(3)=1;	%  sel_counter1_reset, sel signal for counter 1 reset, 0 from ASC[9] and 1 from M0 analog_cfg[1]
            %  sel signal for counter 1 enable, 0 from ASC[16] and 1 from M0 analog_cfg[8]
ASC(4)=1;	%  sel_counter2_reset, sel signal for counter 2 reset, 0 from ASC[10] and 1 from M0 analog_cfg[2]
            %  sel signal for counter 2 enable, 0 from ASC[17] and 1 from M0 analog_cfg[9]
ASC(5)=1;	%  sel_counter3_reset, sel signal for counter 3 reset, 0 from ASC[11] and 1 from M0 analog_cfg[3]
            %  sel signal for counter 3 enable, 0 from ASC[18] and 1 from M0 analog_cfg[10]
ASC(6)=1;	%  sel_counter4_reset, sel signal for counter 4 reset, 0 from ASC[12] and 1 from M0 analog_cfg[4]
            %  sel signal for counter 4 enable, 0 from ASC[19] and 1 from M0 analog_cfg[11]
ASC(7)=1;	%  sel_counter5_reset, sel signal for counter 5 reset, 0 from ASC[13] and 1 from M0 analog_cfg[5]
            %  sel signal for counter 5 enable, 0 from ASC[20] and 1 from M0 analog_cfg[12]
ASC(8)=1;	%  counter0_reset, 0 is reset
            %  && sel signal f or counter 6 reset, 0 from ASC[14] and 1 from M0 analog_cfg[6]
            %  sel signal for counter 6 enable, 0 from ASC[21] and 1 from M0 analog_cfg[13]
ASC(9)=0;	%  counter1_reset, 0 is reset
ASC(10)=0;	%  counter2_reset, 0 is reset
ASC(11)=0;	%  counter3_reset, 0 is reset
ASC(12)=0;	%  counter4_reset, 0 is reset
ASC(13)=0;	%  counter5_reset, 0 is reset
ASC(14)=0;	%  counter6_reset, 0 is reset
ASC(15)=0;	%  counter0_enable, 1 is enable
ASC(16)=0;	%  counter1_enable, 1 is enable
ASC(17)=0;	%  counter2_enable, 1 is enable
ASC(18)=0;	%  counter3_enable, 1 is enable
ASC(19)=0;	%  counter4_enable, 1 is enable
ASC(20)=0;	%  counter5_enable, 1 is enable
ASC(21)=0;	%  counter6_enable, 1 is enable
ASC(22:23)=[1 0];	%  sel_mux3in, 00 LF_CLOCK, 01 LF_ext_PAD, 10 HF_CLOCK, 11 1'b0         //error 10 is external pad
ASC(24)=1;	%  divider_RFTimer_enable, 1 is enable
ASC(25)=1;	%  divider_CortexM0_enable, 1 is enable, HCLK
ASC(26)=1;	%  divider_GFSK_enable, 1 is enable, divider_out_GFSK
ASC(27)=1;	%  divider_ext_GPIO_enable, 1 is enable, EXT_CLK_GPIO
ASC(28)=1;	%  divider_integ_enable, 1 is enable, 
ASC(29)=1;	%  divider_2MHz_enable, 1 is enable,
ASC(30)=1;	%  divider_RFTimer_resetn, 0 is reset
ASC(31)=1;	%  divider_CortexM0_resetn, 0 is reset
ASC(32)=1;	%  divider_GFSK_resetn, 0 is reset
ASC(33)=1;	%  divider_ext_GPIO_resetn, 0 is reset
ASC(34)=1;	%  divider_integ_resetn, 0 is reset
ASC(35)=1;	%  divider_2MHz_resetn, 0 is reset
ASC(36)=1;	%  divider_RFTimer_PT, 1 is passthrough
ASC(37)=1;	%  divider_CortexM0_PT, 1 is passthrough
ASC(38)=1;	%  divider_GFSK_PT, 1 is passthrough
ASC(39)=1;	%  divider_ext_GPIO_PT, 1 is passthrough
ASC(40)=1;	%  divider_integ_PT, 1 is passthrough
ASC(41)=1;	%  divider_2MHz_PT, 1 is passthrough
ASC(49:-1:42)=[0 0 0 0 0 0 0 1];	%  divider_RFTimer_Nin, divide value
ASC(57:-1:50)=[0 0 0 0 0 0 1 1];	%  divider_CortexM0_Nin, divide value
ASC(65:-1:58)=[0 0 0 0 0 0 0 1];	%  divider_GFSK_Nin, divide value
ASC(73:-1:66)=[0 0 0 0 0 0 0 1];	%  divider_ext_GPIO_Nin, divide value
ASC(81:-1:74)=[0 0 0 0 0 0 0 1];	%  divider_integ_Nin, divide value
ASC(89:-1:82)=[0 0 0 0 0 0 0 1];	%  divider_2MHz_Nin, divide value
ASC(90)=0;	%  mux_sel_RFTIMER, 0 is divider_out_RFTIMER, 1 is TIMER32k
ASC(91)=0;	%  mux_sel_GFSK_CLK, 0 is LC_div_N, 1 is divider_out_GFSK
ASC(93:-1:92)=[0 0];	%  mux3_sel_CLK2MHz, 00 RC_2MHz, 01 divider_out_2MHz, 10 LC_2MHz, 11 1'b0
ASC(95:-1:94)=[0 0];	%  mux3_sel_CLK1MHz, 00 LC_1MHz_dyn, 01 divider_out_2MHz, 10 LC_1MHz_stat, 11 1'b0
ASC(97:-1:96)=[0 0];	%  IQ_select, Bob's digital baseband
ASC(99:-1:98)=[0 0];	%  op_mode, Bob's digital baseband
ASC(100)=0;	%  agc_overload, Bob's digital baseband
ASC(101)=0;	%  agc_ext_or_int, Bob's digital baseband
ASC(102)=0;	%  vga_select, Bob's digital baseband
ASC(103)=0;	%  mf_data_sign, Bob's digital baseband
ASC(106:-1:104)=[0 0 0];	%  mux sel for 7 counters output to GPIO bank 11
                            % 0 analog_rdata[31:0]
                            % 1 analog_rdata[63:32]
                            % 2 analog_rdata[95:64]
                            % 3 analog_rdata[127:96]
                            % 4 analog_rdata[159:128]
                            % 5 analog_rdata[191:160]
                            % 6 analog_rdata[223:192]
                            % 7 32'b0
ASC(122:-1:107)=[0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 1];	%  COUNT_THRESH, Brian's DBB
ASC(123)=0;	%  FB_EN, Brian's DBB
ASC(131:-1:124)=[0 0 1 0 0 1 1 0];	%  CLK_DIV_ext, Brian's DBB
ASC(132)=1;	%  DEMOD_EN, Brian's DBB
ASC(143:-1:133)=[0 0 0 0 0 0 0 0 0 0 0];	%  INIT_INTEG, Brian's DBB
ASC(159:-1:144)=0;	
%  ASC(144) sel signal for HCLK divider2 maxdivenb between Memory mapped vs
%	ASC(145) mux in0, 0 ASC[145] 1 analog_cfg[14]
ASC(175:-1:160)=0;	%  Not used
ASC(191:-1:176)=0;	%  Not used
ASC(192)=0;	%  sel_FREQ_CONTR_WORD, Brian's DBB
ASC(208:-1:193)=[0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0];	%  SACLEINT_ext, Brian's DBB
ASC(224:-1:209)=[0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0];	%  EAERLY_DECISION_MARGIN_ext, Brian's DBB
ASC(226:-1:225)=[0 0];	%  mux_sel_FREQ_CTRL_WORD_SCAN
                        % 0 ASC[237:227]
                        % 1 FREQ_CTRL_WORD_SCAN
                        % 2 analog_cfg[74:64]
                        % 3 11'b0
ASC(237:-1:227)=[0 0 0 0 0 0 0 0 0 0 0];	%  FREQ_CTRL_WORD_SCAN
ASC(238)=1;	%  sel dbb Ring data in, 0 SIG_IN, 1 COMPP_ZCC          //choose inputs to brians zcc demod
ASC(239)=1;	%  sel dbb Ring CLK in, 0 ZCC_CLK_Ring, 1 ZCC_CLK
ASC(240)=0;	%  mux sel for RST_B, 0 ASC[241], 1 analog_cfg[75]
ASC(241)=1;	%  ASC RST_B
ASC(242)=0;	%  mux sel ADC reset, 0 adc_reset, 1 adc_reset_gpi
ASC(243)=0;	%  mux sel ADC convert, 0 adc_convert, 1 adc_convert_gpi
ASC(244)=0;	%  mux sel ADC pga amplify, 0 adc_pga_amplify, 1 adc_pga_amplify_gpi
ASC(248:-1:245)=[0 1 1 0];	%  GPO row 1 sel
ASC(252:-1:249)=[0 1 1 0];	%  GPO row 2 sel
ASC(256:-1:253)=[0 1 1 0];	%  GPO row 3 sel
ASC(260:-1:257)=[0 1 1 0];	%  GPO row 4 sel
ASC(262:-1:261)=[0 0];	%  GPI row 1 sel
ASC(264:-1:263)=[0 0];	%  GPI row 2 sel
ASC(266:-1:265)=[1 1];	%  GPI row 3 sel
ASC(268:-1:267)=[0 0];	%  GPI row 4 sel
ASC(269)=0;	%  mux sel for M0 clk, 0 dbbclk, 1 DATA_CLK_IN
ASC(270)=0;	%  mux sel for M0 data, 0 dbbdata, 1 DATA_IN

%% ASC 271:516 -- LC IF Chain
%%----------------------------
%% Q channel gain control
agc_gain_mode = 0;                              % 0 = Gain is controlled via scan chain, 1 = controlled by AGC
code_scan = [1 1 1 1 1 1];                      % Gain control vector from scan chain, binary weighted MSB first <5:0>, all ones is max gain
stg3_gm_tune = [1 1 1 1 1 1 1 1 1 1 1 1 1];     % Gain control for AGC driver <1:13>, all ones is max gain, thermometer (not 1-hot)

% Debug control at output of Q mixer
dbg_bias_en_Q = 0;                              % 1 = Turn on bias for source followers
dbg_out_en_Q = 0;                               % 1 = Turn on output pass gate for debug
dbg_input_en_Q = 0;                             % 1 = Turn on input pass gate for debug

% Mixer bias control
mix_bias_In_ndac = [1 1 1 1];                   % Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage
mix_off_i = 1;                                  % 0 = I mixer enabled
mix_bias_Ip_ndac = [1 1 1 1];                   % Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage
mix_bias_Qn_ndac = [1 1 1 1];                   % Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage
mix_off_q = 1;                                  % 0 = I mixer enabled
mix_bias_Qp_ndac = [1 1 1 1];                   % Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage

mix_bias_In_pdac = [1 1 1 1];                   % Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage
mix_bias_Ip_pdac = [1 1 1 1];                   % Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage
mix_bias_Qn_pdac = [1 1 1 1];                   % Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage
mix_bias_Qp_pdac = [1 1 1 1];                   % Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage

IFsection1 = [agc_gain_mode code_scan stg3_gm_tune dbg_bias_en_Q dbg_out_en_Q dbg_input_en_Q ...
    mix_bias_In_ndac mix_off_i mix_bias_Ip_ndac mix_bias_Qn_ndac mix_off_q ...
    mix_bias_Qp_ndac mix_bias_In_pdac mix_bias_Ip_pdac mix_bias_Qn_pdac mix_bias_Qp_pdac];

%% Q channel
tia_cap_on = [0 0 0 0];                         % TIA bandwidth <4(MSB):1>, binary weighted, all ones = lowest bandwidth                       
tia_pulldown = 1;                               % 1 = pull TIA inputs to ground
tia_enn = 0;                                    % 1 = enable nmos bias in TIA
tia_enp = 1;                                    % 0 = enable pmos bias in TIA

% Q Channel comparator offset trim
pctrl = [0 0 0 0 0];                            % <4(MSB):0>, binary weighted, increase from 0 to add cap to either side of comparator
nctrl = [0 0 0 0 0];                            % <4(MSB):0>, binary weighted, increase from 0 to add cap to either side of comparator

% Q channel comparator control
mode_1bit = 1;                                  % 1 = turn on zero crossing mode
adc_comp_en = 1;                                % Clock gate for comparator, 0 = disable the clock

% Q channel stage3 amp control
stg3_amp_en = 0;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of Q stg3
dbg_out_on_stg3 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg3 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg3 = 0;                           % 1 = Turn on bias for source followers

% Q channel stage2 amp control
stg2_amp_en = 0;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of Q stg2
dbg_out_on_stg2 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg2 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg2 = 0;                           % 1 = Turn on bias for source followers

% Q channel stage1 amp control
stg1_amp_en = 0;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of Q stg1
dbg_out_on_stg1 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg1 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg1 = 0;                           % 1 = Turn on bias for source followers

IFsection2 = [tia_cap_on tia_pulldown tia_enn tia_enp pctrl nctrl mode_1bit adc_comp_en ...
    stg3_amp_en dbg_out_on_stg3 dbg_input_on_stg3 dbg_bias_en_stg3 ...
    stg2_amp_en dbg_out_on_stg2 dbg_input_on_stg2 dbg_bias_en_stg2 ...
    stg1_amp_en dbg_out_on_stg1 dbg_input_on_stg1 dbg_bias_en_stg1];

%% Q channel ADC control
vcm_amp_en = 1;                                 % 1 = enable bias for Vcm buffer amp
vcm_vdiv_sel = [0 0];                           % <0:1> switch cap divider ratio, 00 = ~400mV
vcm_clk_en = 1;                                 % 1 = enable clock for Vcm SC divider
vref_clk_en = 1;                                % 1 = enable clock for Vref SC divider
vref_vdiv_sel = [1 1];                          % <1:0> switch cap divider ratio, 11 = ~50mV, 10 = ~100mV
vref_amp_en = 1;                                % 1 = enable bias for Vref buffer amp
adc_fsm_en = 1;                                 % 1 = enable the adc fsm
adc_dbg_en = 0;                                 % 1 = enable adc debug outputs  (bxp, compp)

IFsection3 = [vcm_amp_en vcm_vdiv_sel vcm_clk_en vref_clk_en vref_vdiv_sel vref_amp_en adc_fsm_en adc_dbg_en];

%% Q channel filter
stg3_clk_en = 1;                                % 1 = enable clock for IIR
stg3_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg3_C1 = [0 0 0];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg2_clk_en = 1;                                % 1 = enable clock for IIR
stg2_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg2_C1 = [0 0 0];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg1_clk_en = 1;                                % 1 = enable clock for IIR
stg1_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg1_C1 = [0 0 0];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

IFsection4 = [stg3_clk_en stg3_C2 stg3_C1 stg2_clk_en stg2_C2 stg2_C1 stg1_clk_en stg1_C2 stg1_C1];

%% I channel ADC control
vcm_amp_en = 1;                                 % 1 = enable bias for Vcm buffer amp
vcm_vdiv_sel = [0 0];                           % <0:1> switch cap divider ratio, 00 = ~400mV
vcm_clk_en = 1;                                 % 1 = enable clock for Vcm SC divider
vref_clk_en = 1;                                % 1 = enable clock for Vref SC divider
vref_vdiv_sel = [1 1];                          % <1:0> switch cap divider ratio, 11 = ~50mV, 10 = ~100mV
vref_amp_en = 1;                                % 1 = enable bias for Vref buffer amp
adc_fsm_en = 1;                                 % 1 = enable the adc fsm
adc_dbg_en = 1;                                 % 1 = enable adc debug outputs (bxp, compp)

IFsection5 = [vcm_amp_en vcm_vdiv_sel vcm_clk_en vref_clk_en vref_vdiv_sel vref_amp_en adc_fsm_en adc_dbg_en];

%% I channel filter
stg3_clk_en = 1;                                % 1 = enable clock for IIR
stg3_C2 = [0 0 1];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg3_C1 = [1 1 1];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg2_clk_en = 1;                                % 1 = enable clock for IIR
stg2_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg2_C1 = [0 0 1];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg1_clk_en = 1;                                % 1 = enable clock for IIR
stg1_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg1_C1 = [0 0 0];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

IFsection6 = [stg3_clk_en stg3_C2 stg3_C1 stg2_clk_en stg2_C2 stg2_C1 stg1_clk_en stg1_C2 stg1_C1];

%% Clock generation control
adc_dbg_en = 1;                                 % 1 = enable output of debug phases for adc clocks (cx,phix)                      
adc_phi_en = 1;                                 % 1 = enable clocks for adc (cx,phix)
filt_phi_en = 1;                                % 1 = enable 4 filter clock phases (0,90,180,270)
clk_select = [0 1];                             % <1:0> mux select for IF clock 00=gnd 01=internal RC 10=divided LC 11=external pad
RC_clk_en = 1;                                  % 1 = enable RC osc
RC_coarse = [0 0 0 0 0];                        % coarse tuning for RC, binary weighted <4(MSB):0>
filt_dbg_en = 1;                                % 1 = enable debug outputs for 4 filter clock phases (0,90,180,270)
RC_fine = [0 0 0 0 0];                          % fine tuning for RC, binary weighted <4(MSB):0>

IFsection7 = [adc_dbg_en adc_phi_en filt_phi_en clk_select RC_clk_en RC_coarse filt_dbg_en RC_fine];

%% I channel
tia_cap_on = [0 0 0 0];                         % TIA bandwidth <4(MSB):1>, binary weighted, all ones = lowest bandwidth                       
tia_pulldown = 1;                               % 1 = pull TIA inputs to ground
tia_enn = 0;                                    % 1 = enable nmos bias in TIA
tia_enp = 1;                                    % 0 = enable pmos bias in TIA

% I Channel comparator offset trim
pctrl = [0 0 0 0 0];                            % <4(MSB):0>, binary weighted, increase from 0 to add cap to either side of comparator
nctrl = [0 0 0 0 0];                            % <4(MSB):0>, binary weighted, increase from 0 to add cap to either side of comparator

% I channel comparator control
mode_1bit = 0;                                  % 1 = turn on zero crossing mode
adc_comp_en = 1;                                % Clock gate for comparator, 0 = disable the clock

% I channel stage3 amp control
stg3_amp_en = 1;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of I stg3
dbg_out_on_stg3 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg3 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg3 = 0;                           % 1 = Turn on bias for source followers

% I channel stage2 amp control
stg2_amp_en = 1;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of I stg2
dbg_out_on_stg2 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg2 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg2 = 0;                           % 1 = Turn on bias for source followers

% I channel stage1 amp control
stg1_amp_en = 1;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of I stg1
dbg_out_on_stg1 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg1 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg1 = 0;                           % 1 = Turn on bias for source followers

IFsection8 = [dbg_bias_en_stg1 dbg_input_on_stg1 dbg_out_on_stg1 stg1_amp_en ...
    dbg_bias_en_stg2 dbg_input_on_stg2 dbg_out_on_stg2 stg2_amp_en ...
    dbg_bias_en_stg3 dbg_input_on_stg3 dbg_out_on_stg3 stg3_amp_en ...
    adc_comp_en mode_1bit nctrl pctrl tia_enp tia_enn tia_pulldown tia_cap_on];


%% I channel
% Debug control at output of I mixer
dbg_input_en_I = 0;                             % 1 = Turn on input pass gate for debug
dbg_out_on_I = 0;                               % 1 = Turn on output pass gate for debug
dbg_bias_en_I = 0;                              % 1 = Turn on bias for source followers


% I channel gain control
stg3_gm_tune = [1 1 1 1 1 1 1 1 1 1 1 1 1];     % Gain control for AGC driver <13:1>, all ones is max gain, thermometer (not 1-hot)
code_scan = [0 1 1 1 1 1];                      % Gain control vector from scan chain, binary weighted <0:5(MSB)>, all ones is max gain
% code_scan = zeros(1,6);
agc_gain_mode = 1;                              % 0 = Gain is controlled via scan chain, 1 = controlled by AGC

IFsection9 = [dbg_input_en_I dbg_out_on_I dbg_bias_en_I stg3_gm_tune code_scan agc_gain_mode];


%% LDO control
% if_ldo_rdac = [0 1 0 1 0 1 1];                  % Resistor for setting bandgap ref voltage <0:6(MSB)>
if_ldo_rdac = [0 0 0 0 0 0 0];                  % Resistor for setting bandgap ref voltage <0:6(MSB)>
por_disable = 1;                                % 1 = disable ability for POR to reset IF blocks
scan_reset = 1;                                 % 0 = force reset from scan chain

IFsection10 = [if_ldo_rdac por_disable scan_reset];


%% Compile into one vector
IF_scan = [IFsection1 IFsection2 IFsection3 IFsection4 IFsection5 IFsection6 IFsection7 IFsection8 IFsection9 IFsection10];

%% Power on control module
% scan_pon_XX turns on LDO via scan chain
% gpio_pon_en_XX will turn on LDO if power on signal from GPIO bank is high
% fsm_pon_en_XX will turn on LDO if power on signal from radio FSM is high
% master_ldo_en_XX = 0 forces LDO off; =1 allows control from gpio, fsm, or scan-chain
scan_pon_if = 0;
scan_pon_lo = lo_enable;
scan_pon_pa = pa_enable;
gpio_pon_en_if = 0;
fsm_pon_en_if = 0;
gpio_pon_en_lo = 0;
fsm_pon_en_lo = 0;
gpio_pon_en_pa = 0;
fsm_pon_en_pa = 0;
master_ldo_en_if = 0;
master_ldo_en_lo = lo_enable;
master_ldo_en_pa = 1;
scan_pon_div = div_enable;
gpio_pon_en_div = 0;
fsm_pon_en_div = 0;
master_ldo_en_div = div_enable;
power_control = [scan_pon_if scan_pon_lo scan_pon_pa ...
    gpio_pon_en_if fsm_pon_en_if ...
    gpio_pon_en_lo fsm_pon_en_lo ...
    gpio_pon_en_pa fsm_pon_en_pa ...
    master_ldo_en_if master_ldo_en_lo master_ldo_en_pa ...
    scan_pon_div gpio_pon_en_div fsm_pon_en_div master_ldo_en_div];

% Entire LC IF sigpath
ASC(271:516) = [IF_scan power_control];
%%----------------------------


% db section A, uses ASC 517:945

% initialize
ASC(517:945)=zeros(size(ASC(517:945)));
ASC(1089:1114)=zeros(size(ASC(1089:1114)));

ASC(517:622)=zeros(size(ASC(517:622))); % rdata bank A
ASC(623)=0  ; 		% 32k timer enable
ASC(624:725)=zeros(size(ASC(624:725))); % rdata bank B
ASC(726:736)=0; % floating

ASC(743:-1:737)=dec2bin(2,7)-48; % RF current source coarse tuning

ictrl(1:4)=zeros(size(1:4)); % toggle current mirror device for the 4 rings
switch -1
	case -1
		ictrl(1:4)=zeros(size(1:4));
	case 0 % s1 ring oscillator
		ictrl(1)=1;
	case 1 % s10 ring oscillator
		ictrl(2)=1;
	case 2 % d8 ring oscillator
		ictrl(3)=1;
	case 3 % d4 ring oscillator
		ictrl(4)=1;
	otherwise
		ictrl(1:4)=zeros(size(1:4));
end
ASC(744:747)=ictrl; 

IF_gate_en(1:10)=zeros(size(1:10)); % enable mixer IF output to baseband amps
switch -1
	case -1
		IF_gate_en(1:10)=zeros(size(1:10));
	case 0 % s1 ring oscillator
		IF_gate_en(1)=1;
	case 1 % s10 ring oscillator
		IF_gate_en(2)=1;
	case 2 % d8 ring oscillator
		IF_gate_en(3:6)=[1,1,1,1];
	case 3 % d4 ring oscillator
		IF_gate_en(7:10)=[1,1,1,1];
	otherwise
		IF_gate_en(1:10)=zeros(size(1:10));
end

LOPA_en(1:4)=zeros(size(1:4)); % enable RF osc output to PA
switch -1
	case -1
		LOPA_en(1:4)=zeros(size(1:4));
	case 0 % s1 ring oscillator
		LOPA_en(1)=1;
	case 1 % s10 ring oscillator
		LOPA_en(2)=1;
	case 2 % d8 ring oscillator
		LOPA_en(3)=1;
	case 3 % d4 ring oscillator
		LOPA_en(4)=1;
	otherwise
		LOPA_en(1:4)=zeros(size(1:4));
end

div_en(1:4)=zeros(size(1:4)); % enable RF/8 divider
div_gate_en(1:4)=zeros(size(1:4)); % enable /8 divider output to more stages of dividers
switch -1
	case -1
		div_en(1:4)=zeros(size(1:4));
		div_gate_en(1:4)=zeros(size(1:4));
	case 0 % s1 ring oscillator
		div_en(1)=1;
		div_gate_en(1)=1;
	case 1 % s10 ring oscillator
		div_en(2)=1;
		div_gate_en(2)=1;
	case 2 % d8 ring oscillator
		div_en(3)=1;
		div_gate_en(3)=1;
	case 3 % d4 ring oscillator
		div_en(4)=1;
		div_gate_en(4)=1;
	otherwise
		div_en(1:4)=zeros(size(1:4));
		div_gate_en(1:4)=zeros(size(1:4));
end

% <0>: smaller single-ended ring oscillator
% <1>: larger single-ended ring oscillator
ASC(748)=div_gate_en(2); % div_gate_en<1>
ASC(749)=0; % kick<1>
ASC(750)=div_en(2); % div_en<1>
ASC(751)=LOPA_en(2); % LOPA_en<1>
ASC(752)=LOPA_en(1); % LOPA_en<0>
ASC(753)=div_en(1); % div_en<0>
ASC(754)=div_gate_en(1); % div_gate_en<0>
ASC(755)=0; % kick<0>
ASC(756)=IF_gate_en(1); % IF_gate_en<0>
ASC(757)=IF_gate_en(2); % IF_gate_en<1>

ASC(758:765)=dec2bin(0,8)-48;	% temp sensor LDO IREF constant-gm resistor; adjusts comparator and PGA amp bias current

PGA_GAIN=dec2bin(0,8)-48; % PGA gain setting
ASC(766:771)=PGA_GAIN(1:6); 
ASC(772)=0; % floating -- PGA_GAIN(7) is at ASC(800)
ASC(773)=PGA_GAIN(8);

ASC(774:777)=zeros(size(774:777)); % floating

LDO_tune=dec2bin(0,7)-48; % temp sensor LDO reference voltage adjustment
ASC(778)=LDO_tune(1); 

ASC(779:784)=LDO_tune(7:-1:2); % LDO_tune(2) is overwritten by MUX_SEL(2) below

MUX_SEL=dec2bin(0,2)-48; % PGA/ADC input mux. 0=VPTAT, 1=VBAT/4, 2=external pad, 3=n/a, floating;
ASC(780)=MUX_SEL(2); % LSB of PGA input mux, also LDO_tune(2) oops

% ASC(790:-1:785)=dec2bin(2^6-1,6)-48; % tune BGR voltage that was padded out 
ASC(790:-1:785)=[0 1 1 1 1 1]; % MSB is inverted
% ASC(797:-1:791)=dec2bin(2^6-1,7)-48; % tune BGR voltage serving as VDDD LDO reference
ASC(791:1:797)=[0 1 1 1 1 1 1]; % should set VDDD ~1V

ASC(798)=0; % enable VBATT/4 voltage divider
ASC(799)=0; % POR bypass in reset logic; low=not bypassed

ASC(800)=PGA_GAIN(7); 

ASC(801)=0; % enable temp sensor LDO

ASC(802)=1; % padded out; set this to 1 so system resets to 0 then toggles this high on successful ASC programming

ASC(803:815)=zeros(size(ASC(803:815))); % floating

ASC(823:-1:816)=dec2bin(0,8)-48; % adjust replica dac that controls time taken for ADC capdac to settle

ASC(824)=0; % floating

HFDIVmux1=dec2bin(0,2)-48; % mux for HFout1 aka Ringdiv8_clock; 0=external pad, 1=synthesized, 2=dynamic logic
ASC(825)=HFDIVmux1(1); % MSB of HFDIVmux1

ASC(826)=0; % mux for HFout0 aka HF_clock; 0=dynamic logic, 1=synthesized

ASC(827)=1; % enableB, dynamic divider for HFout0 aka HF_clock; 1=off
ASC(828)=1; % resetn, synthesized divider for HFout0 aka HF_clock; 1=not being reset
ASC(829)=0; % enable, synthesized divider for HFout0 aka HF_clock; 0=off

HFdiv_dynamic0=dec2bin(0,13)-48; % divide ratio for dynamic divider of HFout0 aka HF_clock
HFdiv_dynamic1=dec2bin(0,13)-48; % divide ratio for dynamic divider of HFout1 aka Ringdiv8_clock
HFdiv_static0=dec2bin(0,16)-48; % divide ratio for synthesized divider of HFout0 aka HF_clock
HFdiv_static1=dec2bin(0,16)-48; % divide ratio for synthesized divider of HFout1 aka Ringdiv8_clock

ASC(830:833)=HFdiv_dynamic0(10:13); % last 4 LSBs
ASC(834:837)=HFdiv_dynamic1(10:13); % last 4 LSBs

ASC(838)=1; % resetn, synthesized divider for HFout1 aka Ringdiv8_clock; 1=not being reset
ASC(839)=0; % enable, synthesized divider for HFout1 aka Ringdiv8_clock; 0=off
ASC(840)=1; % enableB, dynamic divider for HFout1 aka Ringdiv8_clock; 1=off

ASC(841)=HFDIVmux1(2); % LSB of HFDIVmux1

ASC(842)=0; % floating
ASC(843)=0; % floating

ASC(844:851)=HFdiv_static1(9:16); % last 8 LSBs
ASC(852:859)=HFdiv_static0(9:16); % last 8 LSBs

ASC(860)=HFdiv_dynamic1(9); % 5th LSB
ASC(861)=HFdiv_dynamic0(9); % 5th LSB

ASC(862:869)=HFdiv_dynamic1(1:8); % upper 8 MSBs
ASC(870:877)=HFdiv_dynamic0(1:8); % upper 8 MSBs

ASC(878)=0; % floating
ASC(879)=0; % floating

ASC(880:887)=HFdiv_static1(1:8); % first 8 MSBs
ASC(888:895)=HFdiv_static0(1:8); % first 8 MSBs

ASC(896)=IF_gate_en(9); % IF_gate_en<8>
ASC(897)=div_gate_en(4); % div_gate_en<3>
ASC(898)=IF_gate_en(5); % IF_gate_en<4>
ASC(899)=div_gate_en(3); % div_gate_en<2>
ASC(900)=div_en(4); % div_en<3>
ASC(901)=IF_gate_en(6); % IF_gate_en<5>
ASC(902)=div_en(3); % div_en<2>
ASC(903)=IF_gate_en(10); % IF_gate_en<9>
ASC(904)=LOPA_en(3); % LOPA_en<2>
ASC(905)=LOPA_en(4); % LOPA_en<3>
ASC(906)=IF_gate_en(3); % IF_gate_en<2>
ASC(907)=0; % kick<2>
ASC(908)=IF_gate_en(4); % IF_gate_en<3>
ASC(909)=0; % kick<3>
ASC(910)=IF_gate_en(7); % if_gate_en<6>
ASC(911)=IF_gate_en(8); % if_gate_en<7>

ASC(912)=0; % ring IF LDO enable
ASC(913)=0; % ring PA input enable at PA side

ASC(914)=0; % floating

ASC(915)=MUX_SEL(1); % MSB of PGA input mux

ASC(916)=1; % aux digital LDO enable
% ASC(923:-1:917)=dec2bin(0,7)-48; % adjust BGR reference voltage used by aux digital LDO

% ASC(923:-1:917) = [1 1 1 0 0 0 0];
ASC(923:-1:917) = [0 0 0 0 0 0 0];

% ASC(924:929)=dec2bin(0,6)-48; % adjust BGR reference voltage used by always on/analog scan LDO

% ASC(924:929) = [0 1 1 1 1 1];       %760mV
% ASC(924:929) = [0 1 0 0 0 0];       %800mV
% ASC(924:929) = [1 1 1 1 1 1];       %840mV

ASC(924:929) = [0 0 0 0 0 0];  

ASC(930)=0; % floating
ASC(931)=0; % floating

% ASC(932:940)=dec2bin(0,9)-48; % tune current & freq of 20MHz ring
ASC(932:940) = [1 0 0 0 0 0 0 0 0];

ASC(941)=0; % kickstart 20MHz ring

ASC(942:945)=zeros(size(ASC(942:945))); % floating

%% ASC 946:1088

ASC(946:1088)=zeros(size(ASC(946:1088)));

ASC(979) = 0; %unusued
ASC(987) = 0; %unused
ASC(1011) = 0; %unused

%  LC LO frequency tuning and RX/TX
fine_code =   lo_fine_tune;
mid_code =    lo_mid_tune;
coarse_code = lo_coarse_tune;
fine_tune = [fliplr(de2bi(fine_code,5)) 0];
mid_tune = [fliplr(de2bi(mid_code,5)) 0];
coarse_tune = [fliplr(de2bi(coarse_code,5)) 0];

lo_tune_select = 0; % 0 for cortex, 1 for scan chain
polyphase_enable = 0; % 0 disables, 1 enables
lo_current_tune = fliplr(de2bi(lo_current,8));

%  test BG for visibility:
%test_bg = [1 0 1 1 1 1 1]; % panic bit -> msb -> lsb
test_bg = [bg_panic fliplr(de2bi(bg,6))];
%test_bg = [0 0 0 0 0 0 1];

%  LDO settings:
pa_ldo_rdac = [pa_panic fliplr(de2bi(pa_ldo,6))];
lo_ldo_rdac = [lo_panic fliplr(de2bi(lo_ldo,6))];
div_ldo_rdac = [div_panic fliplr(de2bi(div_ldo,6))];

%  modulation settings
mod_logic = [0 1 1 1]; % msb -> lsb, s3:0 to not invert, 1 to invert
                       %             s2:0 cortex, 1 pad
                       %             s0,s1: choose between pad, cortex,
                       %             vdd, gnd
mod_15_4_tune = [0 0 0]; %msb -> lsb
mod_15_4_tune_d = 0; % spare dummy bit

%  divider settings
sel_1mhz_2mhz = sel_12; % 0 is for 2mhz, 1 is for 1mhz
pre_dyn_dummy_b = 0; % disconnected
pre_dyn_b = [0 0 0 0 0 0]; % disconnected
pre_dyn_en_b = 0; % disconnected
pre_2_backup_en = en_backup_2; % enable the backup div-by-2 pre-scaler
pre_2_backup_en_b = 1-pre_2_backup_en; % disable the div-by-2 pre-scaler
pre_5_backup_en = en_backup_5; % enable the backup div-by-5 pre-scaler
pre_5_backup_en_b = 1-pre_5_backup_en; % disable the backup div-by-5 pre-scaler
pre_dyn = [1 1 1 0 0 0];
if(pre_dyn_sel == 0)
    pre_dyn = [1 1 1 0 0 0]; % by-5, by-2, by-5, disconnected, disconnected, disconnected
elseif(pre_dyn_sel == 1)
    pre_dyn = [0 1 1 0 0 0];
elseif(pre_dyn_sel == 2)
    pre_dyn = [1 0 1 0 0 0];
elseif(pre_dyn_sel == 3)
    pre_dyn = [1 1 0 0 0 0];
end
pre_dyn_dummy = 0; % disconnected
div_64mhz_enable = en_64mhz; % enable the 64mhz clock source
div_20mhz_enable = en_20mhz; % enable the 20mhz clock source
div_static_code = static_code; % blaze it
div_static_code_bin = de2bi(div_static_code,16); % in lsb->msb order
div_static_rst_b = 1; % reset (active low) of the static divider
div_static_en = static_en; % static divider enable
div_static_select = [   div_static_code_bin(11:16), ...
                        div_static_code_bin(5:10), ...
                        div_static_en, ...
                        div_static_rst_b, ...
                        div_static_code_bin(1:4)    ];
div_static_select = 1 - div_static_select;
                    
dyn_div_N = 420; % joke's not funny anymore
div_dynamic_select = de2bi(dyn_div_N,13);
div_dynamic_en_b = 1; % disable the dynamic divider
div_dynamic_select = [div_dynamic_select div_dynamic_en_b];
div_tune_select = 1; % 0 for cortex, 1 for scan
source_select_2mhz = [0 0]; % 00 static, 01 dynamic, 10 from aux, 11 pad

% BLE module settings - I have forgotten what most of these do
scan_io_reset = 1;
scan_5mhz_select = [0 0]; % msb -> lsb
scan_1mhz_select = [0 0]; % msb -> lsb
scan_20mhz_select = [0 1]; % msb -> lsb
scan_async_bypass = 0;
scan_mod_bypass = 1;
scan_fine_trim = [0 0 0 0 0 0]; % msb -> lsb % note: test 0 0 0 1 1 1 later.
scan_data_in_valid = 1;
scan_ble_select = 0;

ASC(946:978) = [fine_tune mid_tune ...
                coarse_tune ...
                lo_tune_select ...
                test_bg(2:end) ...
                polyphase_enable ...
                pa_ldo_rdac];
ASC(979) = 0;                               % unused
ASC(980:986) = [lo_ldo_rdac];
ASC(987) = 0;                               % unused
ASC(988:1010) = [lo_current_tune ...
                mod_logic ...
                fliplr(mod_15_4_tune) ...
                mod_15_4_tune_d ...
                div_ldo_rdac ...
                ];
ASC(1011) = 0;
ASC(1012:1086) = [sel_1mhz_2mhz ...
                  pre_dyn_dummy_b ...
                  pre_dyn_b ...
                  pre_dyn_en_b ...
                  pre_2_backup_en_b ...
                  pre_2_backup_en ...
                  pre_5_backup_en ...
                  pre_5_backup_en_b ...
                  pre_dyn ...
                  pre_dyn_dummy ...
                  div_64mhz_enable ...
                  div_20mhz_enable ...
                  scan_io_reset ...
                  scan_5mhz_select ...
                  scan_1mhz_select ...
                  scan_20mhz_select ...
                  scan_async_bypass ...
                  scan_mod_bypass ...
                  scan_fine_trim ...
                  div_static_select ...
                  div_dynamic_select ...
                  div_tune_select ...
                  test_bg(1) ...
                  source_select_2mhz ...
                  scan_data_in_valid ...
                  scan_ble_select
                  ];
              
ASC(1034) =         scan_io_reset;
ASC(1035:1036) =    scan_5mhz_select;
ASC(1037:1038) =    scan_1mhz_select;
ASC(1039:1040) =    scan_20mhz_select;
ASC(1041) =         scan_async_bypass;
ASC(1042) =         scan_mod_bypass;
ASC(1043:1048) =    scan_fine_trim;
ASC(1085) =         scan_data_in_valid;
ASC(1086) =         scan_ble_select;

ASC(1087) = 0;                              % unused
ASC(1088) = 0;                              % unused

%% db section B, uses ASC 1089:1114

% ASC(1089:1113) = dec2bin(0,25)-48; % 2MHz RC symbol clock: coarse1/2/3, fine, superfine

ASC(1089:1093) = [1 1 1 1 1]; % coarse 1
ASC(1094:1098) = [1 1 1 1 1]; % coarse 2
ASC(1099:1103) = [1 0 1 1 1]; % coarse 3
ASC(1104:1108) = [0 0 0 0 0]; % fine
ASC(1109:1113) = [0 0 0 0 0]; % superfine
ASC(1114) = 0; % 2MHz RC enable

%% GPIO Direction Control

% 1 = enabled, 0 = disabled
% <0:15>
%out_mask = gpio_direction;
out_mask = [1 1 1 1  1 1 1 1  1 1 1 1  1 1 1 1];
%out_mask = [0 0 0 0  0 0 0 0  0 0 0 0  0 0 0 0]; 
%in_mask = -gpio_direction + 1;
in_mask = [1 1 1 1  1 1 1 1  1 1 1 1  1 1 1 1];
%in_mask = [0 0 0 0  0 0 0 0  0 0 0 0  0 0 0 0];



% Turning on loopback of gp<1> to conenct gpo<1> to GPIO_PON
%%--------------------------
in_mask(2) = 1;
%%--------------------------




% Map to ASC values, on-chip mapping is:
% out_en<0:15> = ASC<1131>,ASC<1133>,ASC<1135>,ASC<1137>,ASC<1140>,ASC<1142>,ASC<1144>,ASC<1146>,ASC<1115>,ASC<1117>,ASC<1119>,ASC<1121>,ASC<1124>,ASC<1126>,ASC<1128>,ASC<1130>				
% in_en<0:15> = ASC<1132>,ASC<1134>,ASC<1136>,ASC<1138>,ASC<1139>,ASC<1141>,ASC<1143>,ASC<1145>,ASC<1116>,ASC<1118>,ASC<1120>,ASC<1122>,ASC<1123>,ASC<1125>,ASC<1127>,ASC<1129>	
ASC(1131:2:1137) = out_mask(1:4);
ASC(1140:2:1146) = out_mask(5:8);
ASC(1115:2:1121) = out_mask(9:12);
ASC(1124:2:1130) = out_mask(13:16);
ASC(1132:2:1138) = in_mask(1:4);
ASC(1139:2:1145) = in_mask(5:8);
ASC(1116:2:1122) = in_mask(9:12);
ASC(1123:2:1129) = in_mask(13:16);

% Invert due to active low on chip
ASC(1115:1146) = -(ASC(1115:1146) - 1);

%% leftover bits

ASC(1147:1199)=zeros(size(ASC(1147:1199))); % floating


% % Check RC out on ring side
% ASC(912) = 0;               %ldo
% [ASC(517:622), ASC(624:725)] = RingIF_scan(); % set IF with function

%% shift in zeroth bit
sel_clk=0;              % select signal for recovered clock from clock recovery module, 0 is for LC , 1 is for Ring
ASC(1:1200) = [sel_clk, ASC(1:1199)];

%% Print ASC configuration

for iii = 1:37
    x1 = (sprintf('%d-%d',(iii-1)*32,(iii-1)*32+31));
    x2 = dec2hex(bin2dec(num2str(ASC((iii-1)*32+1:(iii-1)*32+32))),8);
    fprintf('ASC[%d] = 0x%s;   //%s\n',iii-1,x2,x1)
end

x2 = dec2hex(bin2dec(num2str(ASC(1184:1200))),8);
fprintf('ASC[37] = 0x%s;\n',x2)

ASC=1-ASC;

%% Send to uC

% Open COM port 
%s = serial('/dev/cu.usbmodem3954801','BaudRate',19200); % SCuMdog milliona
s = serial('COM9','BaudRate',19200);
%s = serial('/dev/cu.usbmodem3642851','BaudRate',19200);
%s = serial('/dev/cu.usbmodem4094511','BaudRate',19200);
%s = serial('/dev/cu.usbmodem4095451','BaudRate',19200);
%s = serial('/dev/cu.usbmodem4001171','BaudRate',19200);
%s = serial('/dev/cu.usbmodem4002011','BaudRate',19200);
s.OutputBufferSize = 2000; 
s.InputBufferSize = 2000;
fopen(s);

%% Transmit pcb config to microcontroller
fprintf(s,'boardconfig');
fprintf(s,num2str(bin2dec(num2str([vdda_pcb_tune vddd_pcb_tune]))));
fprintf(s,num2str(bin2dec(num2str(gpio_direction))));
fprintf(s,num2str(bin2dec(num2str(ldo_enables))));
% disp(fgets(s))

%% ASC configuration
pause(0.1)

fprintf(s,'clockon');

% Convert array to string for uart
ASC_string = sprintf('%d', circshift(ASC,0));

% Send string to uC and program into IC
fprintf(s,'ascwrite');
disp(fgets(s))
fprintf(s,fliplr(ASC_string)); % fliplr to feed bits in the correct direction
disp(fgets(s))

% Execute the load command to latch values inside chip
fprintf(s,'ascload');
disp(fgets(s));

% Read back the scan chain contents
fprintf(s,'ascread');
x = (fgets(s));
x = fliplr(x(1:1200));
disp(ASC_string)
disp(x)

% Compare what was written to what was read back
if(strcmp(ASC_string,x))
    disp('Read matches Write')
else
    disp('Read/Write Comparison Incorrect')
end

fclose(s);