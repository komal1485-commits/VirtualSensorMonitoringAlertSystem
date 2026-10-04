# Stage 6 – Final Documentation & Project Summary

## 1. Project Title

# Virtual Sensor Monitoring & Alert System

A Linux-based virtual sensor monitoring system developed using a Linux character device driver, C++ system programming, threshold-based monitoring, event logging, and a local dashboard.

---

## 2. Project Overview

The Virtual Sensor Monitoring & Alert System is a Linux-based project designed to demonstrate communication between a Linux kernel device driver and a user-space C++ monitoring application.

The system uses a virtual sensor implemented as a Linux character device. The device driver provides simulated sensor readings to the C++ application through the character device `/dev/virtual_sensor`.

The monitoring application reads the sensor data, checks the readings against configured thresholds, generates alerts when required, records events in a CSV file, and provides a local dashboard for monitoring system status.

The project focuses on Linux Device Drivers, Linux System Programming, and C++ programming.

---

## 3. Problem Statement

Real-world monitoring systems need to continuously collect sensor readings and detect abnormal conditions.

This project demonstrates how such a monitoring mechanism can be implemented at the Linux system level without requiring physical sensor hardware.

The project addresses the following requirements:

- Create a Linux character device.
- Transfer sensor data between kernel space and user space.
- Read and process sensor data using C++.
- Detect abnormal sensor conditions using thresholds.
- Generate warning and high-level alerts.
- Record monitoring events.
- Display system status through a local dashboard.

---

## 4. Project Objectives

The main objectives of the project are:

1. To develop a Linux character device driver.
2. To create and access a virtual sensor device.
3. To demonstrate kernel-to-user-space communication.
4. To develop a C++ monitoring application.
5. To implement sensor simulation modes.
6. To detect WARNING and HIGH conditions.
7. To generate alerts based on threshold values.
8. To maintain event logs in CSV format.
9. To provide a local monitoring dashboard.
10. To demonstrate Linux system programming concepts in an integrated application.

---

## 5. System Architecture

The system consists of the following major components:

```text
+-----------------------------+
|     Virtual Sensor Driver   |
|        Kernel Space         |
+-------------+---------------+
              |
              | Character Device
              | /dev/virtual_sensor
              v
+-----------------------------+
|      C++ Monitor            |
|        User Space           |
+-------------+---------------+
              |
       +------+------+
       |             |
       v             v
+-------------+  +-------------+
| Threshold   |  | Event       |
| Detection   |  | Logging     |
+------+------+  +-------------+
       |             |
       v             v
+-------------+  +-------------+
| Alert       |  | events.csv  |
| Generation  |  +-------------+
+-------------+
       |
       v
+-----------------------------+
|     Local Dashboard         |
|       Port 8080             |
+-----------------------------+
```
---

## 6. Technologies Used

### Programming Languages

- C
- C++17
- HTML

### Linux Technologies

- Linux Kernel Module
- Character Device Driver
- Device Files
- udev
- Linux file operations
- ioctl interface
- Kernel-to-user-space communication
- Linux permissions and groups
- Secure Boot module signing

### Development Tools

- GCC
- G++
- Make
- Linux Terminal
- VS Code / Text Editor

---

## 7. Linux Character Device Driver

The project implements a virtual Linux character device driver.

The device created by the driver is:

/dev/virtual_sensor

The driver provides the interface between the Linux kernel and the C++ monitoring application.

Main driver functions include:

- Module initialization and cleanup
- Character device registration
- Device creation
- File open and release operations
- Read and write operations
- IOCTL communication
- Kernel-side sensor value generation
- Threshold-related data handling

The driver was compiled as a kernel module and loaded into the running Ubuntu kernel.

---

## 8. Secure Boot Support

The system was developed on Ubuntu with Secure Boot enabled.

Because Secure Boot can prevent unsigned kernel modules from loading, a Module Signing approach was used.

A Machine Owner Key (MOK) key pair was generated, and the kernel module was signed using the private signing key.

The signed module was then enrolled through the MOK enrollment process.

This allowed the custom virtual sensor kernel module to load successfully while maintaining Secure Boot protection.

The private key file must be kept secure and must not be shared publicly.

---

## 9. Device Permissions

