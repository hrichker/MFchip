import tkinter as tk
from tkinter import messagebox
import serial
import serial.tools.list_ports
import threading
import time

# --- Arduino Serial Setup ---
def find_arduino_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if 'usb' in port.device.lower() or 'arduino' in port.description.lower():
            return port.device
    return None

# Attempt connection
arduino_connected = False
arduino = None
arduino_port = find_arduino_port()

# --- GUI Setup ---
root = tk.Tk()
root.title("Microfluidic Pump Controller")

# GUI Entries
tk.Label(root, text="Desired Flow Rate (µL/min):").grid(row=0, column=0, padx=10, pady=5)
flow_entry = tk.Entry(root)
flow_entry.insert(0, "10")
flow_entry.grid(row=0, column=1, padx=10, pady=5)

tk.Label(root, text="Pump ON Interval (sec):").grid(row=1, column=0, padx=10, pady=5)
on_entry = tk.Entry(root)
on_entry.insert(0, "5")
on_entry.grid(row=1, column=1, padx=10, pady=5)

tk.Label(root, text="Pump OFF Interval (sec):").grid(row=2, column=0, padx=10, pady=5)
off_entry = tk.Entry(root)
off_entry.insert(0, "5")
off_entry.grid(row=2, column=1, padx=10, pady=5)

flow_rate_label = tk.Label(root, text="Current Flow Rate: --- µL/min", font=("Arial", 12), fg="blue")
flow_rate_label.grid(row=5, column=0, columnspan=2, pady=10)

# Send settings to Arduino
def send_settings():
    global arduino_connected
    if not arduino_connected:
        messagebox.showwarning("No Arduino", "Arduino not connected.")
        return
    try:
        flow = float(flow_entry.get())
        on_time = int(float(on_entry.get()) * 1000)
        off_time = int(float(off_entry.get()) * 1000)
        cmd = f"SET,{flow},{on_time},{off_time}\n"
        arduino.write(cmd.encode('utf-8'))
    except ValueError:
        messagebox.showerror("Input Error", "Please enter numeric values.")

tk.Button(root, text="Send Settings", command=send_settings).grid(row=4, column=0, pady=10)
tk.Button(root, text="Exit", command=root.destroy).grid(row=4, column=1, pady=10)

# Thread to update flow rate
def update_flow_rate():
    while arduino_connected:
        if arduino.in_waiting:
            line = arduino.readline().decode('utf-8').strip()
            if line.startswith("FLOW,"):
                try:
                    current_flow = float(line.split(",")[1])
                    flow_rate_label.config(text=f"Current Flow Rate: {current_flow:.2f} µL/min")
                except:
                    pass
        time.sleep(0.1)

# Initial Connection Error Handler
def handle_connection():
    global arduino, arduino_connected
    if arduino_port is None:
        response = messagebox.askyesno("Arduino Not Found", 
            "Arduino not found. Would you like to continue without Arduino?")
        if response:
            arduino_connected = False
            flow_rate_label.config(text="Arduino Disconnected")
        else:
            root.destroy()
    else:
        try:
            arduino = serial.Serial(arduino_port, 9600, timeout=1)
            time.sleep(2)  # Arduino reset
            arduino_connected = True
            threading.Thread(target=update_flow_rate, daemon=True).start()
        except Exception as e:
            response = messagebox.askyesno("Connection Failed", 
                f"Error connecting to Arduino:\n{e}\nContinue without Arduino?")
            if response:
                arduino_connected = False
                flow_rate_label.config(text="Arduino Disconnected")
            else:
                root.destroy()

handle_connection()
root.mainloop()

if arduino_connected and arduino:
    arduino.close()
