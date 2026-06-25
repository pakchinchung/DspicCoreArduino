#!/usr/bin/env python
"""Tiny serial monitor for hardware self-test: read N seconds from a COM port.
Usage: python read_serial.py [PORT] [BAUD] [SECONDS]"""
import sys, time
import serial

port = sys.argv[1] if len(sys.argv) > 1 else "COM6"
baud = int(sys.argv[2]) if len(sys.argv) > 2 else 9600
secs = float(sys.argv[3]) if len(sys.argv) > 3 else 8.0

s = serial.Serial(port, baud, timeout=1)
print(f"# listening on {port} @ {baud} for {secs}s", flush=True)
start = time.time()
end = start + secs
while time.time() < end:
    line = s.readline().decode(errors="replace").rstrip()
    if line:
        # Prefix host elapsed seconds so timing can be cross-checked vs device millis().
        print(f"[{time.time() - start:6.2f}s] {line}", flush=True)
s.close()
