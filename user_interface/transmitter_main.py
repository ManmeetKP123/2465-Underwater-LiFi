import serial
import threading
import queue
import time
import tkinter as tk
from tkinter import scrolledtext
from tkinter import ttk
from PIL import Image, ImageTk
import os

ser = serial.Serial(
    port= 'COM4',
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0.5
)

data_queue = queue.Queue()

root = tk.Tk()
root.title("Underwater LiFi")
root.geometry("700x550")
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

title_label = tk.Label(title_frame, text="Welcome to Underwater LiFi",
                       font=("Segoe UI", 22, "italic", "bold"),
                       fg="black", bg="#FFF5E1")
title_label.pack(side=tk.LEFT)

img_path = os.path.abspath("blueLightbulb.jpg")  

try:
    lightbulb_image = Image.open(img_path)  
    lightbulb_image = lightbulb_image.resize((40, 40), Image.Resampling.LANCZOS)
    lightbulb_icon = ImageTk.PhotoImage(lightbulb_image)
    lightbulb_label = tk.Label(title_frame, image=lightbulb_icon)
    lightbulb_label.image = lightbulb_icon
    lightbulb_label.pack(side=tk.LEFT, padx=10)
except Exception as e:
    print("Error loading image:", e)

text_input_label = tk.Label(content_frame, text="Enter Text", 
                            font=("Fantasque Sans Mono", 13, "bold"),
                            fg="black", bg="#FFF5E1")  
text_input_label.pack(anchor="w", padx=20)

text_input = scrolledtext.ScrolledText(content_frame, width=70, height=5, 
                                       font=("Fantasque Sans Mono", 13),
                                       bg="white", fg="black", wrap="word")
text_input.pack(padx=20, pady=5, fill="x")

output_label = tk.Label(content_frame, text="Output Window", 
                        font=("Fantasque Sans Mono", 13, "bold"),
                        fg="black", bg="#FFF5E1")
output_label.pack(anchor="w", padx=20, pady=(10, 0))

output_display = scrolledtext.ScrolledText(content_frame, width=70, height=12, 
                                           font=("Fantasque Sans Mono", 12),
                                           bg="#2b2b3d", fg="white", wrap="word")
output_display.pack(padx=20, pady=5, fill="x")
output_display.config(state=tk.DISABLED)

button_frame = tk.Frame(content_frame, bg="#FFF5E1")
button_frame.pack(pady=10)

def read_data():
    try:
        if ser.in_waiting > 0:
            next_byte = ser.readline()
            if next_byte:
                msg = next_byte.decode("utf-8")
                data_queue.put(msg)
    except Exception as e:
        print(f"Error reading serial data: {e}")

def display_message(msg):
    output_display.insert(tk.END, msg + '\n')
    output_display.yview(tk.END)

def send_text():
    content = text_input.get("1.0", tk.END).strip()
    if content:
        output_display.config(state=tk.NORMAL)
        output_display.insert(tk.END, f"Sent: {content}\n")
        output_display.config(state=tk.DISABLED)
        text_input.delete("1.0", tk.END)
        ser.write(bytes(content, 'utf-8'))

def clear_output():
    output_display.config(state=tk.NORMAL)
    output_display.delete("1.0", tk.END)
    output_display.config(state=tk.DISABLED)

def check_for_new_data():
    while not data_queue.empty():
        msg = data_queue.get()
        display_message(f"Received: {msg}")
    root.after(100, check_for_new_data)

send_button = ttk.Button(button_frame, text="Send", command=send_text, style='TButton')
send_button.pack(side=tk.LEFT, padx=15, ipadx=10, ipady=3)

clear_button = ttk.Button(button_frame, text="Clear", command=clear_output, style='TButton')
clear_button.pack(side=tk.LEFT, padx=15, ipadx=10, ipady=3) 


def on_enter(event):
    send_text()
    send_button.state(['pressed'])
    root.after(200, lambda: send_button.state(['!pressed']))
    return "break"

text_input.bind("<Return>", on_enter)


text_input.bind("<Return>", on_enter)


threading.Thread(target=read_data, daemon=True).start()
root.after(100, check_for_new_data)
root.mainloop()

ser.close()