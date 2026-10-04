# Stage 5 – Testing & Integration

## 1. Testing Overview

The Virtual Sensor Monitoring & Alert System was tested on Ubuntu Linux using the actual Linux character device driver and the C++17 monitoring application.

Testing covered driver compilation, device creation, user-space communication, simulation modes, threshold detection, alert generation, event logging, monitoring controls, and dashboard/API integration.

## 2. Test Environment

| Component | Configuration |
|---|---|
| Operating System | Ubuntu Linux |
| Kernel | 7.0.0-29-generic |
| Language | C and C++17 |
| Compiler | GCC/G++ |
| Build System | GNU Make |
| Device | `/dev/virtual_sensor` |
| Dashboard | `127.0.0.1:8080` |

## 3. Test Cases

### Test Case 1 – Driver Compilation

**Objective:** Verify that the Linux kernel module compiles successfully.

**Command:**

```text
make driver
```

**Expected Result:** Kernel module is generated.

**Actual Result:** `driver/virtual_sensor.ko` was successfully generated.

**Status:** PASS

---

### Test Case 2 – C++ Application Compilation

**Objective:** Verify that the monitoring application builds successfully.

**Command:**

```text
make app
```

**Expected Result:** Monitoring executable is generated.

**Actual Result:** `build/monitor` was successfully generated.

**Status:** PASS

---

### Test Case 3 – Driver Loading

**Objective:** Verify that the kernel module can be loaded.

**Command:**

```text
sudo insmod driver/virtual_sensor.ko
```

**Expected Result:** Driver loads successfully.

**Actual Result:** Driver loaded successfully.

**Status:** PASS

---

### Test Case 4 – Character Device Creation

**Objective:** Verify that the driver creates the expected device.

**Command:**

```text
ls -l /dev/virtual_sensor
```

**Expected Result:** `/dev/virtual_sensor` exists with appropriate permissions.

**Actual Result:** The device was created with the `vsensor` group and read/write permissions.

**Status:** PASS

---

### Test Case 5 – C++ Monitor Reading

**Objective:** Verify communication between the C++ application and the kernel driver.

**Method:** The monitor was started using the actual kernel-backed device.

**Expected Result:** Continuous simulated temperature readings are received.

**Actual Result:** The monitor continuously received readings from:

```text
kernel:/dev/virtual_sensor
```

Example monitoring output showed sequential simulated samples.

**Status:** PASS

---

### Test Case 6 – Monitoring Start

**Objective:** Verify that monitoring can be started.

**Method:** The monitoring Start API/dashboard control was tested.

**Expected Result:** Monitoring becomes active and new readings are processed.

**Actual Result:** The API returned:

```text
HTTP/1.1 200 OK
{"ok":true}
```

The status API subsequently reported:

```text
"monitoring": true
```

**Status:** PASS

---

### Test Case 7 – Monitoring Stop

**Objective:** Verify that monitoring can be stopped.

**Method:** The Stop API/dashboard control was tested.

**Expected Result:** Monitoring becomes inactive.

**Actual Result:** The API returned:

```text
HTTP/1.1 200 OK
{"ok":true}
```

The status API reported:

```text
"monitoring": false
```

**Status:** PASS

---

### Test Case 8 – HIGH Simulation Mode

**Objective:** Verify that the virtual sensor can enter HIGH simulation mode.

**Method:** HIGH mode was selected through the API.

**Expected Result:** Temperature gradually moves toward the high simulation range.

**Actual Result:** Readings around 60 °C were observed.

Example:

```text
temperature_c: 60.21
```

**Status:** PASS

---

### Test Case 9 – WARNING Threshold Detection

**Objective:** Verify threshold-based warning detection.

**Method:** The warning threshold was temporarily changed to 55 °C while HIGH simulation was active.

**Configuration:**

```text
Warning threshold: 55 °C
Critical offset: 10 °C
Sampling interval: 700 ms
```

**Expected Result:** A temperature above 55 °C should produce WARNING status.

**Actual Result:** A reading of approximately 59.67 °C produced:

```text
status: WARNING
```

**Status:** PASS

---

