import serial
import time

# Replace '/dev/ttyACM0' with the name of the serial port connected to your nRF52840 card.
# Replace 115200 with the baud rate of your nRF52840 card.
nameList = ['/dev/ttyACM0','/dev/ttyACM1','/dev/ttyACM2','/dev/ttyACM3','/dev/ttyACM4']
port_connected = False
for each in nameList:
    print(each)
    try:
        ser = serial.Serial(each, 115200, timeout=0.5)
        time.sleep(0.5)

        command = "monitor portok"
        ser.write(command.encode("utf-8") + b"\n")  # Send the command

        # Read and discard the echoed input
        response = ser.readline().decode("utf-8")
        while response.strip() == command:
            response = ser.readline().decode("utf-8")

        # Print the actual response
        print(response + " was the response")
        if "PORTOK" in response:
                print("port was ", each)
                port_connected = True
                break
    except serial.serialutil.SerialException:
        print(each + " port not in use")
if port_connected == False:
    print("No port responded, ending program")
    quit()

# Send a command to the shell.
ser.write(b'monitor\n')  # Send the 'ls' command. The '\n' character is the newline character, which simulates pressing the Enter key.

# Read the response.
response = ser.readline()
print(response)

ser.close()
