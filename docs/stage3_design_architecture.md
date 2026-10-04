# Stage 3 – Design, Architecture & UML

## 1. Design Overview

The Virtual Sensor Monitoring & Alert System is designed using a layered architecture that separates the Linux kernel driver, user-space monitoring logic, and local dashboard.

The system follows the flow:

```text
Virtual Sensor
      ↓
Character Device Driver
      ↓
/dev/virtual_sensor
      ↓
C++ Device Interface
      ↓
Monitoring Logic
      ↓
Threshold Detection
      ↓
Alerts + Event Logging
      ↓
HTTP API + Web Dashboard
```

## 2. System Architecture

```text
+--------------------------------------------------+
|                 Web Dashboard                   |
|                  web/index.html                 |
+-------------------------+------------------------+
                          |
                          | HTTP / JSON
                          ↓
+--------------------------------------------------+
|              C++ Dashboard Server              |
|            dashboard_server.hpp                |
+-------------------------+------------------------+
                          |
                          ↓
+--------------------------------------------------+
|                C++ Monitor                     |
|                 monitor.hpp                     |
|                                                  |
|  Reading → Processing → Threshold → Alert       |
+-------------------------+------------------------+
                          |
                          ↓
+--------------------------------------------------+
|             Sensor Device Interface             |
|                sensor_device.hpp                |
|                                                  |
|       open() / read() / ioctl()                 |
+-------------------------+------------------------+
                          |
                    User Space
==================================================
                    Kernel Space
                          |
                          ↓
+--------------------------------------------------+
|          Linux Character Device Driver           |
|               virtual_sensor.c                  |
|                                                  |
|   Virtual Temperature Generation                |
|   File Operations                               |
|   ioctl Controls                                |
+-------------------------+------------------------+
                          |
                          ↓
                 /dev/virtual_sensor
```

## 3. Major Components

### 3.1 Virtual Sensor Driver

File:

```text
driver/virtual_sensor.c
```

The driver is implemented as a Linux kernel module.

It generates simulated temperature readings and exposes them through a character device.

The driver supports:

- Device registration
- Device creation
- Read operation
- Device control using `ioctl()`
- Simulation modes
- Sensor reset
- Module initialization
- Module cleanup

### 3.2 Shared Device Interface

File:

```text
include/vs_ioctl.h
```

This header defines the interface shared between the kernel driver and the C++ user-space application.

It contains the `ioctl()` command definitions and related structures/constants.

### 3.3 Sensor Device Interface

File:

```text
src/sensor_device.hpp
```

This component provides the user-space abstraction for accessing the virtual sensor.

The main implementation communicates with:

```text
/dev/virtual_sensor
```

A mock implementation is also available for development and unit testing.

### 3.4 Monitoring Logic

Files:

```text
src/core.hpp
src/monitor.hpp
```

These components process the sensor readings.

The system determines the current state based on configurable thresholds:

```text
NORMAL
WARNING
CRITICAL
```

The monitoring component also controls the monitoring loop and alert generation.

### 3.5 Event Logger

The monitoring system records readings and state changes in CSV format.

The log contains:

- Timestamp
- Sensor
- Reading
- Threshold
- Status
- Event type
- Message

### 3.6 Dashboard Server

File:

```text
src/dashboard_server.hpp
```

The project includes a lightweight local HTTP server.

It provides JSON API endpoints for:

- Current status
- Latest reading
- Reading history
- Alerts
- Configuration
- Events
- Monitoring control
- Simulation mode control

### 3.7 Web Interface

File:

```text
web/index.html
```

The web interface provides a visual dashboard for interacting with the monitoring system.

## 4. Kernel Space and User Space

The project is divided into two major execution areas.

### Kernel Space

The Linux character device driver executes in kernel space.

It is responsible for:

- Creating the character device
- Generating simulated sensor data
- Handling device operations
- Processing `ioctl()` commands

### User Space

The C++ application executes in user space.

It is responsible for:

- Reading sensor data
- Monitoring readings
- Detecting threshold violations
- Generating alerts
- Logging events
- Providing the local dashboard/API

The character device acts as the communication boundary between the two.

## 5. Data Flow

The data flow is:

```text
1. Driver generates simulated temperature
             ↓
2. Driver exposes reading through character device
             ↓
3. C++ application opens /dev/virtual_sensor
             ↓
4. C++ application reads the value
             ↓
5. Monitoring logic evaluates threshold
             ↓
6. State is classified as NORMAL/WARNING/CRITICAL
             ↓
7. Alert/event is generated when required
             ↓
8. Event is written to CSV log
             ↓
9. Dashboard displays current system information
```

## 6. Threshold State Machine

The monitoring logic uses three primary states:

```text
                 temperature
                      |
          +-----------+-----------+
          |                       |
       < Warning              >= Warning
          |                       |
          ↓                       ↓
       NORMAL                  WARNING
                                  |
                              >= Critical
                                  |
                                  ↓
                              CRITICAL
```

The system uses hysteresis when returning from higher severity states to reduce repeated transitions near a threshold.

## 7. UML – Use Case Description

### Actor

**User**

### Main Use Cases

```text
User
 |
 +--> Load Driver
 |
 +--> Start Monitoring
 |
 +--> View Sensor Reading
 |
 +--> Change Simulation Mode
 |
 +--> Configure Threshold
 |
 +--> View Alerts
 |
 +--> View Event Logs
 |
 +--> Stop Monitoring
 |
 +--> Unload Driver
```

## 8. UML – Component View

```text
+-------------------+
|   Web Dashboard   |
+---------+---------+
          |
          v
+-------------------+
| Dashboard Server  |
+---------+---------+
          |
          v
+-------------------+
| C++ Monitor       |
+---------+---------+
          |
          v
+-------------------+
| SensorDevice      |
+---------+---------+
          |
          v
+-------------------+
| Character Device  |
| /dev/virtual_sensor|
+---------+---------+
          |
          v
+-------------------+
| Linux Kernel      |
| Driver            |
+-------------------+
```

## 9. Design Decisions

### Software-based Sensor

A virtual sensor was selected so that the project can demonstrate device-driver functionality without requiring physical hardware.

### Character Device

A character device was selected because sensor readings are naturally handled as a stream of data and it provides a clear kernel/user-space interface.

### C for Driver

The Linux kernel driver is implemented in C because the Linux kernel provides its driver-development interfaces primarily through C.

### C++ for Application

The user-space monitoring system is implemented in C++17 to demonstrate object-oriented design and system programming.

### Local Dashboard

A local dashboard was selected to demonstrate interaction with the monitoring system without introducing cloud or IoT dependencies.

## 10. Development Environment

The project was developed and tested on:

- Ubuntu Linux
- Linux kernel 7.0.0-29-generic
- GCC/G++
- C++17
- GNU Make
- Linux kernel headers
- Git/GitHub for version control

## 11. Design Summary

The design separates responsibilities between kernel space and user space.

The Linux driver handles virtual device operations, while the C++ application handles monitoring, threshold processing, alerts, logging, and dashboard functionality.

This separation provides a clear demonstration of Linux Device Driver and System Programming concepts.
