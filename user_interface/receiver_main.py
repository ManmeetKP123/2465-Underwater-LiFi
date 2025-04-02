import tkinter as tk
import serial
import threading

SERIAL_PORT = 'COM11' # '/dev/tty.usbserial-1410'  # Set to your ESP32 serial port
BAUDRATE = 115200

class SerialReceiverGUI:
    def __init__(self, master):
        self.master = master
        master.title("Receiver GUI")

        # Text widget to display data from ESP32
        self.text = tk.Text(master, height=20, width=50)
        self.text.pack()

        # Set up serial communication
        self.serial_port = serial.Serial(SERIAL_PORT, BAUDRATE)
        self.running = True

        # Queue to hold incoming data from the serial thread
        self.data_queue = []

        # Starting the background thread to read the serial data
        self.thread = threading.Thread(target=self.read_serial)
        self.thread.daemon = True  # Ensures thread stops when GUI is closed
        self.thread.start()

        # Schedule the update for GUI periodically (e.g., every 100ms)
        self.update_gui()

        # Handle window close event
        master.protocol("WM_DELETE_WINDOW", self.close)

    def read_serial(self):
        """Reads data from the serial port in a separate thread"""
        while self.running:
            if self.serial_port.in_waiting:
                data = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                if data:
                    self.data_queue.append(data)  # Add data to the queue for GUI update

    def update_gui(self):
        """This function is called periodically to update the GUI with new data"""
        if self.data_queue:
            # Insert all available data into the text widget
            for data in self.data_queue:
                # Filter the data to only show relevant messages
                if("Full Message Recieved:" in data or
                    "Receiving Initiated..." in data or
                    "Length of Message:" in data):
                    self.text.insert(tk.END, data + "\n")
            self.text.see(tk.END)  # Scroll to the end of the text widget
            self.data_queue.clear()  # Clear the queue after displaying the data

        # Schedule the next update in 100ms
        self.master.after(100, self.update_gui)

    def close(self):
        """Closes the serial port and stops the program"""
        self.running = False
        self.serial_port.close()
        self.master.destroy()

# Create the Tkinter root window
root = tk.Tk()
app = SerialReceiverGUI(root)
root.mainloop()
