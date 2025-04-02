import tkinter as tk
from tkinter import ttk, scrolledtext
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
        output_display.config(state=tk.NORMAL)
        for data in data_queue:
            output_display.insert(tk.END, data + "\n")
        output_display.config(state=tk.DISABLED)
        output_display.see(tk.END)
        data_queue.clear()
    root.after(100, update_gui)

def clear_output():
    """Clears the output display."""
    output_display.config(state=tk.NORMAL)
    output_display.delete("1.0", tk.END)
    output_display.config(state=tk.DISABLED)

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
root.configure(bg="#FFF5E1")

style = ttk.Style()
style.theme_use('clam')

style.configure('TButton',
                font=('Segoe UI', 12, 'bold'), 
                background='black',
                foreground='white',
                padding=7,
                borderwidth=5,
                relief='ridge')

style.map('TButton',
          background=[('pressed', '#A2CFFE'), ('active', '#A2CFFE')],
          relief=[('pressed', 'sunken'), ('!pressed', 'ridge')])

content_frame = tk.Frame(root, bg="#FFF5E1")
content_frame.pack(expand=True, fill="both")

title_frame = tk.Frame(content_frame, bg="#FFF5E1")
title_frame.pack(pady=5)

output_label = tk.Label(content_frame, text="Output Window", 
                        font=("Fantasque Sans Mono", 13, "bold"),
                        fg="black", bg="#FFF5E1")
output_label.pack(pady=(10, 0))

output_display = scrolledtext.ScrolledText(content_frame, width=70, height=12, 
                                           font=("Fantasque Sans Mono", 12),
                                           bg="#2b2b3d", fg="white", wrap="word")
output_display.pack(padx=20, pady=5, fill="both", expand=True)
output_display.config(state=tk.DISABLED)

button_frame = tk.Frame(content_frame, bg="#FFF5E1")
button_frame.pack(pady=10, fill="x")

clear_button = ttk.Button(button_frame, text="Clear", command=clear_output, style="TButton")
clear_button.pack(pady=5)

thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

update_gui()

root.protocol("WM_DELETE_WINDOW", close)
root.mainloop()

