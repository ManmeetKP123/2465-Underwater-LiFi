import serial

def main():
    print("Welcome to the Underwater LiFi User Interface!")

    ser = serial.Serial(
        port='COM4',
        baudrate=115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=0)

    print("connected to: " + ser.portstr)

    while True:
        # wait for user input.
        print("Please input a message to send to the ESP32:")
        user_input = input()

        ser.write(bytes(user_input, 'utf-8'))
        print("Sent message to ESP32")
        
        next_byte = 0
        msg = ""
        while True:
            next_byte = ser.readline()
            if(next_byte != b''):
                msg = next_byte.decode("utf-8")
                print(msg)
                if(msg.find("Message transmitted.") != -1):
                    break
    ser.close()

if __name__ == "__main__":
    main()