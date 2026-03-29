# embedded-linux-smart_parking
# System Design

This project implements a smart parking system using Embedded Linux on Raspberry Pi.

## Components

- HC-SR04 Ultrasonic Sensors (2 slots)
- Raspberry Pi (running Yocto Linux)
- Kernel module for sensor interfacing
- User-space applications
- LED indicators

## Working

1. Kernel module controls GPIO pins
2. Ultrasonic sensors measure distance
3. Interrupts capture echo timing
4. Distance is calculated
5. Slot status determined:
   - < threshold → Occupied
   - > threshold → Vacant
6. User-space app reads via /dev/parking_sensor
