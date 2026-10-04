# Stage 2 – Project Requirements / PRD

## 1. Project Title

**Virtual Sensor Monitoring & Alert System**

## 2. Purpose

The purpose of this project is to develop a Linux-based software system that demonstrates communication between a Linux kernel character device driver and a C++ user-space monitoring application.

The system generates simulated temperature readings, monitors them against configurable thresholds, generates alerts for abnormal conditions, and records monitoring events.

## 3. Functional Requirements

### FR1 – Virtual Sensor

The system shall provide a software-based virtual temperature sensor.

The sensor shall generate simulated temperature values that can change gradually over time.

### FR2 – Character Device Driver

The system shall implement a Linux character device driver.

The driver shall create and expose the device:

```text
/dev/virtual_sensor
```

### FR3 – Sensor Data Reading

The user-space application shall be able to open and read sensor data from the character device.

### FR4 – Device Control

The system shall use `ioctl()` commands to control the virtual sensor.

The supported controls include simulation mode management and sensor reset functionality.

### FR5 – Monitoring Application

A C++17 application shall continuously read sensor values and process the readings.

### FR6 – Threshold Detection

The application shall classify readings into:

- NORMAL
- WARNING
- CRITICAL

The warning threshold and critical offset shall be configurable.

### FR7 – Alert Generation

The system shall generate an alert when the sensor reading crosses a configured severity threshold.

Repeated alerts shall be avoided while the sensor remains at the same severity.

### FR8 – Event Logging

The system shall record sensor readings and important state changes in CSV format.

The log shall contain information such as timestamp, sensor name, reading, threshold, status, event type, and message.

### FR9 – Monitoring Control

The system shall provide Start and Stop controls for monitoring.

### FR10 – Dashboard

The system shall provide a local web dashboard for viewing sensor status, readings, alerts, configuration, and monitoring controls.

### FR11 – API

The system shall provide local HTTP/JSON API endpoints for monitoring and control.

## 4. Non-Functional Requirements

### Performance

The monitoring application should continuously process sensor readings without unnecessary delays.

### Reliability

The system should continue monitoring while the virtual device is available and should report device or logging errors when they occur.

### Security

The application should operate locally through `127.0.0.1`.

The kernel module shall be loaded through the Linux kernel module mechanism.

Private Secure Boot signing keys shall not be included in the project repository.

### Maintainability

The project shall use separate components for:

- Device driver
- Device interface
- Monitoring logic
- Dashboard server
- Web interface
- Tests

### Portability

The project is intended for Linux systems with a compatible kernel and development environment.

## 5. Hardware Requirements

No physical hardware is required.

Minimum requirements:

- Linux operating system
- x86-64 computer or compatible Linux system
- Sufficient storage for source code and build tools

## 6. Software Requirements

- Linux OS
- GCC/G++
- C++17 support
- Linux kernel headers
- GNU Make
- Kernel module development support
- Basic command-line utilities
- Web browser for the local dashboard

## 7. Technology Constraints

The project follows the training requirement that only **C/C++** be used for implementation.

The kernel driver is implemented in C because Linux kernel drivers are commonly developed using C.

The user-space monitoring application is implemented in C++17.

No Python, Java, JavaScript backend, IoT hardware, MQTT, or cloud platform is required for the core system.

## 8. User Requirements

A user should be able to:

1. Load the virtual sensor kernel module.
2. Verify the `/dev/virtual_sensor` device.
3. Start the C++ monitoring application.
4. View sensor readings.
5. Change simulation modes.
6. Configure warning thresholds.
7. Start and stop monitoring.
8. Observe generated alerts.
9. View monitoring events.
10. Access the system through the local dashboard.

## 9. Expected Inputs

The system receives:

- Simulated temperature readings from the kernel driver
- Configuration values from the monitoring application/API
- Simulation mode commands
- Monitoring Start/Stop commands

## 10. Expected Outputs

The system produces:

- Current temperature readings
- NORMAL/WARNING/CRITICAL status
- Alerts
- CSV event logs
- JSON
