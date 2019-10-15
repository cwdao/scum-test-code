import numpy as np
import re
import serial
import signal
import sys
import time

def measure_counters(scm_ser, temp_ser, counts_2M, counts_32k, temperatures):
	"""
	Inputs:
		scm_ser: String. Serial to SCM over UART.
		temp_ser: String. Serial to temperature sensor.
		counts_2M: Array. Stores the 2M counts.
		counts_32k: Array. Stores the 32k counts.
		temperatures: Array. Stores the temperatures.
	Outputs:
		No return value. Reads the counters and temperature into a file.
	"""
	while True:
		counter_data = scm_ser.readline().decode().rstrip()
		counts = counter_data.split()

		if counter_data and len(counts) == 4:
			count_2M = re.sub(r"[^\d]", "", counts[1])
			counts_2M.append(int(count_2M))

			count_32k = re.sub(r"[^\d]", "", counts[3])
			counts_32k.append(int(count_32k))

			# Throw away first temperature reading
			temp_ser.write(b"temp\n")
			temp_ser.readline()
			temp_ser.write(b"temp\n")
			temp_data = temp_ser.readline().decode().rstrip()

			temperatures.append(float(temp_data))

			print("Read counters and temperature, temp=" + temp_data)

if __name__ == "__main__":
	scm_port = "COM12"
	temp_port = "COM11"
	filename = "counter_temp_data"

	counts_2M, counts_32k, temperatures = [], [], []

	def close(sig, frame):
		# Close serial ports
		scm_ser.close()
		temp_ser.close()

		# Save arrays to file
		filename_time = filename + "_" + time.strftime("%Y%m%d_%H%M%S")
		np.savez(filename_time, counts_2M=counts_2M, counts_32k=counts_32k, temperature=temperatures)
		print("Saved to " + filename_time + ".npz")

	# Open UART connection to SCM
	scm_ser = serial.Serial(
		port=scm_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=.5)

	# Open COM port to temperature sensor
	temp_ser = serial.Serial(
		port=temp_port,
		baudrate=19200)

	# Register SIGINT handler
	signal.signal(signal.SIGINT, close)

	measure_counters(
		scm_ser,
		temp_ser,
		counts_2M,
		counts_32k,
		temperatures)
