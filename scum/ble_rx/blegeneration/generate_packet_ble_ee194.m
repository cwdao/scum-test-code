function [I,Q,packet01] = generate_packet_ble_ee194(Fs)

% Chip period
T = 1/1e6;
dT = 1/Fs;
dt=dT;

%% Pre-written packet

packet01=[0,1,0,1,0,1,0,1,0,1,1,0,1,0,1,1,0,1,1,1,1,1,0,1,1,0,0,1,0,0,0,1,0,1,1,1,0,0,0,1,1,1,1,1,0,0,0,1,0,1,0,0,0,0,1,1,1,0,0,0,1,0,0,1,1,0,0,0,0,1,0,0,1,1,1,1,0,0,0,0,1,0,1,0,1,0,1,1,0,0,1,0,0,1,1,0,0,0,0,0,1,1,0,1,1,1,1,0,1,1,1,0,0,0,0,0,1,1,0,0,0,0,1,0,1,0,0,0,0,1,1,1,0,0,1,0,0,1,1,1,1,0,0,1,0,1,0,0,1,1,0,0,1,0,1,1,1,1,0,1,0,1,0,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,0,1,1,0,0,0,0,0,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,0,1,1,1];


% Convert packet to +/- 1
packet = 2 * packet01 - 1;
bits=1*packet; 

%% Weaver


%% setup

wif=2*pi*2.5e6; % 2.5MHz IF
fmod=.1e6; wmod=2*pi*fmod; % +/- 250kHz
fdata=1e6; % need freq not natural freq b/c this value doesn't generate a cosine
wrf=2*pi*0.05e9; % 50MHz for now
duration=numel(bits)/fdata;

% dt=1/250e6; % Agilent AWG runs at 250MHz
t=0:dt:duration-dt;

% FFT generals
L=size(t,2);
f=[(-1/dt)*(L/2-1:-1:0)/L (1/dt)*(1:1:L/2)/L];



%% build w(t) from bits

% this section turns base bit sequence into bits sampled at dt

if(0) % abrupt freq changes
    pt=[]; 
    for ii=1:numel(bits)
        pt=[pt bits(ii)*ones(1,1/(dt*fdata))];
    end
    dwdt=pt*wmod;
end

if(0) % half-sine shaping, probably wrong
    pt=[];
    for ii=1:numel(bits)
        pt=[pt bits(ii)*sin(2*pi*fdata/2*(0:dt:1/fdata-dt))];
    end
end

if(1) % abrupt freq changes w/ gaussian filter
    pt=[];
    for ii=1:numel(bits)
        pt=[pt bits(ii)*ones(1,1/(dt*fdata))];
    end
    dwdt=pt*wmod;
%     HG=gaussdesign(1,4,1/(dt*fdata));
%     dwdt=filtfilt(HG,1,dwdt);
end



%% generate signals

amp_noise=0*randn(size(t));
freq_noise=0*rand(size(t));

% pure tone
% s1=cos((wif+freq_noise).*t) + amp_noise; % pure tone at wif

% FSK signal: wif+/-wmod
% need to fix phase discontinuities

phicorr=2*pi*rand(1); % start with random phase
phi(1)=(wif+wmod*pt(1))*t(1) + phicorr(1) + freq_noise(1);

for ii=2:numel(t)
    % accumulate phase offset to correct phase discontinuities that would
    % otherwise result from changing frequency suddenly
    if (pt(ii)==-1 && pt(ii-1)==1) % 1 to 0 transition
        phicorr(ii)=phicorr(ii-1)+2*wmod*t(ii); % result of on-paper math to work
        % out how to get phases to line up with changing frequencies
    elseif(pt(ii)==1 && pt(ii-1)==-1) % 0 to 1 transition
        phicorr(ii)=phicorr(ii-1)-2*wmod*t(ii);
    else
        phicorr(ii)=phicorr(ii-1);
    end
    
    phi(ii)=(wif+wmod*pt(ii))*t(ii) + phicorr(ii) + freq_noise(ii);

end

dwdt_short=dwdt(numel(dwdt)-numel(t)+1:end); %trim a few off the front
% s1=cos(phi) + amp_noise;
s1=cos((wif+dwdt_short).*t+phicorr);
%% Weaver step 1: downconvert

idown=cos(1*wif*t);
qdown=cos(1*wif*t+pi/2);
s2=s1.*idown;
s3=s1.*qdown;

if(0) % double-sided FFT
%     figure; plot(t,s2);
    
    Fy=fft(s3);
    P1=abs(Fy/L);
    P2b=P1(1:floor(L/2)+1);
    P2a=P1(floor(L/2)+2:end);
    P3=[P2a P2b];
    figure; plot(f/1e6,10*log10(P3)); grid on; xlim([-10 10]);
end



%% Weaver step 2: filter

% Ideally we'd have a brick wall at wif so the upper mixing product is
% entirely cancelled.

% s4 and s5 are the waveforms we want to output to the function generator

lpf=fir1(500,wif*dt/pi);
%load lpf.mat;
s4=filtfilt(lpf,1,s2);
s5=filtfilt(lpf,1,s3);

if(0) % double-sided FFT
%     figure; plot(t,s5);
    
    Fy=fft(s4);
    P1=abs(Fy/L);
    P2b=P1(1:floor(L/2)+1);
    P2a=P1(floor(L/2)+2:end);
    P3=[P2a P2b]; % this line and the two above it are fftshift() oops
    figure; plot(f/1e6,10*log10(P3)); grid on; xlim([-20 20]);
end

%% Weaver step 3: upconvert & difference
% This is just to see what we should expect on the output of the Rohde; the
% Rohde will do this operation for us (mix, and subtract Q from I)

% we already have irf and qrf from the direct upconvert test section, so:
% s6=s4.*irf+s5.*qrf;

if(0) % double-sided FFT
    figure; plot(t,s6);
    
    Fy=fft(s6);
    P1=abs(Fy/L);
    P2b=P1(1:floor(L/2)+1);
    P2a=P1(floor(L/2)+2:end);
    P3=[P2a P2b];
    figure; plot(f/1e6,10*log10(P3)); grid on; %xlim([-20 20]);
end

I=s4-mean(s4);
Q=s5-mean(s5);

fullmemI=zeros(1,.5e6);
fullmemQ=zeros(1,.5e6);
fullmemI(1:numel(I))=I;
fullmemQ(1:numel(Q))=Q;
I=single(fullmemI);
Q=single(fullmemQ);

end

