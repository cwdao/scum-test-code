// Change Log
//  v9:
//  Fixed gpio issues: && and off by 1
//  Updated control for ANYOUT regulators to drive low/hi-Z
//  Slowed down 3wb bootloader and have it strobe hard reset first
//  Changed pin states on DSC pins to start hi-Z to avoid drive fights with gpio --untested

//  v10:
//  Added I2C back in.  Move i2c init calls to after the 3.3V regulator has been turned on

//  v11:
//  I2C changes

//  v12:
//  Added external interrupt strobe
//  Merge I2C changes
//  Add ability to pulse N clock cycles or output continuous 5MHz clock (reuse optical pin - 38)

//  v13:
//  Add modulation pin for LC

//  v14:
//  Only assert ASC_source_select when actually using scan chain, otherwise default it to internal
//  v16:
//  Add digital data output from matlab (for fpga, re-uses pins 33 and 34)
//  Change hard reset strobe time to 500 ms to support hard reset via teensy on fpga

//  v18:
//  Add delay during optical programming to allow for reset time
//  Add 4b5b version of optical programming

//  v19:
//  Add function for doing optical data transfer
//  Add function for optical boot without hard reset

//  v20:
//  Fixes to digital scan chain readout

//  SCM3B - v1 - Adapted from SCM3 teensy code
//  Remove board configuration functions
//  Add 4b/5b encoding step
//  Add CRC computation
//  Update pin mapping

//  SCM3B - v2
//  Add function to send optical SFDs at fixed interval for optical calibration

//  SCM3C - v3
//  Open-drain for hard reset to prevent SCM damage (that pin can't handle 3.3V)
//  Add support to 3wb for doing initial frequency calibration after programming


// PIN MAPPINGS (Teensy 3.6)
// ---------------
// digital data output
const int CLK_OUT = 0;
const int DATA_OUT = 1;

// 3wb bootloader
const int clkPin = 16;
const int enablePin = 15;
const int dataPin = 14;
const int hReset = 17;

const int sReset = 18;

// Optical bootloader
const int optical_out = 24;

// Digital Scan Chain (DSC)
const int dPHI = 32;
const int dPHIb = 31;
const int dSCANi0o1 = 30;
const int dSCANOUT = 29;
const int dSCANIN = 28;

// Analog Scan Chain (ASC)
const int asc_source_select = 33;
const int aPHI = 37;
const int aPHIb = 38;
const int aLOAD = 35;
const int aSCANIN = 34;
const int aSCANOUT = 36;

// Clock output
const int clock_out = 20;

// Variables for command interpreter
String inputString = "";
boolean stringComplete = false;

// Variables for copying binary bootload data to memory
int iindex;
byte ram[85000];

// Array to hold program data after 4B5B conversion
byte payload4b5b[85000];

// Optical programmer variables
int PREAMBLE_LENGTH = 100;  // How many 0x55 bytes to send to allow RX to settle
int REBOOT_BYTES = 200;    // How many 0x55 bytes to send while waiting for cortex to hard reset

int dig_data_bytes = 1500;//4*2000 + 1;

// Delay times for high/low, long/short optical pulses
int p1, p2, p3, p4;

// Runs once at power-on
void setup() {
  // Open USB serial port; baud doesn't matter; 12Mbps regardless of setting
  Serial.begin(9600);

  // Reserve 200 bytes for the inputString:
  inputString.reserve(200);

  // Setup pins for 3wb output
  pinMode(clkPin, OUTPUT); // CLK
  pinMode(enablePin, OUTPUT); // EN
  pinMode(dataPin, OUTPUT); // DATA

  // Setup misc pins
  //pinMode(hReset, OUTPUT);
  //digitalWrite(hReset, LOW);
  pinMode(hReset, INPUT);   //Hi-Z
  pinMode(asc_source_select, OUTPUT);
  digitalWrite(asc_source_select, LOW);

  pinMode(sReset, OUTPUT);
  digitalWrite(sReset, HIGH);

  // Setup pins for digital scan chain
  // Leave these high-Z until needed or else will fight GPIO
  pinMode(dPHI, INPUT);
  pinMode(dPHIb, INPUT);
  pinMode(dSCANi0o1, INPUT);
  pinMode(dSCANOUT, INPUT);
  pinMode(dSCANIN, INPUT);



  // Setup pins for analog scan chain
  pinMode(aPHI, OUTPUT);
  pinMode(aPHIb, OUTPUT);
  pinMode(aLOAD, OUTPUT);
  pinMode(aSCANIN, OUTPUT);
  pinMode(aSCANOUT, INPUT);
  digitalWrite(aPHI, LOW);
  digitalWrite(aPHIb, LOW);
  digitalWrite(aLOAD, LOW);
  digitalWrite(aSCANIN, LOW);

  // Pin for optical TX
  pinMode(optical_out, OUTPUT);
  digitalWrite(optical_out, LOW);

}


