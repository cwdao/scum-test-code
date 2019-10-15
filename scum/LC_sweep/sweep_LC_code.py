import numpy as np
import re
import serial
import signal
import sys
import time

def measure_counters(scm_ser, LC_codes, counts_LC):
	"""
	Inputs:
		scm_ser: String. Serial to SCM over UART.
		LC_codes: Array. Stores the LC code.
		counts_LC: Array. Stores the LC counts.
	Outputs:
		No return value. Reads the LC counters into a file.
	"""
	while True:
		try:
			sweep_data = scm_ser.readline().decode().rstrip()
		except:
			continue
		print(sweep_data)
		data = sweep_data.split()

		if sweep_data and data[0].lower().startswith("monotonic") and len(data) == 6:
			LC_code = int(re.sub(r"[^\d]", "", data[3]))
			LC_codes.append(LC_code)

			count_LC = int(re.sub(r"[^\d]", "", data[5]))
			counts_LC.append(count_LC)

			print("Read LC_code={}, count_LC={}".format(LC_code, count_LC))

if __name__ == "__main__":
	scm_port = "COM12"
	filename = "LC_code_sweep_data"

	LC_code, counts_LC = [], []

	def close(sig, frame):
		# Close serial ports
		scm_ser.close()

		# Save arrays to file
		filename_time = filename + "_" + time.strftime("%Y%m%d_%H%M%S")
		np.savez(filename_time, LC_code=LC_code, counts_LC=counts_LC)
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
		LC_code,
		counts_LC)
