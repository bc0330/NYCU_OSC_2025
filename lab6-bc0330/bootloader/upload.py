import os
import struct
import time
import serial

path = '/dev/pts/19'
filename = '../kernel8.img'
file_stats = os.stat(filename)
file_size = file_stats.st_size

with open(filename, 'rb') as f:
    kernel_data = f.read()
checksum = sum(kernel_data) & 0xFFFFFFFF

tty = serial.Serial(path, 115200, timeout=1)
tty.write(b'\xAA')
ack = tty.read(1)
if ack == b'\xAA':
    print("Acknowledgement received!")
    print("Start data transmission...") 
    # header = struct.pack('<III', 
    #                         0x544F4F42, 
    #                         file_size, 
    #                         checksum)

    # tty.write(header)
    tty.write(struct.pack('<I', file_size))   # Little endian, unsigned int
    time.sleep(3)
    with open(filename, 'rb') as f:
        while True:
            data = f.read(1024)
            if not data:
                break
            tty.write(data)
            time.sleep(1)

    print("Data transmission completed!")

else:
    print("Failed to receive acknowledgement!")
    print(ack)
tty.close()

        