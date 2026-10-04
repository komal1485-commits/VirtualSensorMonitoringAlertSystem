# Virtual Sensor Monitoring & Alert System

A Linux-based software project that simulates a temperature sensor and demonstrates **Linux Character Device Driver, System Programming, C++, threshold monitoring, event logging, and a local web dashboard**.

**All sensor readings are simulated in software. This project does not use physical sensors or IoT hardware.**

## 1. Project Objective

The objective of this project is to develop a complete Linux-based sensor monitoring system using a custom character device driver.

The system generates simulated temperature readings inside a Linux kernel module, exposes them through the character device `/dev/virtual_sensor`, and allows a C++17 user-space application to read and monitor the data.

The application detects threshold violations, generates alerts, records events, and provides a local web dashboard for monitoring and control.

## 2. System Architecture

```text
Software Virtual Sensor
        |
        v
Linux Character Device Driver
        |
        v
 /dev/virtual_sensor
        |
        v
   C++17 Monitor
        |
        v
Threshold Detection
    /         \
   /           \
NORMAL       WARNING / CRITICAL
                 |
                 v
            Alert Logging
                 |
                 v
          Local Web Dashboard
```

## 3. Main Features

- Custom Linux character device driver
- Virtual temperature sensor implemented in software
- Character device: `/dev/virtual_sensor`
- C/C++ based implementation
- C++17 user-space monitoring application
- `read()` based sensor data retrieval
- `ioctl()` based device control
- NORMAL, WARNING and CRITICAL states
- Configurable warning threshold
- Configurable critical offset
- HIGH, LOW and NORMAL simulation modes
- Alert generation when severity increases
- Event logging in CSV format
- Local HTTP web dashboard
- REST-style JSON API
- Monitoring Start/Stop controls
- Device availability monitoring
- User-space mock mode for development/testing

## 4. Technologies Used

| Technology | Purpose |
|---|---|
| C | Linux kernel character device driver |
| C++17 | User-space monitoring and system programming |
| Linux | Project development and execution environment |
| Linux Kernel Module | Virtual sensor implementation |
| Character Device | Communication between kernel and user space |
| ioctl | Device control and simulation mode management |
| HTTP/JSON | Local dashboard and API |
| CSV | Event logging |
| Make | Build automation |

## 5. Project Structure

```text
VirtualSensorProject/
|
├── driver/
│   ├── virtual_sensor.c
│   ├── Makefile
│   └── 99-virtual-sensor.rules
|
├── include/
│   └── vs_ioctl.h
|
├── src/
│   ├── core.hpp
│   ├── dashboard_server.hpp
│   ├── main.cpp
│   ├── monitor.hpp
│   └── sensor_device.hpp
|
├── tests/
│   ├── test_core.cpp
│   └── RESULTS.md
|
├── web/
│   └── index.html
|
├── docs/
│   └── Project documentation
|
├── logs/
│   └── Runtime CSV logs (ignored by Git)
|
├── Makefile
├── README.md
└── .gitignore
```

## 6. Building the Project

The project must be built on Linux.

Install the required build tools and kernel headers:

