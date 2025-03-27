import serial
import tkinter as tk
from tkinter import scrolledtext
import threading

# Initialize the serial connection
ser = serial.Serial(
    port='COM4',  # Update this as needed
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0
)

# Function to send data to ESP32 and receive a response
def send_receive_data():
    while True:
        user_input = message_entry.get()
        if user_input:
            # Send the user input to ESP32
            ser.write(bytes(user_input, 'utf-8'))
            display_message(f"Sent: {user_input}")
            
            # Read the response from the ESP32
            while True:
                next_byte = ser.readline()
                if next_byte != b'':
                    msg = next_byte.decode("utf-8")
                    display_message(f"Received: {msg}")
                    if "Message transmitted." in msg:
                        break

# Function to update the output area with new messages
def display_message(msg):
    output_area.insert(tk.END, msg + '\n')
    output_area.yview(tk.END)

# Function to handle the button press in the GUI
def on_send_button_click():
    # Start a new thread to send and receive data
    threading.Thread(target=send_receive_data, daemon=True).start()

# Create the main GUI window
root = tk.Tk()
root.title("Underwater LiFi User Interface")

output_area = scrolledtext.ScrolledText(root, wrap=tk.WORD, width=40, height=10, state='normal')
output_area.pack(padx=10, pady=10)

# Add a text entry field for user input
message_entry = tk.Entry(root, width=30)
message_entry.pack(padx=10, pady=5)

# Add a button to send the message to the ESP32
send_button = tk.Button(root, text="Send to mESP32", command=on_send_button_click)
send_button.pack(pady=10)

# Start the GUI
root.mainloop()

# Close the serial connection when the GUI is closed
ser.close()
