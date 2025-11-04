# NILM Host – Real-Time ESP32 Telemetry Visualizer

## Overview
This project receives live telemetry from an **ESP32 NILM simulator** over **MQTT**,  
plots it in real time, and optionally saves the stream to a CSV log.

It is the *host-side* component of the future NILM system:  
the ESP32 publishes simulated energy data, and this Python module  
listens, visualizes, and prepares it for analysis or training.

---

## Project structure
...