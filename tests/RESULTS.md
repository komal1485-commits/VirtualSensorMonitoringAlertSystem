# Actual test results (run 2026-10-02, dev sandbox: Ubuntu 24.04, g++ 13.3, kernel 6.18 WITHOUT headers)
- `make test`: 33/33 checks passed (threshold classification + hysteresis, input validation, alert generation,
  no duplicate alerts, device-failure handling, log persistence after restart, log-file error handling) — mock device.
- HTTP smoke (curl, mock): index 200; status/latest/config OK; invalid threshold -> 400; invalid mode -> 400;
  high mode produced WARNING then CRITICAL alert; 404 handled; CSV export OK; SIGINT -> "shut down cleanly".
- Real-device path with no /dev/virtual_sensor: see run notes (status reports device_available=false, no crash).
## NOT tested
- Kernel driver build/insmod/rmmod/read/ioctl (no kernel headers in sandbox).
- Dashboard in a real browser (served correctly; rendering not visually verified).
- Performance measurements.
