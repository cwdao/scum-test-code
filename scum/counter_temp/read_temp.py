import serial
import signal
import time

if __name__ == "__main__":
	temp_port = "COM11"

	def close(sig, frame):
		# Close serial port
		temp_ser.close()
	# Open COM port to temperature sensor
	temp_ser = serial.Serial(
		port=temp_port,
		baudrate=19200)

	# Register SIGINT handler
	signal.signal(signal.SIGINT, close)

	while True:
		temp_ser.write(b"temp\n")
		temp_ser.readline()
		temp_ser.write(b"temp\n")
		temp_data = temp_ser.readline().decode().rstrip()
		print("Temperature:", temp_data)
		time.sleep(1)
