from flask import Flask, request, render_template_string
from datetime import datetime
import serial
import threading
import time

app = Flask(__name__)

# Global state variables for demo purposes
sensor_status = "Off"
mode = "0"
unit = "Lux"
frequency = "500"

# Initialize serial connections for both Arduino boards
try:
    arduino_1 = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
    arduino_2 = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
    time.sleep(2)  # Allow time for Arduino to reset
except serial.SerialException as e:
    print(f"Error: Could not open serial connection: {e}")
    arduino_1 = arduino_2 = None

# Log file paths
log_file_1 = 'logs_arduino_1.log'
log_file_2 = 'logs_arduino_2.log'
html_file = 'log_viewer.html'

# Lock to prevent simultaneous file writing
html_file_lock = threading.Lock()

# Function to send a command to Arduino
def send_command(command):
    if arduino_1:
        arduino_1.write(command.encode())  # Send the command as bytes
    if arduino_2:
        arduino_2.write(command.encode())  # Send the command as bytes
    time.sleep(0.5)  # Wait for the Arduino to process the command

def update_html(log_data_1, log_data_2):
    """Update the HTML file with the latest log data."""
    html_content = f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Log Data</title>
        <style>
            body {{ font-family: Arial, sans-serif; }}
            .log-container {{ margin: 20px; }}
            pre {{ background: #f4f4f4; padding: 10px; border: 1px solid #ccc; }}
            h2 {{ color: #333; }}
        </style>
        <script>
            setTimeout(() => {{
                location.reload();
            }}, 1000);
        </script>
    </head>
    <body>
        <div class="log-container">
            <h1>Arduino Log Data</h1>
            <h2>Arduino 1 (ACM0)</h2>
            <pre>{log_data_1}</pre>
            <h2>Arduino 2 (USB0)</h2>
            <pre>{log_data_2}</pre>
        </div>
    </body>
    </html>
    """
    with html_file_lock:
        with open(html_file, 'w') as file:
            file.write(html_content)

def read_serial_data(serial_device, log_file):
    """Reads data from a serial device and writes it to the respective log file."""
    while True:
        try:
            received = serial_device.readline()
            decoded_data = received.decode('utf-8').strip()
            with open(log_file, 'a') as f:
                f.write(decoded_data + '\n')
        except Exception as e:
            print(f"Error reading from {serial_device.port}: {e}")

def update_webpage():
    """Reads log files and updates the HTML webpage every second."""
    while True:
        try:
            # Read the last 3 lines from both log files
            with open(log_file_1, 'r') as f1, open(log_file_2, 'r') as f2:
                log_lines_1 = f1.readlines()
                log_lines_2 = f2.readlines()
                log_data_1 = ''.join(log_lines_1[-3:])  # Last 3 lines for Arduino 1
                log_data_2 = ''.join(log_lines_2[-3:])  # Last 3 lines for Arduino 2
            # Update the HTML content with the new log data
            update_html(log_data_1, log_data_2)
            time.sleep(1)  # Sleep for 1 second before the next update
        except Exception as e:
            print(f"Error updating webpage: {e}")

# Threads for reading data from each Arduino
thread_arduino_1 = threading.Thread(target=read_serial_data, args=(arduino_1, log_file_1))
thread_arduino_2 = threading.Thread(target=read_serial_data, args=(arduino_2, log_file_2))
thread_webpage = threading.Thread(target=update_webpage)

# Start the threads
thread_arduino_1.start()
thread_arduino_2.start()
thread_webpage.start()

# HTML template for the main page
template = """
<!doctype html>
<html>
<head>
    <title>Project III</title>
</head>
<body style="width:960px; margin: 20px auto;">
    <h1 style="text-align: center;">Project III</h1>

    <p style="text-align: center;"><strong>Date is</strong> {{ current_date }}</p>
    <p style="text-align: center;"><strong>Sensor is :</strong> {{ sensor_status }}</p>

    <form action="/" method="post" name="Mode">
        <p style="text-align: center;"><strong>Sensor :</strong>
        <input name="sensor" type="submit" value="On" />
        <input name="sensor" type="submit" value="Off" />
        <input name="sensor" type="submit" value="Toggle" /></p>
    </form>

    <form action="/" method="post" name="Mode">
        <p style="text-align: center;"><strong>Mode:</strong>
        <input name="mode" type="submit" value="0" />
        <input name="mode" type="submit" value="1" />
        <input name="mode" type="submit" value="2" /></p>
    </form>

    <form action="/" method="post" name="Unit">
        <p style="text-align: center;"><strong>Unit:&nbsp;</strong>
        <input name="unit" type="submit" value="Lux" />
        <input name="unit" type="submit" value="Voltage" /></p>
    </form>

    <form action="/" method="post" name="Frequency">
        <p style="text-align: center;"><strong>Freq:</strong>
        <input name="frequency" type="submit" value="500" />
        <input name="frequency" type="submit" value="1000" />
        <input name="frequency" type="submit" value="1500" />
        <input name="frequency" type="submit" value="2000" />
        <input name="frequency" type="submit" value="5000" /></p>
    </form>

    <h2 style="text-align: center;">Log Data from Arduino 1 & Arduino 2</h2>
    <p style="text-align: center;"><a href="/logs">View Log</a></p>
</body>
</html>
"""

@app.route("/", methods=["GET", "POST"])
def index():
    global sensor_status, mode, unit, frequency
    if request.method == "POST":
        # Handle Sensor Actions
        if "sensor" in request.form:
            action = request.form["sensor"]
            if action == "On":
                sensor_status = "On"
                send_command("sensor on")
            elif action == "Off":
                sensor_status = "Off"
                send_command("sensor off")
            elif action == "Toggle":
                sensor_status = "On" if sensor_status == "Off" else "Off"
                send_command(f"sensor {sensor_status.lower()}")

        # Handle Mode
        elif "mode" in request.form:
            mode = request.form["mode"]
            send_command(f"mode {mode}")

        # Handle Unit
        elif "unit" in request.form:
            unit = request.form["unit"]
            send_command(f"unit {unit.lower()}")

        # Handle Frequency
        elif "frequency" in request.form:
            frequency = request.form["frequency"]
            send_command(f"freq {frequency}")

    return render_template_string(
        template,
        current_date=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        sensor_status=sensor_status,
        mode=mode,
        unit=unit,
        frequency=frequency,
    )

@app.route("/logs")
def logs():
    """Serve the log file as HTML."""
    try:
        with open(html_file, 'r') as f:
            log_data = f.read()
        return log_data
    except Exception as e:
        return f"Error loading log data: {e}"

if __name__ == "__main__":
    app.run(host="192.168.0.24", port=9070, debug=True)
    # app.run(host="172.20.10.4", port=7000, debug=True)


