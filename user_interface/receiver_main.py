import tkinter as tk
import serial 
import threading 

SERIAL_PORT = '/dev/tty.usb-serial/....' # whatever the port number is from our receiver esp
BAUDRATE = 115200

class SerialReceiverGUI:
    def __init__(self, master):
        self.master = master
        master.title("Receiver GUI")

        self.text = tk.Text(master, height=20, width=50)
        self.text.pack()

        self.serial_port = serial.Serial(SERIAL_PORT, BAUDRATE)
        self.running = True

        # starting the threads for reading serial monitor 
        self.thread = threading.Thread(target=self.read_serial)
        self.thread.daemon = True
        self.thread.start()

        master.protocol("WM_DELETE_WINDOW", self.close)

    def read_serial(self):
        while self.running:
            if self.serial_port.in_waiting:
                data = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                self.text.insert(tk.END, data+"\n")
                self.text.see(tk.END)

    def close(self):
        self.running = False
        self.serial_port.close()
        self.master.destroy()

root = tk.Tk()
app = SerialReceiverGUI(root)
root.mainloop()