# Mine Rescue Rover - AI Human Detection

AI-based human detection system for an underground mine rescue rover.

## System Architecture

ESP32-CAM → Wi-Fi → Raspberry Pi → OpenCV → YOLOv8s → Human Detection

## Hardware

- Raspberry Pi 4 Model B (2 GB RAM)
- ESP32-CAM
- Wi-Fi

## Software

- Python
- OpenCV
- Ultralytics YOLO
- YOLOv8s
- PyTorch

## AI Processing

All YOLO inference is performed locally on the Raspberry Pi.

The laptop is used only for development, SSH access, and monitoring.

## Features

- Real-time ESP32-CAM video input
- Human detection
- Partial-person detection using pretrained YOLO weights
- Confidence-based detection
- Automatic ESP32-CAM stream reconnection
- CPU-based inference on Raspberry Pi