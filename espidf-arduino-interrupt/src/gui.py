import tkinter as tk
from tkinter import scrolledtext
import serial
import threading
import time

# Set up the serial port (modify COM port and baud rate as needed)
SERIAL_PORT = 'COM3'  # Example for Windows, adjust for your system (e.g., '/dev/ttyUSB0' for Linux)
BAUD_RATE = 115200

# Initialize the serial connection
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# Function to read data from the ESP32 and update the GUI
def read_from_esp32():
    while True:
        if ser.in_waiting > 0:
            response = ser.readline().decode('utf-8').strip()
            if response:
                console_output.insert(tk.END, "ESP32: " + response + '\n')
                console_output.yview(tk.END)  # Auto scroll to the bottom
        time.sleep(0.1)

# Function to send data to ESP32
def send_to_esp32():
    message = input_text.get()  # Get the message from the input field
    if message:
        ser.write((message + '\n').encode('utf-8'))
        console_output.insert(tk.END, "You: " + message + '\n')
        input_text.delete(0, tk.END)  # Clear the input field

# Set up the main window
root = tk.Tk()
root.title("ESP32 Serial Monitor")

# Create a scrollable text widget for the console output
console_output = scrolledtext.ScrolledText(root, width=50, height=15)
console_output.pack(pady=10)

# Create an input field and send button
input_text = tk.Entry(root, width=40)
input_text.pack(pady=5)

send_button = tk.Button(root, text="Send", command=send_to_esp32)
send_button.pack(pady=5)

# Start the serial reading thread
serial_thread = threading.Thread(target=read_from_esp32, daemon=True)
serial_thread.start()

# Run the GUI loop
root.mainloop()