A dedicated Linux group named `vsensor` was created for controlled access to the virtual sensor device.

The device permissions were configured using a udev rule.

The resulting device was verified using:

ls -l /dev/virtual_sensor

The device appeared as a character device and was accessible to users belonging to the appropriate group.

This demonstrates Linux device permissions and controlled user-space access to a kernel device.

---

## 10. C++ Monitoring Application

The monitoring application was developed using C++17.

Its responsibilities include:

- Opening the virtual sensor device
- Reading sensor data
- Communicating with the driver
- Applying threshold rules
- Detecting abnormal conditions
- Generating alerts
- Recording events
- Managing simulation modes
- Providing monitoring control
- Serving dashboard/API information

The application follows a modular structure using separate components for device communication, monitoring logic, core functionality, and dashboard handling.

---

## 11. Simulation Modes

The system provides multiple simulation modes for testing different sensor conditions.

### NORMAL

Represents normal sensor operation.

### WARNING

Represents a sensor value approaching or exceeding the configured warning level.

### HIGH

Produces a high sensor value to demonstrate alert generation and threshold detection.

### LOW

Produces a low sensor value for testing another sensor operating condition.

These modes make it possible to test the monitoring system without requiring a physical sensor.

---

## 12. Threshold Detection

The monitoring application continuously checks the sensor value against the configured threshold.

The normal warning threshold used in the final configuration was:

70 °C

When the sensor value crosses the configured threshold, the monitoring application identifies the condition as abnormal.

For demonstration and testing, the threshold was temporarily reduced so that the HIGH simulation mode could trigger an alert. After testing, the threshold was restored to the final configured value.

This demonstrates configurable threshold-based monitoring using C++ system programming.

---

## 13. Alert Generation

When an abnormal sensor condition is detected, the monitoring application generates an alert.

The alert mechanism was tested using the HIGH simulation mode.

The system successfully detected the abnormal condition and generated the corresponding alert event.

The alert process demonstrates how user-space software can process data obtained from a Linux character device and apply application-level monitoring logic.

---

## 14. Event Logging

Detected events are recorded in a CSV log file:

logs/events.csv

The logging system records monitoring events such as abnormal sensor conditions and alerts.

CSV format was selected because it is simple, human-readable, and easy to inspect using text editors or spreadsheet applications.

This provides persistent event history for testing and demonstration.

---

## 15. Local Dashboard

A lightweight local dashboard was integrated into the C++ monitoring application.

The dashboard is accessible locally through:

http://localhost:8080

The dashboard displays information such as:

- Device availability
- Monitoring status
- Current simulation mode
- Sensor source
- Last error
- Logging status

The final tested dashboard status was:

device_available: true
monitoring: true
last_error: ""
log_error: ""
sim_mode: normal
source: kernel:/dev/virtual_sensor

The dashboard provides a convenient user-space interface for observing the monitoring system.

---

## 16. Testing Performed

The following components were tested during project integration:

| Test | Result |
|---|---|
| Driver compilation | PASS |
| C++ application compilation | PASS |
| Kernel module loading | PASS |
| Character device creation | PASS |
| Device permissions | PASS |
| Kernel-to-user communication | PASS |
| Monitoring start/stop | PASS |
| HIGH simulation mode | PASS |
| WARNING detection | PASS |
| Alert generation | PASS |
| CSV event logging | PASS |
| LOW simulation mode | PASS |
| NORMAL simulation mode | PASS |
| Configuration handling | PASS |
| Dashboard/API integration | PASS |

The final integrated test confirmed communication between the Linux kernel driver and the C++ monitoring application.

---

## 17. Problems Encountered and Solutions

### 17.1 Make Command Not Available

Initially, the `make` command was not installed.

Solution: The required build tools were installed and the project was compiled successfully.

### 17.2 Secure Boot Module Loading

Secure Boot prevented an unsigned custom kernel module from loading.

Solution: A MOK key pair was generated, the kernel module was signed, and the key was enrolled through the Secure Boot MOK process.

### 17.3 Device Permission Issue

Access to `/dev/virtual_sensor` initially required appropriate permissions.

Solution: A dedicated `vsensor` group and udev rule were configured, and the user was added to the group.

### 17.4 Dashboard Port Conflict

A second monitoring application instance attempted to use port 8080 while another instance was already running.

