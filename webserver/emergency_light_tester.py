import serial
import time
import random
import queue
import json
import threading
import struct
import re
from flask import Flask, jsonify, render_template, request
from datetime import datetime



app = Flask(__name__)
clear = "\n"
serial_buffer_status = queue.Queue(0)
serial_buffer_result = queue.Queue(0)
serial_buffer_log = queue.Queue(0)
Log_dict = {}
port_test = 9
buffer_temp = []
text = ''
remaining_elements = []
nodes_missing_list = []
nameList = ['/dev/ttyACM0','/dev/ttyACM1','/dev/ttyACM2','/dev/ttyACM3','/dev/ttyACM4']
port_connected = False
ansi_escape = re.compile(r'\x1b(\[[0-?]*[ -/]*[@-~]?|[ -/]*[@-~])')

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

nodes_list = []
with open('data/nodes_file.txt', 'r') as openfile:
    nodes_list = [line.strip() for line in openfile.readlines()]
     
def generate_nodes():
    with open('data/nodes_file.txt', 'r') as openfile:
        nodes_list = [line.strip() for line in openfile.readlines()]
    return {node: 'No response' for node in nodes_list}


@app.route("/")
def home():
    return render_template('index.html')

@app.route("/get_status_updates")
def get_status_updates():
    output = {}
    try:
        while not serial_buffer_status.empty():
            sentence = serial_buffer_status.get(block=False)
            name, response = sentence.split()[1:]
            output[name] = response
    except queue.Empty:
        print("Empty")
    if output:
        print(output)
    return jsonify({"status": output})

@app.route("/get_result_updates")
def get_result_updates():
    output = {}
    try:
        while not serial_buffer_result.empty():
            sentence = serial_buffer_result.get(block=False)
            name, response = sentence.split()[1:]
            output[name] = response
            
    except queue.Empty:
        print("Empty")

    if output:
        print(output)
    return jsonify({"result": output})

@app.route("/get_logged_results")
def get_logged_results():
    try:
        while not serial_buffer_log.empty():
            sentence = serial_buffer_log.get(block=False)
            result, timestamp, node = sentence.split()

            # Log result to global_logs
            log_to_dict(Log_dict, result, timestamp, node)

    except queue.Empty:
        print("Empty")

    return jsonify(Log_dict)

def log_to_dict(dict, result, timestamp, node):
    timestamp = str(datetime.fromtimestamp(int(timestamp))) 

    # Check if node is already in the dictionary
    if node not in dict:
        # If node is not in the dictionary, add a new dictionary as its value
        dict[node] = {}

    # Add result to node dictionary only if timestamp does not exist, preventing duplicates
    if timestamp not in dict[node]:
        dict[node][timestamp] = result

    return dict

@app.route("/request_status")
def request_status():

    ser.write(clear.encode("utf-8"))
    msg = "monitor status\n"
    ser.write(msg.encode("utf-8"))
    return []

@app.route("/test")
def request_test():

    list = set_nodes_list()
    data = jsonify(nodes_list)
    return data

def set_nodes_list():
    if nodes_list:
        
        ser.write(clear.encode("utf-8"))
        msg = "monitor reset_nodes\n"
        ser.write(msg.encode("utf-8"))
        msg = "monitor add_first_node " + nodes_list[0] + "\n"
        ser.write(msg.encode("utf-8"))
        time.sleep(0.1)
        for each in nodes_list[1:]:
            time.sleep(0.1)
            msg = "monitor add_node " + each + "\n"
            ser.write(msg.encode("utf-8"))
        return(nodes_list)
    else:
        return []


@app.route("/test2")
def request_test2():
    selected_value = request.args.get('selectedValue')
    print(selected_value)
    msg = "monitor log " + str(int(selected_value)) + "\n"
    ser.write(msg.encode("utf-8"))    
    return []


@app.route("/calibrate")
def calibrate():
    selected_value = request.args.get('selectedValue')
    msg = "monitor calibrate " + selected_value + "\n"
    ser.write(msg.encode("utf-8"))
    return []


@app.route("/request_test")
def request_test_start():
    dt = datetime.now()
    ts = int(datetime.timestamp(dt))
    ser.write(clear.encode("utf-8"))
    test_start = "monitor start "  + str((request.args.get('durationValue', default=60, type=int))) + " " + str(ts) + "\n"
    ser.write(test_start.encode("utf-8"))
    return jsonify(generate_nodes())  


def serial_data_buffer():
    get_nodes = 8
    ser.write(get_nodes.to_bytes(1, byteorder='big'))
    set_nodes_list()
    
    while True:
        try:
            raw_data = ser.readline()
            line = raw_data.decode("utf-8").strip()
            line = line.replace("uart:~$", "")
            line = line.lstrip("IJ")
            line = ansi_escape.sub('', line).strip()
            if line.startswith("result"):
                serial_buffer_result.put(line, timeout=1)
            elif line.startswith("status"):
                serial_buffer_status.put(line, timeout=1)
            elif line.startswith("acking"):
                serial_buffer_result.put(line, timeout=1)
                node_name = line[7:].split(" ")[0]
                if node_name not in nodes_list:
                    nodes_list.append(node_name)
                    store_node(node_name)
            elif line.startswith("nodeok"):
                node_name = line[7:].split(" ")[0]
                print("got node " + node_name)
                if node_name not in nodes_list:
                    print("added to nodes list", node_name)
                    store_node(node_name)
                    nodes_list.append(node_name)
            elif line.startswith("logged"):
                serial_buffer_log.put(line[7:], timeout=1)
            elif line and not line.isspace():
                print(line)
                
        except UnicodeDecodeError:
            print("Got something we could not read")

def store_node(node):
    with open("data/nodes_file.txt", "a") as outfile:   
        outfile.writelines(node+"\n")


thread = threading.Thread(target=serial_data_buffer)
thread.start()
  
if __name__ == '__main__':
    app.run()