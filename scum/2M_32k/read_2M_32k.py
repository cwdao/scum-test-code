import numpy as np
import re
import serial
import signal
import sys
import time

def measure_counters(scm_ser, counts_2M, counts_32k):
	"""
	Inputs:
		scm_ser: String. Serial to SCM over UART.
		counts_2M: Array. Stores the 2M counts.
		counts_32k: Array. Stores the 32k counts.
	Outputs:
		No return value. Reads the counters into a file.
	"""
	while True:
		counter_data = scm_ser.readline().decode().rstrip()
		counts = counter_data.split()

		if counter_data and len(counts) == 4:
			count_32k = re.sub(r"[^\d]", "", counts[3])
			counts_32k.append(int(count_32k))

			count_2M = re.sub(r"[^\d]", "", counts[1])
			counts_2M.append(int(count_2M))

			ratio = int(count_2M) / int(count_32k)

			print("Read 2M and 32k counters, count_2M={}, count_32k={}, ratio={}".format(count_2M, count_32k, ratio))

if __name__ == "__main__":
	scm_port = "COM12"
	filename = "2M_32k_counter_data"

	counts_2M, counts_32k = [], []

	def close(sig, frame):
		# Close serial ports
		scm_ser.close()

		# Save arrays to file
		filename_time = filename + "_" + time.strftime("%Y%m%d_%H%M%S")
		np.savez(filename_time, counts_2M=counts_2M, counts_32k=counts_32k)
		print("Saved to " + filename_time + ".npz")

	# Open UART connection to SCM
	scm_ser = serial.Serial(
		port=scm_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=.5)

	# Register SIGINT handler
	signal.signal(signal.SIGINT, close)

	measure_counters(
		scm_ser,
		counts_2M,
		counts_32k)