// Runs repeatedly
// Monitors uart for commands and then makes function call
void loop() {
  // Do nothing until a '\n' terminated string is received
  if (stringComplete) {

    if (inputString == "transfersram\n") {
      transfer_sram();
    }

    else if (inputString == "insertcrc\n") {
      insert_crc();
    }

    else if (inputString == "transfersram4b5b\n") {
      transfer_sram_4b5b();
    }

    else if (inputString == "transfersdigdata\n") {
      transfer_dig_data();
    }

    else if (inputString == "digdata\n") {
      output_digital_data();
    }

    else if (inputString == "transmit_chips\n") {
      transmit_chips();
    }

    else if (inputString == "boot3wb\n") {
      bootload_3wb();
    }

    else if (inputString == "boot3wb_debug\n") {
      bootload_3wb_debug();
    }

    else if (inputString == "boot3wb4b5b\n") {
      bootload_3wb_4b5b();
    }

    else if (inputString == "optdata\n") {
      optical_data_transfer(p1, p2, p3, p4);
    }

    else if (inputString == "bootopt4b5b\n") {
      bootload_opt_4b5b(p1, p2, p3, p4);
    }

    else if (inputString == "bootopt4b5bnorst\n") {
      bootload_opt_4b5b_no_reset(p1, p2, p3, p4);
    }

    else if (inputString == "configopt\n") {
      optical_config();
    }

    else if (inputString == "opti_cal\n") {
      opti_cal();
    }

    else if (inputString == "3wb_cal\n") {
      cal_3wb();
    }

    else if (inputString == "encode4b5b\n") {
      encode_4b5b();
    }

    else if (inputString == "ascwrite\n") {
      asc_write();
    }

    else if (inputString == "ascload\n") {
      asc_load();
    }

    else if (inputString == "ascread\n") {
      asc_read();
    }

    else if (inputString == "toggle3wbclk\n") {
      toggle_3wb_enable();
    }

    else if (inputString == "dscread\n") {
      dsc_read();
    }
    
    else if (inputString == "dscdebug\n") {
      dsc_debug();
    }
    
    else if (inputString == "stepclk\n") {
      step_clock();
    }

    else if (inputString == "clockon\n") {
      clock_on();
    }

    else if (inputString == "clockoff\n") {
      clock_off();
    }

    else if (inputString == "togglehardreset\n") {
      togglehardreset();
    }

    // Reset to listen for a new '\n' terminated string over serial
    inputString = "";
    stringComplete = false;
  }
}

unsigned int parse_bindata_payload(char* payload, int payload_size, unsigned int sram_write_head) {
  char hexh[3] = {0}; // Add null terminator
  char* endptr;
  byte byte_value;
  // Process the hex data
  for (int i = 0; i < payload_size; i++) {
    hexh[0] = payload[i * 2];
    hexh[1] = payload[i * 2 + 1];
    hexh[2] = '\0';

    byte_value = (byte)strtoul(hexh, &endptr, 16);

    ram[sram_write_head++] = (byte)byte_value;
  }

  // debug: print back the byte array
  // for (int i = 0; i < payload_size; i++) {
  //   Serial.print(ram[sram_write_head - payload_size + i], HEX);
  //   Serial.print(" ");
  // }
  // Serial.println(); 
  return sram_write_head;
}

// Buffers can be any byte length, we need to loop over the buffer, find the preamble
// Then pass the packet to parse_bindata_payload

// Parses the received buffer from UART, writes to SRAM, and returns the new SRAM write head
// Packets are in the format
// Preamble: "T" "s"
// Length: 6 digits
// Hex data: 2 hex digits per byte
// End of packet: "\n"
unsigned int parse_bindata_buffer(char* buffer, int buffer_size, unsigned int sram_write_head ) {
  char* packet_head = buffer;

  unsigned int old_sram_write_head = sram_write_head;
  
  while (packet_head < buffer + buffer_size) {
    unsigned int length;
    // Check if the packet starts with "Ts"
    unsigned int offset = 2;
    if (packet_head[0] != 'T' || packet_head[1] != 's') {
      // We can try to recover from the missing first byte
      offset = 1;
      if(packet_head[0] != 's') {
        Serial.println("Error: Invalid packet format");
        return sram_write_head;
      }
    }

    // Extract the length from the next 6 characters
    char length_str[7];
    char* endptr;
    strncpy(length_str, packet_head + offset, 6);
    length_str[6] = '\0';
    length = (unsigned int)strtoul(length_str, &endptr, 10);

    // Validate that the length is valid and doesnt exceed our buffer size
    if (endptr != length_str + 6 || length == 0) {
      Serial.println("Error: Invalid length");
      return sram_write_head;
    }
   
    // Now its safe to hand off the payload to parse_bindata_payload
    unsigned int tmp = parse_bindata_payload(packet_head + 6 + offset, length, sram_write_head);
    if(tmp == sram_write_head) {
      return old_sram_write_head;
    }
    sram_write_head = tmp;

    // Check for the newline character at the end
    if (packet_head[(6 + offset) + length * 2] != '\n') {
      Serial.println("Error: Missing newline at end of packet");
      Serial.flush();
      return sram_write_head;
    }

    // Move the packet head forward by the length of the packet + newline
    packet_head += (6+offset) + length * 2 + 1;

    Serial.print("Processed ");
    Serial.print(length);
    Serial.println(" bytes");

  }

  return sram_write_head;
}

