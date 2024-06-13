with open("./bcm2710-rpi-3-b-plus.dtb", "rb") as f:
	data = f.read()
# Filter out the 0x00 bytes
filtered_data = bytes(b for b in data if b != 0x00)

# Print the filtered binary data
print(filtered_data)

#print(data)