<img src="https://raw.githubusercontent.com/wiki/PolySync/OSCC/images/oscc_logo_title.png">

The original OSSC Joystick Demo can be found [here](https://github.com/PolySync/oscc-joystick-commander), and has instructions for setting up the demo. This repository contains some additional features to allow 'wireless' driving:
- UDP server in C listening for inputs on a second thread
- Throttle, brake, and steering values are updated based on data received by the UDP server
- UDP client that accepts inputs from a (binary input) gamepad and dynamically adjusts throttle and steering.

TO-DO: 
- Catch SIGINT and clean up threads in UDP client
- Dynamically adjust brake as well