uint32_t calculate_crc32(uint8_t* ram) {
  // Calculate CRC32 over the SRAM contents
  uint32_t crc = 0xFFFFFFFF;
  for (int i = 0; i < 65536; i++) {
    uint8_t byte = ram[i];
    crc ^= byte;
    for (int j = 0; j < 8; j++) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xEDB88320;
      else
        crc = crc >> 1;
    }
  }
  return ~crc;
}

void transfer_sram() {
  Serial.println("SRAM Transfer - SCM3B Rev 2");
  Serial.send_now();

  // Look for \r\n to indicate we are ready to receive data
  while((char)Serial.read() != '\r');
  while((char)Serial.read() != '\n');
  Serial.println("newline received!");

  int doneflag = 0;
  iindex = 0;

  const int chunk_size = 1024;
  const int buffer_size = chunk_size * 2; // 128 bytes * 2 hex chars + \r\n
  char buffer[buffer_size] = {0}; // Buffer to hold chunk_size bytes * 2 hex chars + \r\n
  int buffer_ptr = 0;

  // Loop until entire 64kB received over serial
  while (!doneflag) {
    // Retrieve SRAM contents over serial
    if (Serial.available()) {
      while (Serial.available()) {
        char val = Serial.read();
        buffer[buffer_ptr++] = val;
      }

      // print out the buffer as a string of characters
      for (int i = 0; i < buffer_ptr; i++) {
        Serial.print(buffer[i]);
      }
      Serial.println();

      // Now process the packet
      int tmp  = parse_bindata_buffer(buffer, buffer_ptr, iindex);
      buffer_ptr = 0;
      if(tmp == iindex) 
        continue;
      iindex = tmp;

      Serial.print("OK ");
      Serial.println(iindex);
      Serial.send_now();
    }

    if (iindex == 65536) {
      doneflag = 1;
      Serial.println("SRAM Transfer Complete");
    }
  }
  // Calculate CRC by passing back over sram array, send that CRC 


  uint32_t crc = calculate_crc32(ram);

  // Send the calculated CRC over serial
  Serial.print("CRC: 0x");
  Serial.println(crc, HEX);
}

void transfer_sram_4b5b() {
  Serial.println("Executing SRAM Transfer");
  int doneflag = 0;
  iindex = 0;




  while (!doneflag) {

    // Retrieve SRAM contents over serial
    if (Serial.available()) {
      ram[iindex] = (byte)Serial.read();
      iindex++;
    }

    if (iindex == 81920)  {
      doneflag = 1;
      Serial.println("SRAM Transfer Complete");
    }
  }


  // For checking if Teensy-based 4b5b encoding matches matlab
  //  for (int i = 0; i < 81920; i++) {
  //    if (ram[i] != payload4b5b[i]) {
  //      Serial.println("ram = " + String(ram[i]) + ", 4b5b = " + String(payload4b5b[i]) + ", i =" + String(i));
  //    }
  //  }
  //Serial.println("OK");




}


// Transmits 32 bits over 3wb for debugging
void bootload_3wb_debug()  {
  //Serial.println("Executing 3wb Bootload");

  //ram[0] = 0;
  //ram[1] = 255;
  //ram[2] = 0;
  //ram[3] = 255;

  int xx = 64*1024;

  // Need to send at least 64*1024 bytes to get cortex to reset
  for (int i = 1; i < (xx+1); i++) {

    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, (ram[i - 1] >> j) & 1);
      delayMicroseconds(1);

      // Every 32 bits need to strobe the enable high for one cycle
      if ((i % 4 == 0) && (j == 7)) {
        digitalWrite(enablePin, 1);
      }
      else {
        digitalWrite(enablePin, 0);
      }

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }
Serial.println("3WB Debug Complete");
}



void transfer_dig_data() {
  //Serial.println("Executing Digital Data Transfer");
  int doneflag = 0;
  iindex = 0;

  // Loop until entire data set received over serial
  while (!doneflag) {

    // Retrieve SRAM contents over serial
    if (Serial.available()) {
      ram[iindex] = (byte)Serial.read();
      iindex++;
    }

    if (iindex == dig_data_bytes)  {
      doneflag = 1;
      //Serial.println("Digital Data Transfer Complete");
    }
  }
}

