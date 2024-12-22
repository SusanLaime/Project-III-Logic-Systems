import serial
from datetime import datetime
import threading

# Initialize serial connections for both Arduino boards
arduino_1 = serial.Serial('/dev/ttyACM0', 115200)
arduino_2 = serial.Serial('/dev/ttyUSB0', 115200)

# Log file paths
log_file_1 = 'logs_arduino_1.log'
log_file_2 = 'logs_arduino_2.log'

def read_serial_data(serial_device, log_file, device_name):
    """Reads data from a serial device and writes it to the respective log file."""
    while True:
        try:
            recibido = serial_device.readline()
            decoded_data = recibido.decode('utf-8').strip()
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            log_entry = f"{timestamp} - {device_name}: {decoded_data}"
            print(log_entry)
            with open(log_file, 'a') as f:
                f.write(log_entry + '\n')
        except Exception as e:
            print(f"Error reading from {serial_device.port}: {e}")

# Threads for reading data from each Arduino
thread_arduino_1 = threading.Thread(target=read_serial_data, args=(arduino_1, log_file_1, "Arduino 1 (ACM0)"))
thread_arduino_2 = threading.Thread(target=read_serial_data, args=(arduino_2, log_file_2, "Arduino 2 (USB0)"))

# Start the threads
thread_arduino_1.start()
thread_arduino_2.start()

# Keep the main program running
thread_arduino_1.join()
thread_arduino_2.join()