import serial
import tkinter as tk
from tkinter import scrolledtext
import threading
import queue
import time

# Initialize the serial connection
ser = serial.Serial(
    port= 'COM4',#'/dev/tty.usbserial-1410',  # Update this to your correct port
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0.5  # Set timeout for non-blocking reads
)

# Create a queue for thread-safe communication between serial thread and the main thread
data_queue = queue.Queue()

# Function to send data to ESP32
def send_data():
    user_input = message_entry.get()
    if user_input:
        print(f"Sending: {user_input}")  # Debug print
        ser.write(bytes(user_input, 'utf-8'))
        display_message(f"Sent: {user_input}")
        message_entry.delete(0, tk.END)  # Clear the entry field after sending

# Function to read data from the ESP32 and put it in the queue
def read_data():
    try:
        if ser.in_waiting > 0:
            next_byte = ser.readline()
            if next_byte:
                msg = next_byte.decode("utf-8")
                data_queue.put(msg)  # Put received data into the queue
    except Exception as e:
        print(f"Error reading serial data: {e}")

    # Schedule the next serial read
    threading.Timer(0.1, read_data).start()  # Call read_data every 100ms

# Function to update the output area with new messages
def display_message(msg):
    output_area.insert(tk.END, msg + '\n')
    output_area.yview(tk.END)

# Function to handle the button press in the GUI
def on_send_button_click():
    send_data()

# Function to check the queue and update the GUI with new data
def check_for_new_data():
    while not data_queue.empty():  # Check if there's new data in the queue
        msg = data_queue.get()  # Get the data from the queue
        display_message(f"Received: {msg}")
    root.after(100, check_for_new_data)  # Schedule the next check

# Create the main GUI window
root = tk.Tk()
root.title("Underwater LiFi User Interface")

output_area = scrolledtext.ScrolledText(root, wrap=tk.WORD, width=40, height=10, state='normal')
output_area.pack(padx=10, pady=10)

# Add a text entry field for user input
message_entry = tk.Entry(root, width=30)
message_entry.pack(padx=10, pady=5)

# Add a button to send the message to the ESP32
send_button = tk.Button(root, text="Send to ESP32", command=on_send_button_click)
send_button.pack(pady=10)

# Start the serial reading thread (this thread will handle reading data from ESP32)
threading.Thread(target=read_data, daemon=True).start()

# Start checking for new data and updating the GUI
root.after(100, check_for_new_data)

# Start the GUI
root.mainloop()

# Close the serial connection when the GUI is closed
ser.close()