// Call this function to configure pulse widths for optical TX
void optical_config() {

  //Serial.println("Calling optical config");

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }

  p1 = inputString.toInt();
  //Serial.println("Received " + String(p1));

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }
  p2 = inputString.toInt();
  //Serial.println("Received " + String(p2));

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }
  p3 = inputString.toInt();
  //Serial.println("Received " + String(p3));

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }
  p4 = inputString.toInt();
  //Serial.println("Received "  + String(p4));

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;
}


void bootload_opt_4b5b(int p1, int p2, int p3, int p4) {

#define NOP1 "nop\n\t"

  int bitmask = 0x01;
  noInterrupts();
  int xx;

  int start_symbol[4] = {169, 176, 167, 50};
  int start_symbol_without_hard_reset[4] = {184, 84, 89, 40};
  int preamble = 85;

  // Send Preamble of 101010 to allow optical RX to settle
  for (int i = 0; i < PREAMBLE_LENGTH; i++) {
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (preamble & (bitmask << j)) {

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send start symbol to intiate hard reset
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (start_symbol[i] & (bitmask << j)) {

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send REBOOT_BYTES worth of preamble to give cortex time to do hard reset
  // The REBOOT_BYTES variable must be the same value as in the verilog
  for (int i = 0; i < REBOOT_BYTES; i++) {
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (preamble & (bitmask << j)) {

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send program data encoded in 4b5b format
  for (int i = 0; i < 81920; i++) {
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (payload4b5b[i] & (bitmask << j)) {

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Need to send 5 extra bits to clock last value through
//  for (int j = 0; j < 6; j++) {
  for (int j = 0; j < 600; j++) {
    digitalWriteFast(optical_out, HIGH);
    for (xx = 0; xx < p1; xx++) {
      __asm__(NOP1);
    }
    digitalWriteFast(optical_out, LOW);
    for (xx = 0; xx < p2; xx++) {
      __asm__(NOP1);
    }
  }

  interrupts();
  Serial.println("Optical Boot Complete");
}




void bootload_opt_4b5b_no_reset(int p1, int p2, int p3, int p4) {

#define NOP1 "nop\n\t"

  int bitmask = 0x01;
  noInterrupts();
  int xx;

  int start_symbol[4] = {169, 176, 167, 50};
  int start_symbol_without_hard_reset[4] = {184, 84, 89, 40};
  int preamble = 85;

  // Send Preamble of 101010 to allow optical RX to settle
  for (int i = 0; i < PREAMBLE_LENGTH; i++) {
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (preamble & (bitmask << j)) {

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send start symbol to intiate boot without hard reset
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (start_symbol_without_hard_reset[i] & (bitmask << j)) {

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send program data encoded in 4b5b format
  for (int i = 0; i < 81920; i++) {
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (payload4b5b[i] & (bitmask << j)) {

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Need to send 5 extra bits to clock last value through
  for (int j = 0; j < 6; j++) {

    digitalWriteFast(optical_out, HIGH);
    for (xx = 0; xx < p1; xx++) {
      __asm__(NOP1);
    }
    digitalWriteFast(optical_out, LOW);
    for (xx = 0; xx < p2; xx++) {
      __asm__(NOP1);
    }
  }

  interrupts();
  Serial.println("Optical Boot Complete - No Reset");
}


void optical_data_transfer(int p1, int p2, int p3, int p4) {

#define NOP1 "nop\n\t"

  int bitmask = 0x01;
  noInterrupts();
  int xx;

  int sfd[4] = {221, 176, 231, 47};
  int preamble = 85;

  // Send Preamble of 101010 to allow optical RX to settle
  for (int i = 0; i < PREAMBLE_LENGTH; i++) {
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (preamble & (bitmask << j)) {

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send start symbol to throw interrupt for aligning data
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (sfd[i] & (bitmask << j)) {

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // The very first data bit gets lost so send it twice to re-align
  if (ram[0] & (bitmask)) {

    digitalWriteFast(optical_out, HIGH);
    for (xx = 0; xx < p1; xx++) {
      __asm__(NOP1);
    }
    digitalWriteFast(optical_out, LOW);
    for (xx = 0; xx < p2; xx++) {
      __asm__(NOP1);
    }
  }
  else {    // data = 0, short pulse

    digitalWriteFast(optical_out, HIGH);
    for (xx = 0; xx < p3; xx++) {
      __asm__(NOP1);
    }
    digitalWriteFast(optical_out, LOW);
    for (xx = 0; xx < p4; xx++) {
      __asm__(NOP1);
    }
  }



  // Send payload in raw binary (no 4B5B done in hardware)
  for (int i = 0; i < dig_data_bytes; i++) {
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (ram[i] & (bitmask << j)) {

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(optical_out, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(optical_out, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }


  interrupts();
}



void output_digital_data() {

#define NOP7 "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
#define NOP10 "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"

  int bitmask = 0x01;
  noInterrupts();
  int xx;

  for (int i = 0; i < dig_data_bytes; i++) {
    for (int j = 0; j < 8; j++) {

      if (ram[i] & (bitmask << j)) {
        digitalWriteFast(DATA_OUT, HIGH);
      }
      else {
        digitalWriteFast(DATA_OUT, LOW);
      }

      // Toggle clock
      delayMicroseconds(10);
      // __asm__(NOP7);
      digitalWriteFast(CLK_OUT, HIGH);
      delayMicroseconds(10);
      // __asm__(NOP7);
      digitalWriteFast(CLK_OUT, LOW);
      delayMicroseconds(10);
    }
  }
  interrupts();
  Serial.println("Digital data output complete.");
}


// Receives 1200 analog scan chain bits over serial
// Then bitbangs them out to ASC
void asc_write() {
  Serial.println("Executing ASC Write");
  int count = 0;
  char scanbits[1200];

  // Set scan chain source as external
  digitalWrite(asc_source_select, HIGH);

  // Loop until all 1200 scan chain bits received over serial
  while (count != 1200) {

    // Read one bit at a time over serial
    if (Serial.available()) {
      scanbits[count] = Serial.read();
      count++;
    }
  }

  // Once all bits are received, bitbang to ASC input
  for (int x = 0; x < 1200; x++) {
    if (scanbits[x] == '1') {
      digitalWrite(aSCANIN, HIGH);
    }
    else if (scanbits[x] == '0') {
      digitalWrite(aSCANIN, LOW);
    }
    else {
      // There was an error reading in the bits over uart
      Serial.println("Error in ASC Write");
    }
    // Pulse PHI and PHIB
    atick();
  }
  Serial.println("ASC Write Complete");
}

// Latches the loaded data to the outputs of the scan chain
// Must call asc_write() before asc_load()
void asc_load() {
  Serial.println("Executing ASC Load");
  digitalWrite(aLOAD, HIGH);
  digitalWrite(aLOAD, LOW);

  // Set scan chain source back to internal
  delayMicroseconds(1);
  digitalWrite(asc_source_select, LOW);
}


// Transmits 64kB binary over wired 3wb
void bootload_3wb_4b5b()  {
  Serial.println("Executing 3wb Bootload");

  int start_symbol[4] = {169, 176, 167, 50};
  int preamble = 85;

  // Send start symbol
  for (int i = 0; i < 4; i++) {
    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, (start_symbol[i] >> j) & 1);

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }

  // Send dummy bytes to wait for reset to finish
  for (int i = 0; i < REBOOT_BYTES; i++) {
    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, (preamble >> j) & 1);

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }

  // Need to send at least 64*1024 bytes to get cortex to reset
  for (int i = 1; i < 81921; i++) {
    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, (ram[i - 1] >> j) & 1);
      delayMicroseconds(1);

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }

  // Send 5 extra bits to cycle last 4b5b value through
  for (int j = 0; j < 6; j++) {
    digitalWrite(dataPin, 0);

    // Toggle the clock
    delayMicroseconds(1);
    digitalWrite(clkPin, 1);
    delayMicroseconds(1);
    digitalWrite(clkPin, 0);
    delayMicroseconds(1);
  }


}

void toggle_3wb_enable()  {
  for (int i = 1; i < 2; i++){
    // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(enablePin, 1);
      digitalWrite(dataPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(enablePin, 0);
      digitalWrite(clkPin, 0);
      digitalWrite(dataPin, 0);
      //delayMicroseconds(20);
  }
}

// Transmits 64kB binary over wired 3wb
void bootload_3wb()  {
  //Serial.println("Executing 3wb Bootload");

  // Execute hard reset
  pinMode(hReset, OUTPUT);  //Drive low
  delayMicroseconds(500);
  //delay(500);
  pinMode(hReset, INPUT);   //Back to Hi-Z
  delayMicroseconds(500);

  // Need to send at least 64*1024 bytes to get cortex to reset
  for (int i = 1; i < 65537; i++) {

    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, (ram[i - 1] >> j) & 1);
      delayMicroseconds(1);

      // Every 32 bits need to strobe the enable high for one cycle
      if ((i % 4 == 0) && (j == 7)) {
        digitalWrite(enablePin, 1);
      }
      else {
        digitalWrite(enablePin, 0);
      }

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }

  //  // Execute soft reset
  //  digitalWrite(sReset, LOW);
  //  delay(20);
  //  digitalWrite(sReset, HIGH);
  //  delay(20);

  for (int i = 1; i < 10000; i++) {

    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, 1);
      delayMicroseconds(1);

      // Every 32 bits need to strobe the enable high for one cycle
      if ((i % 4 == 0) && (j == 7)) {
        digitalWrite(enablePin, 1);
      }
      else {
        digitalWrite(enablePin, 0);
      }

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }


  Serial.println("3WB Bootload Complete");
}


// Transmits chips at near 2Mcps
void transmit_chips()  {

#define NOP1 "nop\n\t"

  int lpval = 5;
  int xx;

  Serial.println("Executing Raw Chip Output");

  for (int i = 1; i < dig_data_bytes; i++) {

    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWriteFast(DATA_OUT, (ram[i - 1] >> j) & 1);

      // Toggle the clock
      for (xx = 0; xx < lpval; xx++) {
        __asm__(NOP1);
      }
      digitalWriteFast(CLK_OUT, 1);
      for (xx = 0; xx < lpval; xx++) {
        __asm__(NOP1);
      }
      digitalWriteFast(CLK_OUT, 0);
      for (xx = 0; xx < lpval; xx++) {
        __asm__(NOP1);
      }
    }
  }
}


//10 KHz clock, 40 percent duty cycle
void dtick() {
  delayMicroseconds(1);
  digitalWrite(dPHI, HIGH);
  delayMicroseconds(1);

  //Read
  digitalWrite(dPHI, LOW);
  delayMicroseconds(1);

  //Shift
  digitalWrite(dPHIb, HIGH);
  delayMicroseconds(1);
  digitalWrite(dPHIb, LOW);
  delayMicroseconds(1);


}

void dsc_debug() {
  // Setup pins for digital scan chain
  pinMode(dPHI, OUTPUT);
  pinMode(dPHIb, OUTPUT);
  pinMode(dSCANi0o1, OUTPUT);
  pinMode(dSCANIN, OUTPUT);
  digitalWrite(dPHI, HIGH);
  digitalWrite(dPHIb, HIGH);
  digitalWrite(dSCANi0o1, LOW);   //stay low until ready
  digitalWrite(dSCANIN, LOW);
}


//Note that Serial.println has a 697 char (699 out) max.
//2157 = 600 + 600 + 600 + 357
void dsc_read() {
  //Serial.println("Executing DSC Read");
  String st = "";
  int val = 0;

  // Setup pins for digital scan chain
  pinMode(dPHI, OUTPUT);
  pinMode(dPHIb, OUTPUT);
  pinMode(dSCANi0o1, OUTPUT);
  pinMode(dSCANIN, OUTPUT);
  digitalWrite(dPHI, LOW);
  digitalWrite(dPHIb, LOW);
  digitalWrite(dSCANi0o1, LOW);   //stay low until ready
  digitalWrite(dSCANIN, LOW);
  delay(1);

  digitalWrite(dSCANi0o1, HIGH);
  dtick();
  digitalWrite(dSCANi0o1, LOW);
  //First bit should be available
  val = digitalRead(dSCANOUT);
  if (val)
    st = String(st + "1");
  else
    st = String(st + "0");

  //2-600
  for (int i = 1; i < 600 ; i = i + 1) {
    dtick();
    val = digitalRead(dSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

   //digitalWrite(dSCANIN, HIGH);

  //601-1200
  for (int i = 0; i < 600 ; i = i + 1) {
    dtick();
    val = digitalRead(dSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";


  //1201-1800
  for (int i = 0; i < 600 ; i = i + 1) {
    dtick();
    val = digitalRead(dSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  //digitalWrite(dSCANIN, LOW);


  //1801-2157
  for (int i = 0; i < 357 ; i = i + 1) {
    dtick();
    val = digitalRead(dSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  Serial.println(); //Terminator

  // Set pins back to hi-Z
  pinMode(dPHI, INPUT);
  pinMode(dPHIb, INPUT);
  pinMode(dSCANi0o1, INPUT);
  pinMode(dSCANIN, INPUT);

  return;
}

//10 KHz clock, 40 percent duty cycle
void atick() {
  digitalWrite(aPHI, LOW);
  delayMicroseconds(10);

  //Shift
  digitalWrite(aPHIb, HIGH);
  delayMicroseconds(40);
  digitalWrite(aPHIb, LOW);
  delayMicroseconds(10);

  //Read
  digitalWrite(aPHI, HIGH);
  delayMicroseconds(40);
}

//Note that Serial.println has a 697 char (699 out) max.
//1200 = 600 + 600
void asc_read() {
  //Serial.println("Executing ASC Read");
  String st = "";
  int val = 0;

  // Set scan chain source as external
  digitalWrite(asc_source_select, HIGH);

  //digitalWrite(aSCANi0o1, HIGH);
  //atick();
  //digitalWrite(aSCANi0o1, LOW);
  //First bit should be available
  val = digitalRead(aSCANOUT);
  if (val)
    st = String(st + "1");
  else
    st = String(st + "0");

  //2-600
  for (int i = 1; i < 600 ; i = i + 1) {
    atick();
    val = digitalRead(aSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  //601-1200
  for (int i = 0; i < 600 ; i = i + 1) {
    atick();
    val = digitalRead(aSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  Serial.println(); //Terminator

  // Set scan chain source back to internal
  digitalWrite(asc_source_select, LOW);

  return;
}


// Toggle pin a specified number of times for use as clock when debugging
// Note that the period of this clock is < 5MHz (measured 491.8kHz on scope) (when delayu = 1)
void step_clock() {
  // Read number of clock ticks to output
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }

  int num_ticks = inputString.toInt();
  //pinMode(optical_out, OUTPUT);

  pinMode(clock_out, OUTPUT);

  for (int i = 0; i < num_ticks ; i = i + 1) {
    digitalWrite(clock_out, HIGH);
    delayMicroseconds(10);
    digitalWrite(clock_out, LOW);
    delayMicroseconds(10);
  }
}

// Output a 100kHz clock
void clock_on() {
  analogWrite(clock_out, 128);
  analogWriteFrequency(clock_out, 10000);
  analogWrite(clock_out, 128);
}

// Turn off clock on pin optical_out
void clock_off() {
  pinMode(clock_out, OUTPUT);
  digitalWrite(clock_out, LOW);
}


// Toggle hard reset low then high
void togglehardreset() {
  pinMode(clock_out, OUTPUT);
  //digitalWrite(hReset, LOW);
  pinMode(hReset, OUTPUT);
  delay(10);
    digitalWrite(clock_out, HIGH);
    delayMicroseconds(10);
    digitalWrite(clock_out, LOW);
    delayMicroseconds(10);
  //digitalWrite(hReset, HIGH);
  pinMode(hReset, INPUT); //Back to Hi-Z
}

// For timing transfer after 3wb programming
// Trigger an interrupt at 100ms intervals
// clk_3wb comes out GPO<8> for debug visibility
// GPI<8> is connected to EXT_INTERRUPT<1> which is active high
// So to calibrate, after booting the software should enable this interrupt,
// set group 3 GPI input to bank 1 and enable GPI<8>
// When the cal_3wb function is called, it will pulse clk_3wb every 100ms to execute the ISR in software
// 30 pulses in total will be sent
void cal_3wb() {

  int jj;

  for(jj=0; jj<30; jj++){
  
    digitalWriteFast(clkPin,HIGH);
    delayMicroseconds(1); //Pulse stays high long enough to trigger interrupt (at least one HCLK cycle)
    digitalWriteFast(clkPin,LOW);

    // These values adjusted by measuring interrupt rate on scope
    delay(99);
    delayMicroseconds(995);
 
  }
}

// For timing transfer via optical
// Send optical SFDs at 100ms intervals
void opti_cal() {

    for (int ii = 0; ii < 50; ii++) {
    
  #define NOP1 "nop\n\t"
  
    int bitmask = 0x01;
    noInterrupts();
    int xx;
  
    int sfd[4] = {221, 176, 231, 47};
    int preamble = 85;
  
    // Send Preamble of 101010 to allow optical RX to settle
    for (int i = 0; i < PREAMBLE_LENGTH; i++) {
      for (int j = 0; j < 8; j++) {
  
        //data = 1, long pulse
        if (preamble & (bitmask << j)) {
  
          digitalWriteFast(optical_out, HIGH);
          for (xx = 0; xx < p1; xx++) {
            __asm__(NOP1);
          }
          digitalWriteFast(optical_out, LOW);
          for (xx = 0; xx < p2; xx++) {
            __asm__(NOP1);
          }
        }
        else {    // data = 0, short pulse
  
          digitalWriteFast(optical_out, HIGH);
          for (xx = 0; xx < p3; xx++) {
            __asm__(NOP1);
          }
          digitalWriteFast(optical_out, LOW);
          for (xx = 0; xx < p4; xx++) {
            __asm__(NOP1);
          }
        }
      }
    }
  
    // Send start symbol to throw interrupt 
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 8; j++) {
  
        //data = 1, long pulse
        if (sfd[i] & (bitmask << j)) {
  
          digitalWriteFast(optical_out, HIGH);
          for (xx = 0; xx < p1; xx++) {
            __asm__(NOP1);
          }
          digitalWriteFast(optical_out, LOW);
          for (xx = 0; xx < p2; xx++) {
            __asm__(NOP1);
          }
        }
        else {    // data = 0, short pulse
  
          digitalWriteFast(optical_out, HIGH);
          for (xx = 0; xx < p3; xx++) {
            __asm__(NOP1);
          }
          digitalWriteFast(optical_out, LOW);
          for (xx = 0; xx < p4; xx++) {
            __asm__(NOP1);
          }
        }
      }
    }

      // Need to send some extra bits to clock last value through
  for (int j = 0; j < 50; j++) {

    digitalWriteFast(optical_out, HIGH);
    for (xx = 0; xx < p1; xx++) {
      __asm__(NOP1);
    }
    digitalWriteFast(optical_out, LOW);
    for (xx = 0; xx < p2; xx++) {
      __asm__(NOP1);
    }
  }

    // Adjusted by measuring interrupt rate on scope
    //delay(96);
    //delayMicroseconds(877);
    delay(96);
    delayMicroseconds(910);
  }
}

// First use transfer_sram() to copy 64kB payload into Teensy SRAM variable ram[]
// Then call insert_crc() if desired
// This function will then apply 4B5B encoding and store the new payload in payload4b5b[]
// This conversion has been validated against the matlab-based conversion and gives the same result
// BW 11/17/18
void encode_4b5b() {

  byte converted_nibbles[8];

  // Process input binary data 4 bytes at a time
  for (int ii = 0; ii < 16384 ; ii = ii + 1) {

    // Convert the 4-bit MSB/LSB chunks of the 4 bytes into 8 5-bit encoded values
    for (int jj = 0; jj < 4 ; jj = jj + 1) {

      //MSBs
      converted_nibbles[2 * jj + 1] = LUT_4B5B(ram[ii * 4 + jj] >> 4);

      //LSBs
      converted_nibbles[2 * jj] = LUT_4B5B(ram[ii * 4 + jj] & 0xF);

    }

    // The 8 chunks of 5 bits are then turned back into 5 bytes
    payload4b5b[ii * 5 + 0]  = converted_nibbles[0] | converted_nibbles[1] << 5;
    payload4b5b[ii * 5 + 1] = converted_nibbles[1] >> 3 | converted_nibbles[2] << 2 | converted_nibbles[3] << 7;
    payload4b5b[ii * 5 + 2] = converted_nibbles[3] >> 1 | converted_nibbles[4] << 4;
    payload4b5b[ii * 5 + 3] = converted_nibbles[4] >> 4 | converted_nibbles[5] << 1 | converted_nibbles[6] << 6;
    payload4b5b[ii * 5 + 4] = converted_nibbles[6] >> 2 | converted_nibbles[7] << 3;
  }

  // For debug printing
  //  for (byte ii = 0; ii < 15 ; ii = ii + 1) {
  //    Serial.println(payload4b5b[ii]);
  //  }
  // Serial.println("Encoding Done");

}

// Look-up table for doing 4bit to 5bit mapping
byte LUT_4B5B(byte in) {
  if (in == 0) return 0x1E;
  if (in == 1) return 0x09;
  if (in == 2) return 0x14;
  if (in == 3) return 0x15;

  if (in == 4) return 0x0A;
  if (in == 5) return 0x0B;
  if (in == 6) return 0x0E;
  if (in == 7) return 0x0F;

  if (in == 8) return 0x12;
  if (in == 9) return 0x13;
  if (in == 10) return 0x16;
  if (in == 11) return 0x17;

  if (in == 12) return 0x1A;
  if (in == 13) return 0x1B;
  if (in == 14) return 0x1C;
  if (in == 15) return 0x1D;
}


// Reverses (reflects) bits in a 32-bit word.
unsigned reverse(unsigned x) {
  x = ((x & 0x55555555) <<  1) | ((x >>  1) & 0x55555555);
  x = ((x & 0x33333333) <<  2) | ((x >>  2) & 0x33333333);
  x = ((x & 0x0F0F0F0F) <<  4) | ((x >>  4) & 0x0F0F0F0F);
  x = (x << 24) | ((x & 0xFF00) << 8) |
      ((x >> 8) & 0xFF00) | (x >> 24);
  return x;
}

// Same C function used for calculating CRC on SCM
unsigned int crc32c(unsigned int length) {
  int i, j;
  unsigned int byte, crc;

  i = 0;
  crc = 0xFFFFFFFF;
  while (i < length) {
    byte = ram[i];            // Get next byte.
    byte = reverse(byte);         // 32-bit reversal.
    for (j = 0; j <= 7; j++) {    // Do eight times.
      if ((int)(crc ^ byte) < 0)
        crc = (crc << 1) ^ 0x04C11DB7;
      else crc = crc << 1;
      byte = byte << 1;          // Ready next msg bit.
    }
    i = i + 1;
  }
  return reverse(~crc);
}

// First use transfer_sram() to copy 64kB payload into Teensy SRAM variable ram[]
// The code length must already be inserted at memory address 0xFFF8 by bootloader script
// This function calculates the CRC over the code length and stores it at 0xFFFC
// SCM C code must also be set up for doing CRC check or else don't call this function
void insert_crc() {

  unsigned int code_length, calculated_crc;

  // Extract code length from address 0xFFF8
  code_length = 256 * ram[65529] + ram[65528];

  // Calculate CRC value
  calculated_crc = crc32c(code_length);

  // Store CRC in binary at location 0xFFFC
  ram[65535] = (calculated_crc & 0xFF000000) >> 24;
  ram[65534] = (calculated_crc & 0x00FF0000) >> 16;
  ram[65533] = (calculated_crc & 0x0000FF00) >> 8;
  ram[65532] =  calculated_crc & 0x000000FF;

  // For debugging
  //Serial.println("Code length = " + String(code_length) + " -- " + String(ram[65529]) + " - " + String(ram[65528]));
  //Serial.println("Calculated CRC = " + String(calculated_crc));
}


/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  if (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}