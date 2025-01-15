import tkinter as tk
from tkinter import messagebox

# Function to handle the input from the user when the Enter button is clicked
def on_enter():
    user_input = entry.get()
    if user_input:
        # Open the file in append mode (creates the file if it doesn't exist)
        with open("user_input.txt", "w") as file:
            file.write(user_input + "\n") 
        messagebox.showinfo("Success", f"The word '{user_input}' has been saved to the file.")

        entry.delete(0, tk.END)
    else:
        messagebox.showwarning("Input Error", "Please enter a word before clicking Enter.")

# Create the main window
root = tk.Tk()
root.title("Word Input GUI")

# Create a label to prompt the user
label = tk.Label(root, text="Please enter a word:")
label.pack(pady=10)

# Create an entry widget for the user to input their word
entry = tk.Entry(root, width=30)
entry.pack(pady=5)

# Create an Enter button that will trigger the on_enter function
enter_button = tk.Button(root, text="Enter", command=on_enter)
enter_button.pack(pady=10)

# Start the Tkinter event loop
root.mainloop()