Solution: The running monitor instance was identified and the duplicate process was handled so that only the required dashboard instance used the port.

### 17.5 Alert Demonstration

The HIGH simulation value was below the original 70 °C threshold.

Solution: The threshold was temporarily reduced for demonstration, the alert was verified, and the threshold was restored to 70 °C.

---

## 18. System Programming Concepts Demonstrated

The project demonstrates several Linux system programming concepts:

- File descriptors
- Device file access
- System calls
- open()
- read()
- write()
- close()
- IOCTL communication
- Kernel-space and user-space interaction
- Process execution
- Signal handling
- File-based logging
- Error handling
- Resource management
- Multithreading/concurrent monitoring concepts
- Network socket/server programming for the local dashboard

These concepts connect the Linux kernel driver with the C++ user-space application.

---

## 19. Project File Structure

The main project structure is:

VirtualSensorProject/
|
|-- driver/
|   |-- virtual_sensor.c
|   |-- Makefile
|   `-- 99-virtual-sensor.rules
|
|-- include/
|   `-- vs_ioctl.h
|
|-- src/
|   |-- main.cpp
|   |-- core.hpp
|   |-- monitor.hpp
|   |-- sensor_device.hpp
|   `-- dashboard_server.hpp
|
|-- tests/
|   |-- test_core.cpp
|   `-- RESULTS.md
|
|-- logs/
|   `-- events.csv
|
|-- web/
|   `-- index.html
|
|-- build/
|   `-- monitor
|
|-- docs/
|   |-- stage1_project_introduction.md
|   |-- stage2_requirements_prd.md
|   |-- stage3_design_architecture.md
|   |-- stage4_implementation.md
|   |-- stage5_testing.md
|   `-- stage6_final_documentation.md
|
|-- Makefile
`-- README.md

The project is organized into separate driver, header, source, test, documentation, logging, and web components.

---

## 20. Limitations

The current project has some limitations:

1. The sensor is virtual and does not represent a physical sensor.
2. Sensor values are generated for simulation and testing.
3. The system currently focuses on a single virtual sensor device.
4. The dashboard is designed for local use.
5. The alert mechanism is intended for demonstration rather than production deployment.
6. The system does not currently provide persistent database storage.
7. The project does not include a production-grade authentication mechanism for dashboard access.

These limitations are acceptable within the academic scope of the project.

---

## 21. Future Scope

Possible future improvements include:

- Support for multiple virtual sensor devices
- More configurable sensor parameters
- Additional alert levels
- More advanced monitoring statistics
- Graphical sensor history
- Persistent database storage
- Improved dashboard functionality
- User authentication
- Configurable logging policies
- Additional Linux driver features
- Support for real hardware sensors in a separate extension
- Improved automated testing
- Packaging and installation scripts

These improvements could extend the project while maintaining its Linux system programming and device-driver focus.

---

## 22. Final System Status

The final integrated system successfully achieved the main project objectives.

Final status:

Linux character device driver       : WORKING
/dev/virtual_sensor                 : AVAILABLE
C++17 monitoring application        : WORKING
Kernel-to-user communication        : WORKING
Simulation modes                    : WORKING
Threshold detection                : WORKING
Alert generation                   : WORKING
CSV event logging                  : WORKING
Local dashboard                    : WORKING
Secure Boot module signing         : COMPLETED
Device permissions                 : CONFIGURED

The project successfully demonstrates a complete interaction between a Linux kernel module and a C++ user-space monitoring application.

---

## 23. Conclusion

The Virtual Sensor Monitoring & Alert System successfully demonstrates the integration of Linux Device Drivers, Linux System Programming, and C++ programming.

The project implements an actual Linux character device driver that creates /dev/virtual_sensor. The C++17 monitoring application communicates with this device, receives sensor information, processes the data, detects threshold violations, generates alerts, and records monitoring events.

The project also demonstrates practical Linux concepts including kernel module compilation and loading, device permissions, udev configuration, Secure Boot module signing, device-file communication, system calls, IOCTL communication, logging, and local socket/server programming.

Testing confirmed that the major components work together successfully. The final system therefore provides a practical academic demonstration of how a Linux kernel device driver can interact with a C++ user-space application to create a complete monitoring and alert system.
