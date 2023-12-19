The project concentrated on making the testing of emergency lighting in
commercial buildings more efficient. Traditional methods for testing these
lights are time-consuming and expensive for several reasons. Each light fixture
must be manually checked, a process that becomes even more arduous in large
buildings with many lights. This involves verifying each light's functionality
and compliance with safety standards, often requiring access to hard-to-reach
areas and coordination with multiple maintenance teams. Such manual inspections
can also disrupt a building's daily operations, leading to additional indirect
costs. The need for specialized staff and equipment for testing and maintenance
further increases expenses. By implementing Bluetooth Mesh technology, I was
able to automate this process, significantly reducing the need for manual labor
and easing these operational and financial challenges.

You will need at least:
* two nRF52840 Development Kits or more
* a computer with Python v3.10 and the nRF Connect SDK installed
* a phone with the nRF Mesh mobile app
* a board compatible ADC brightness-sensor, in this case a light dependent resistor is used.
* relay as well as cables to connect them together.

How to setup:

1. Clone the project into your mesh samples folder
1. Connect the boards to the computer using a USB cable.
1. Build and flash one of the boards with the light monitor server and one with the light monitor client. See here for a guide on how to do so: https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/samples/bluetooth/mesh/light/README.html#building_and_running https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/getting_started.html
1. Using the nRF Mesh app, provision your boards. Establish a group address for the client to publish to and one for the servers to publish to. Subscribe the client to the servers publish address and the servers to the clients. See here for a guide on how to do this: https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/samples/bluetooth/mesh/light/README.html#provisioning_the_device
1. Connect the sensor to pin 03 on the server board as well as ground and power.
1. Connect the relay to pin 29 on the server board as well as ground and power.
1. Open the terminal and navigate to where you have the emergency_light_tester.py file.
1. Activate venv using activate. It can be found in the /venv/bin folder, or you can use the command: `venv/bin/activate`
1. Run the command: `flask --app emergency_light_tester run`
1. Open a web browser and go to the address: http://127.0.0.1:5000/
