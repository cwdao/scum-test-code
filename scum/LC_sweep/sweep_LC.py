import numpy as np
import re
import serial
import signal
import sys
import time

def measure_counters(scm_ser, coarses, mids, fines, LC_codes, counts_LC):
	"""
	Inputs:
		scm_ser: String. Serial to SCM over UART.
		coarses: Array. Stores the coarse.
		mids: Array. Stores the coarse.
		fines: Array. Stores the coarse.
		LC_codes: Array. Stores the LC code.
		counts_LC: Array. Stores the LC counts.
	Outputs:
		No return value. Reads the counters into a file.
	"""
	while True:
		try:
			sweep_data = scm_ser.readline().decode().rstrip()
		except:
			continue
		data = sweep_data.split()

		if sweep_data and data[0].lower().startswith("sweep") and len(data) == 9:
			coarse = int(re.sub(r"[^\d]", "", data[2]))
			coarses.append(coarse)

			mid = int(re.sub(r"[^\d]", "", data[4]))
			mids.append(mid)

			fine = int(re.sub(r"[^\d]", "", data[6]))
			fines.append(fine)

			LC_code = coarse * (32 ** 2) + mid * 32 + fine
			LC_codes.append(LC_code)

			count_LC = int(re.sub(r"[^\d]", "", data[8]))
			counts_LC.append(count_LC)

			print("Read coarse={}, mid={}, fine={}, LC_code={}, count_LC={}".format(coarse, mid, fine, LC_code, count_LC))

if __name__ == "__main__":
	scm_port = "COM12"
	filename = "LC_sweep_data"

	coarse, mid, fine, LC_code, counts_LC = [], [], [], [], []

	def close(sig, frame):
		# Close serial ports
		scm_ser.close()

		# Save arrays to file
		filename_time = filename + "_" + time.strftime("%Y%m%d_%H%M%S")
		np.savez(filename_time, LC_code=LC_code, coarse=coarse, mid=mid, fine=fine, counts_LC=counts_LC)
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
		coarse,
		mid,
		fine,
		LC_code,
		counts_LC)