```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

Build the kernel driver:

```bash
make driver
```

Build the C++ application:

```bash
make app
```

The driver module will be generated as:

```text
driver/virtual_sensor.ko
```

The monitoring application will be generated as:

```text
build/monitor
```

## 7. Loading the Kernel Driver

Load the driver:

```bash
sudo insmod driver/virtual_sensor.ko
```

Verify that the device exists:

```bash
ls -l /dev/virtual_sensor
```

The device is configured to use the `vsensor` group.

Set up the group if required:

```bash
sudo groupadd -f vsensor
sudo usermod -aG vsensor $USER
```

Log out and log back in after adding the user to the group.

The project includes the udev rule:

```text
driver/99-virtual-sensor.rules
```

Install the rule:

```bash
sudo cp driver/99-virtual-sensor.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
```

After loading the driver, the device should be available as:

```text
/dev/virtual_sensor
```

## 8. Running the Monitor

Run the C++ monitoring application:

```bash
./build/monitor --verbose
```

The application reads the virtual sensor through:

```text
/dev/virtual_sensor
```

The local dashboard is available at:

```text
http://127.0.0.1:8080/
```

The server is bound to localhost and is intended for local demonstration.

## 9. Simulation Modes

The driver supports simulated temperature modes.

### NORMAL

Produces readings around the normal operating range.

### HIGH

Moves the simulated temperature toward a high temperature range.

### LOW

Moves the simulated temperature toward a low temperature range.

The dashboard/API can change the simulation mode.

Example:

```bash
curl -s -X POST http://127.0.0.1:8080/api/simulation/mode \
-H "Content-Type: application/json" \
-d '{"mode":"high"}'
```

Return to normal:

```bash
curl -s -X POST http://127.0.0.1:8080/api/simulation/mode \
-H "Content-Type: application/json" \
-d '{"mode":"normal"}'
```

## 10. Threshold and Alert System

The monitoring application classifies readings into:

```text
NORMAL
WARNING
CRITICAL
```

The warning threshold and critical offset can be configured dynamically.

Default configuration:

```text
Warning threshold : 70 °C
Critical offset   : 10 °C
Critical threshold: 80 °C
Sampling interval : 1000 ms
```

An alert is generated when the sensor state rises to a higher severity.

The system also uses hysteresis to reduce repeated state transitions around a threshold.

## 11. Event Logging

Runtime events are recorded in:

```text
logs/events.csv
```

The log contains:

```text
timestamp,
sensor,
reading_c,
threshold_c,
status,
event_type,
message
```

Example event:

```text
WARNING: temperature reached the configured warning threshold
```

The CSV log is excluded from Git using `.gitignore` because it is generated runtime data.

## 12. API Endpoints

### GET

```text
/api/status
/api/readings/latest
/api/readings/history?limit=N
/api/alerts
/api/config
/api/events
/api/events/export
```

### POST

```text
/api/config
/api/simulation/mode
/api/monitoring/start
/api/monitoring/stop
```

Example status request:

```bash
curl -s http://127.0.0.1:8080/api/status
```

## 13. Testing Performed

The completed implementation was tested on Ubuntu with the actual kernel driver.

Verified components include:

- Kernel module compilation
- Kernel module loading
- Character device creation
- `/dev/virtual_sensor` permissions
- C++ application reading from the kernel device
- Continuous simulated sensor readings
- Monitoring Start operation
- Monitoring Stop operation
- HIGH simulation mode
- LOW simulation mode
- NORMAL simulation mode
- Configurable warning threshold
- WARNING alert generation
- Alert API
- CSV event logging
- Dynamic configuration
- Local dashboard/API integration

A warning alert was demonstrated by temporarily setting the warning threshold below the simulated HIGH temperature.

The configuration was subsequently restored to:

```text
Warning threshold : 70 °C
Critical offset   : 10 °C
Critical threshold: 80 °C
Sampling interval : 1000 ms
```

## 14. Security / Secure Boot

The kernel module was tested on a system with Secure Boot enabled.

A Machine Owner Key (MOK) was used to sign the kernel module for local loading.

**Private signing keys are excluded from the Git repository and must never be uploaded to GitHub.**

## 15. Mock Mode

For development and unit testing, the application also supports a user-space mock sensor:

```bash
./build/monitor --mock
```

Mock mode does **not** represent the Linux kernel driver.

The primary demonstration uses the actual:

```text
Linux Kernel Module
        ↓
/dev/virtual_sensor
        ↓
C++ Monitor
```

## 16. Cleanup

Stop the monitoring application with:

```text
Ctrl+C
```

Unload the kernel module:

```bash
sudo rmmod virtual_sensor
```

Clean generated build artifacts:

```bash
make clean
```

## 17. Limitations

- Sensor values are software-generated rather than measured from physical hardware.
- The dashboard is intended for local demonstration.
- The HTTP server is a lightweight project-level implementation rather than a production web server.
- The virtual sensor represents a single temperature sensor.

## 18. Future Improvements

Possible future extensions include:

- Multiple virtual sensor devices
- Additional sensor types
- More advanced device-driver controls
- Improved dashboard visualization
- Persistent database storage
- More extensive automated driver testing
- Service management using systemd

## 19. Project Status

**Project implementation completed and tested on Linux.**

The project demonstrates the complete flow:

```text
Virtual Sensor
      ↓
Linux Character Device Driver
      ↓
/dev/virtual_sensor
      ↓
C++17 System Programming
      ↓
Threshold Monitoring
      ↓
Alerts
      ↓
CSV Event Logging
      ↓
Local Web Dashboard
```
