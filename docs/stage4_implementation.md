# Stage 4 – Implementation

## 1. Implementation Overview

The Virtual Sensor Monitoring & Alert System was implemented on Linux using C for the kernel device driver and C++17 for the user-space monitoring application.

The implementation was divided into the following components:

```text
Linux Kernel Driver
        ↓
Character Device
        ↓
C++ Device Interface
        ↓
Monitoring Application
        ↓
Threshold & Alert System
        ↓
CSV Logging
        ↓
Local Web Dashboard
```

## 2. Linux Character Device Driver

The driver is implemented in:

```text
driver/virtual_sensor.c
```

It is compiled as a Linux kernel module.

The driver creates the character device:

```text
/dev/virtual_sensor
```

The device provides an interface through which the C++ application can obtain simulated temperature readings.

## 3. Driver Operations

The driver implements the required character-device operations.

### Open

The device can be opened by the user-space monitoring application.

### Read

The `read()` operation provides the current simulated sensor value to the application.

### IOCTL

The driver supports `ioctl()` commands for device control.

These commands are defined in:

```text
include/vs_ioctl.h
```

The interface allows simulation mode control and sensor reset functionality.

## 4. Virtual Temperature Generation

The sensor values are generated inside the driver.

The implementation supports different simulation modes:

```text
NORMAL
HIGH
LOW
```

The generated value changes gradually rather than jumping directly between values.

Small variations are also introduced to make the simulated readings behave more like changing sensor data.

## 5. C++ User-Space Application

The main application is implemented using C++17.

Important source files include:

```text
src/main.cpp
src/sensor_device.hpp
src/core.hpp
src/monitor.hpp
src/dashboard_server.hpp
```

The application is compiled into:

```text
build/monitor
```

## 6. Device Abstraction

The `SensorDevice` interface separates the monitoring logic from the underlying sensor implementation.

The kernel-backed implementation communicates with:

```text
/dev/virtual_sensor
```

A mock device implementation is also available for unit testing and development.

The actual project demonstration uses the Linux kernel device.

## 7. Monitoring System

The monitoring component continuously reads sensor values from the device.

Each reading is processed and classified according to the configured thresholds.

The three states are:

```text
NORMAL
WARNING
CRITICAL
```

The monitoring system also maintains the current monitoring state and handles Start/Stop operations.

## 8. Threshold Configuration

The system supports configurable monitoring parameters.

Default configuration:

```text
Warning threshold : 70 °C
Critical offset   : 10 °C
Critical threshold: 80 °C
Sampling interval : 1000 ms
```

The configuration can be changed through the local API.

## 9. Alert Generation

When the temperature reaches or exceeds the configured warning threshold, the system changes the sensor state to WARNING.

When the temperature reaches the critical threshold, the system changes to CRITICAL.

The system avoids generating repeated alerts while the sensor remains at the same severity.

State changes are also recorded.

## 10. Event Logging

Sensor readings and important events are written to:

```text
logs/events.csv
```

The log records:

```text
timestamp
sensor
reading
threshold
status
event_type
message
```

Runtime CSV logs are excluded from Git using `.gitignore`.

## 11. Local Dashboard

The project includes a local web dashboard.

The dashboard is served by the C++ HTTP server implemented in:

```text
src/dashboard_server.hpp
```

The web interface is:

```text
web/index.html
```

The server runs on:

```text
127.0.0.1:8080
```

## 12. API Implementation

The application provides API endpoints for system monitoring and control.

### Status and Reading APIs

```text
GET /api/status
GET /api/readings/latest
GET /api/readings/history?limit=N
```

### Alert and Event APIs

```text
GET /api/alerts
GET /api/events
GET /api/events/export
```

### Configuration

```text
GET  /api/config
POST /api/config
```

### Simulation

```text
POST /api/simulation/mode
```

### Monitoring

```text
POST /api/monitoring/start
POST /api/monitoring/stop
```

## 13. Build System

The project uses GNU Make.

The main Makefile provides targets for:

```text
make driver
make app
make test
make clean
```

The driver has a separate Makefile under:

```text
driver/Makefile
```

The kernel module is built against the currently running Linux kernel.

## 14. Secure Boot Support

The development system has Secure Boot enabled.

The kernel module was signed using a locally generated Machine Owner Key (MOK), and the corresponding certificate was enrolled so that the Linux kernel could load the signed module.

The private signing key is excluded from the project repository using `.gitignore`.

## 15. Device Permissions

A udev rule is provided in:

```text
driver/99-virtual-sensor.rules
```

The rule assigns the device to the `vsensor` group and gives the group read/write permissions.

This allows the monitoring application to access the device without running the application as root.

## 16. Implementation Verification

The implementation was successfully verified on the development Linux system.

The following were confirmed:

- Driver compilation succeeded.
- `virtual_sensor.ko` was generated.
- Driver loading succeeded.
- `/dev/virtual_sensor` was created.
- Device permissions were configured.
- C++ monitor compiled successfully.
- Monitor successfully read data from the kernel device.
- HIGH simulation mode worked.
- LOW simulation mode worked.
- NORMAL mode worked.
- Warning threshold detection worked.
- Alert generation worked.
- CSV event logging worked.
- Dashboard/API integration worked.
- Monitoring Start and Stop operations worked.

## 17. Final Implementation Flow

```text
                 Linux Kernel Space
                       |
                       v
             +-------------------+
             | Virtual Sensor    |
             | Character Driver  |
             +---------+---------+
                       |
                       v
              /dev/virtual_sensor
                       |
=======================|========================
                       |
                 User Space
                       |
                       v
             +-------------------+
             | C++ SensorDevice  |
             +---------+---------+
                       |
                       v
             +-------------------+
             | C++ Monitor       |
             +---------+---------+
                       |
             +---------+---------+
             |                   |
             v                   v
      Threshold Logic       Event Logger
             |                   |
             v                   v
          Alerts             CSV Log
             |
             v
       Local Dashboard
```

## 18. Implementation Conclusion

The implementation successfully connects a Linux kernel character device driver with a C++17 user-space monitoring application.

The completed system demonstrates kernel/user-space communication, device control, continuous monitoring, threshold detection, alert generation, event logging, and local dashboard integration using only Linux and C/C++.
