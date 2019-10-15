import sys

if len(sys.argv) != 2 and len(sys.argv) != 3:
	print('Usage: python generate_csv_packet.py <packet> [<file>]')
	sys.exit(1)

def parse(packet):
	bits = packet.replace('/[^01]/', '')
	bits_array = list(bits)
	padded_bits_array = bits_array + ['0'] * (1000 - len(bits_array))
	return ','.join(padded_bits_array)

if __name__ == '__main__':
	packet = parse(sys.argv[1])
	
	if len(sys.argv) == 3:
		filename = sys.argv[2]
		if not filename.endswith('.csv'):
			filename += '.csv'
		file = open(filename, 'w')
		file.write(packet)
		file.close()
		print("Wrote to", filename)
		sys.exit(0)
	else:
		print(packet)
		sys.exit(0)
