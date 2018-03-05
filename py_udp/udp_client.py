from __future__ import division

import socket
from time import sleep
from threading import Thread

from inputs import devices
from inputs import get_gamepad

throttle = 0.0 # 0.0 - 1.0
brake = 0.0 # 0.0 - 1.0
steering = 1.0 # 0.0 - 2.0

throttle_active = False
brake_active = False
steering_direction = 0 # 1 is right, -1 is left

def get_gamepad_events():
    global throttle_active, brake_active, steering_direction
    while(1):
        events = get_gamepad()
        for event in events:
            # You should change the event.code handlers to match your gamepad controller

            # Throttle
            if 'BTN_BASE2' in event.code:
                if int(event.state) == 1:
                    throttle_active = True
                elif int(event.state) == 0:
                    throttle_active = False

            # Brake
            if 'BTN_TOP' in event.code:
                if int(event.state) == 1:
                    brake_active = True
                elif int(event.state) == 0:
                    brake_active = False

            # Steering
            if 'ABS_X' in event.code:
                if int(event.state) == 127:
                    steering_direction = 0
                elif int(event.state) == 255:
                    steering_direction = 1
                elif int(event.state) == 0:
                    steering_direction = -1

def update():
    global throttle, throttle_active, brake, brake_active, steering, steering_direction
    
	# Update values ---
    while(1):
        sleep(0.1)
        
        # Throttle
        if throttle_active:
            # 0.6 maximum throttle for parking lot
            if throttle < 0.6:
                throttle += 0.1
        elif throttle > 0.1:
            throttle -= 0.1

        # Brake
        if brake_active:
            brake = 0.4
        else:
            brake = 0.0

        # Steering
        if steering_direction == 1 and steering < 1.9:
            steering += 0.1
        elif steering_direction == 0 and steering != 1.0:
            dif = 1.0 - steering
            if dif < 0:
                # Need to turn wheel left to get back to center
                steering -= 0.1
            else:
                steering += 0.1
        elif steering_direction == -1 and steering > 0.09:
            steering -= 0.1

        #print "throttle: ", round(throttle, 1), "brake: ", brake, "steering: ", round(steering, 1)

thread1 = Thread(target=get_gamepad_events)
thread2 = Thread(target=update)

thread1.start()
thread2.start()

UDP_IP_ADDRESS = "127.0.0.1"
UDP_PORT_NO = 6565

clientSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while True:
    sleep(0.1)
    message = "{:1.1f} {:1.1f} {:1.1f}".format(brake, throttle, steering)
    print(message)
    clientSock.sendto(message, (UDP_IP_ADDRESS, UDP_PORT_NO))