### Test Case 10 – Alert Generation

**Objective:** Verify that a threshold violation creates an alert.

**Expected Result:** An alert should be generated when the warning threshold is exceeded.

**Actual Result:** The alerts API returned an alert similar to:

```text
WARNING: 59.61 C reached limit 55.00 C
```

**Status:** PASS

---

### Test Case 11 – CSV Event Logging

**Objective:** Verify persistent event logging.

**Expected Result:** Sensor readings and events should be written to the CSV log.

**Actual Result:** `logs/events.csv` contained timestamped readings and WARNING events.

Example structure:

```text
timestamp,virtual_temp,60.62,55.00,WARNING,READING,...
```

**Status:** PASS

---

### Test Case 12 – LOW Simulation Mode

**Objective:** Verify LOW simulation mode.

**Expected Result:** Temperature should move toward a lower simulated range.

**Actual Result:** A reading of approximately 12.17 °C was observed while LOW mode was active.

**Status:** PASS

---

### Test Case 13 – NORMAL Simulation Mode

**Objective:** Verify return to NORMAL simulation mode.

**Expected Result:** The system returns to normal simulation behavior.

**Actual Result:** NORMAL mode was successfully selected and the final system status reported:

```text
"sim_mode": "normal"
```

**Status:** PASS

---

### Test Case 14 – Configuration Restoration

**Objective:** Verify that the normal monitoring configuration can be restored after testing.

**Final configuration:**

```text
Warning threshold : 70 °C
Critical offset   : 10 °C
Critical threshold: 80 °C
Sampling interval : 1000 ms
```

**Status:** PASS

## 4. Integration Testing

The complete pipeline was tested together:

```text
Virtual Sensor
      ↓
Kernel Character Driver
      ↓
/dev/virtual_sensor
      ↓
C++ Monitoring Application
      ↓
Threshold Detection
      ↓
Alert Generation
      ↓
CSV Logging
      ↓
Dashboard/API
```

The integrated system successfully transferred simulated readings from the kernel driver to the C++ application and then to the monitoring and dashboard components.

## 5. Final System Status

At the end of testing, the system was returned to NORMAL simulation mode.

The final status confirmed:

```text
device_available: true
monitoring: true
last_error: ""
log_error: ""
sim_mode: normal
source: kernel:/dev/virtual_sensor
```

## 6. Testing Summary

| Area | Result |
|---|---|
| Driver compilation | PASS |
| Application compilation | PASS |
| Driver loading | PASS |
| Character device | PASS |
| Device permissions | PASS |
| Kernel-to-user communication | PASS |
| Monitoring Start | PASS |
| Monitoring Stop | PASS |
| HIGH mode | PASS |
| WARNING detection | PASS |
| Alert generation | PASS |
| CSV logging | PASS |
| LOW mode | PASS |
| NORMAL mode | PASS |
| Configuration | PASS |
| Dashboard/API integration | PASS |

## 7. Issues Encountered and Resolutions

### Issue 1 – Device Permission

Initially, access to the character device required correct group permissions.

**Resolution:** A `vsensor` group and udev rule were configured, and the user was added to the group.

### Issue 2 – Secure Boot

Secure Boot required the kernel module to be signed before it could be loaded.

**Resolution:** A Machine Owner Key was generated, enrolled, and used to sign the kernel module.

### Issue 3 – Dashboard Port Conflict

Starting a second monitor instance caused port 8080 to already be in use.

**Resolution:** The existing monitor instance was identified and reused/stopped before starting another instance.

### Issue 4 – Alert Demonstration

The default warning threshold of 70 °C was above the HIGH simulation range of approximately 60 °C.

**Resolution:** The warning threshold was temporarily reduced to 55 °C for testing. After the alert was verified, the configuration was restored to 70 °C.

## 8. Conclusion

Testing confirmed that the major components of the Virtual Sensor Monitoring & Alert System work together successfully.

The project was tested using the actual Linux character device driver rather than relying only on the user-space mock implementation.

The system successfully demonstrates Linux device-driver communication, C++ system programming, threshold monitoring, alert generation, event logging, and local dashboard integration.
