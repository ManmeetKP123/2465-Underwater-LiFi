import tkinter as tk
import serial
import threading
from tkinter import scrolledtext

SERIAL_PORT = 'COM5' # '/dev/tty.usbserial-1410'  # Set to your ESP32 serial port
BAUDRATE = 115200

def read_serial():
    """Reads data from serial port in background thread."""
    global running
    while running:
        if ser.in_waiting:
            data = ser.readline().decode('utf-8', errors='ignore').strip()
            if data:
                # Filter the data to only show relevant messages
                if("Full Message Received:" in data or
                    "Receiving" in data or
                    "Length of Message:" in data):
                    data_queue.append(data)

def update_gui():
    """Updates GUI with new data from serial."""
    if data_queue:
        for data in data_queue:
            output_display.config(state=tk.NORMAL)
            output_display.insert(tk.END, data + "\n")
            output_display.config(state=tk.DISABLED)
            output_display.see(tk.END)
        data_queue.clear()
    root.after(100, update_gui)

def close():
    """Closes the serial port and destroys the window."""
    global running
    running = False
    ser.close()
    root.destroy()

ser = serial.Serial(SERIAL_PORT, BAUDRATE)
running = True
data_queue = []

root = tk.Tk()
root.title("Receiver GUI")
root.configure(bg="#f5f0e1")

output_display = scrolledtext.ScrolledText(root, width=70, height=12, font=("Fantasque Sans Mono", 12), 
                                           bg="#2b2b3d", fg="white", wrap="word")
output_display.pack(padx=20, pady=5, fill="both", expand=True)
output_display.config(state=tk.DISABLED)

thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

update_gui()

root.protocol("WM_DELETE_WINDOW", close)
root.mainloop()

