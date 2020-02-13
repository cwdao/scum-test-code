// Raw Bit/Chip Shift Register
// Rev 2, Brad Wheeler, 6/25/18

// CLK/DATA from demodulator shifts one bit at a time into this register

// Updates to v1:
// Add start symbol register (set by analog_cfg) to help find where the data stream starts
// Add correlator to check Hamming distance against value set from analog_cfg
// Add second interrupt that goes off when current shift register is within target Hamming distance of start symbol
// Add syncronizers on interrupts to cortex and a signal for cortex to clear interrupts

// Behavior:
// Assert an interrupt every time 32 bit register gets full - latch it into another 32bit register to hold it for reading
// Can set a target 32-bit value to search for and assert an interrupt when found
// Can specify a hamming distance target for how well the input must match the target value
// When the target value is found, reset the 32-bit interrupt counter as well


// Instantiation template
//raw_chips_shift_register shift1 (
//    .bit_clk(bit_clk), 														// Demodulator clock output
//    .HCLK(HCLK), 																// Digital system HCLK for synchronizing interrupts
//    .rst_b(HARD_RESETn), 													// Digital system hard reset for clearing interrupts
//    .bit_in(bit_in), 															// Demodulator data output
//    .startval({analog_cfg[31:16],analog_cfg[15:0]}), 				// The 32-bit value to search for
//    .corr_threshold(analog_cfg[36:32]), 								// Hamming distance for how well input must match search string
//    .clear_interrupt32(analog_cfg[37]), 								// Clear interrupt signal from software
//    .clear_interrupt_startval(analog_cfg[38]), 						// Clear interrupt signal from software
//    .interrupt32(interrupt32), 											// Interrupt that goes off every 32bits of new input
//    .interrupt_startval(interrupt_startval), 							// Interrupt for when input is within threshold distance of target
//    .data_out({analog_rdata[31:16],analog_rdata[15:0]})			// 32bit output register for reading data by software
//    );

module raw_chips_shift_register(
	input bit_clk,
	input HCLK,
	input rst_b,
	input bit_in,
	input [31:0] startval,
	input [4:0] corr_threshold,
	input clear_interrupt32,
	input clear_interrupt_startval,
	output reg interrupt32,
	output reg interrupt_startval,
	output reg [31:0] data_out
	);

	// The input buffer and counter
	reg [31:0] buffer;
	reg [4:0] counter;
	
	// Internal interrupt signals before being syncronized to HCLK
	reg interrupt32_int, interrupt_startval_int;
	
	// Correlator
	reg [31:0] hamming_distance_xor;
	reg [5:0]  hamming_distance_sum;
	
	// Used for avoiding metastability and syncronizing interrupts to HCLK
	reg R1,R2,R4,R5;	
	
	// Must synchronize interrupts to HCLK
	// Assume reset is digital system soft reset which is syncrhonized to HCLK
	always @ (posedge HCLK) begin
		if(~rst_b) begin
			R1 <= 0;
			R2 <= 0;
		end
		else begin
			R1 <= interrupt32_int;
			R2 <= R1;
		end
	end
	
	// If shifter register interrupt goes off, keep it asserted until cleared by software
	always @(posedge HCLK) begin
		if(clear_interrupt32)
			interrupt32 <= 0;
		else if(R2)
			interrupt32 <= 1;
	end
	
	// Must synchronize interrupts to HCLK
	// Assume reset is digital system soft reset which is syncrhonized to HCLK
	always @ (posedge HCLK) begin
		if(~rst_b) begin
			R4 <= 0;
			R5 <= 0;
		end
		else begin
			R4 <= interrupt_startval_int;
			R5 <= R4;
		end
	end
	
	// If shifter register interrupt goes off, keep it asserted until cleared by software
	always @(posedge HCLK) begin
		if(clear_interrupt_startval)
			interrupt_startval <= 0;
		else if(R5)
			interrupt_startval <= 1;
	end
	
	// New bit comes in at MSB and gets shifted right
	always @ (posedge bit_clk or negedge rst_b) begin
		if (~rst_b) begin
			buffer <= 32'd0;
		end
		else 
			buffer <= {bit_in, buffer[31:1]};
	end

	// Reset is asynchronous to the bit clock	
	always @ (posedge bit_clk or negedge rst_b) begin
		if (~rst_b) begin
			interrupt32_int <= 1'd0;
			interrupt_startval_int <= 1'd0;
			counter <= 5'd0;
			data_out <= 32'd0;
		end
		// If match detector, assert interrupt_startval_int for 1 cycle and reset counter
		else if(hamming_distance_sum <= corr_threshold) begin
			interrupt32_int <= 1'd0;
			interrupt_startval_int <= 1'd1;
			data_out <= buffer;
			counter <= 5'd0;
		end
		// Otherwise assert interrupt32_int and reset counter every 32 bits of input
		else if (counter == 5'd31) begin
			interrupt32_int <= 1'd1;
			interrupt_startval_int <= 1'd0;
			data_out <= buffer;
			counter <= 5'd0;
		end
		// Else just increment the counter
		else begin
			interrupt32_int <= 1'b0;
			interrupt_startval_int <= 1'd0;
			counter <= counter + 1'd1;
		end
	end

	// Correlator to calculate the Hamming distance between the current input power and the target value
	always @(*) begin
		hamming_distance_xor = startval ^ buffer;
		hamming_distance_sum = 	  hamming_distance_xor[0]  + hamming_distance_xor[1]  + hamming_distance_xor[2]  + hamming_distance_xor[3]  +
								  hamming_distance_xor[4]  + hamming_distance_xor[5]  + hamming_distance_xor[6]  + hamming_distance_xor[7]  +
								  hamming_distance_xor[8]  + hamming_distance_xor[9]  + hamming_distance_xor[10] + hamming_distance_xor[11] +
								  hamming_distance_xor[12] + hamming_distance_xor[13] + hamming_distance_xor[14] + hamming_distance_xor[15] +
								  hamming_distance_xor[16] + hamming_distance_xor[17] + hamming_distance_xor[18] + hamming_distance_xor[19] +
								  hamming_distance_xor[20] + hamming_distance_xor[21] + hamming_distance_xor[22] + hamming_distance_xor[23] +
								  hamming_distance_xor[24] + hamming_distance_xor[25] + hamming_distance_xor[26] + hamming_distance_xor[27] +
								  hamming_distance_xor[28] + hamming_distance_xor[29] + hamming_distance_xor[30] + hamming_distance_xor[31];
	end

endmodule