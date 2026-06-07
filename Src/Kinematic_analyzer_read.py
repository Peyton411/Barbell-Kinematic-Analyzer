# -*- coding: utf-8 -*-
"""
Created on Sun May 31 13:59:50 2026

@author: Peyto
"""

import asyncio
from bleak import BleakClient, BleakScanner
import struct
import csv
import time
import numpy as np
import matplotlib.pyplot as plt
import nest_asyncio
nest_asyncio.apply()
from collections import deque
from threading import Thread, Lock

# BLE settings
DEVICE_NAME = "DSD TECH"
CHAR_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"
AXES = 6
BYTES_PER_PACKET = AXES * 4  #24 of them

# File output
output_file = "kinematic_data.csv"
header = ["timestamp_ms", "now_val", "vz_val", "fake1", "fake2", "fake3", "fake4"]
csv_file = open(output_file, mode='w', newline='')
csv_writer = csv.writer(csv_file)
csv_writer.writerow(header)

# Plotting buffer
plot_window_size = 200
data_buffer = deque(maxlen=plot_window_size)
buffer_lock = Lock()

# My addition, buffer
# So basically the BLE can send things tht are not 24 bytes, so this just
#accumulates everything until it has 24 bytes
byte_buffer = bytearray()
byte_buffer_lock = Lock()
# ─────────────────────────────────────────────────────────────────────────────

def handle_notification(_, data):
    global byte_buffer

    with byte_buffer_lock:
        byte_buffer.extend(data)  # adds the bytes together

        # Continue if 24 bytes
        while len(byte_buffer) >= BYTES_PER_PACKET:

            # Takes 24 bytes and leaves leftovers
            packet_bytes = byte_buffer[:BYTES_PER_PACKET]
            byte_buffer = byte_buffer[BYTES_PER_PACKET:]  # Move buffer

            now_ms = int(time.time() * 1000)

            try:
                # Unpack the floats now_val, vz_val, fake1, fake2, fake3, and fake4
                floats = struct.unpack("<6f", packet_bytes)
                now_val, vz_val = floats[0], floats[1]

                row = [now_ms] + list(floats)
                csv_writer.writerow(row)

                with buffer_lock:
                    # Store the ones I want, now (time), and velocity
                    data_buffer.append([now_val, vz_val])

            except Exception as e:
                print(" Data parsing error:", e)

#BLE loop
async def ble_loop():
    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover(timeout=5.0)
    target = next((d for d in devices if d.name and DEVICE_NAME in d.name), None)

    if not target:
        print(f" Device named '{DEVICE_NAME}' not found.")
        csv_file.close()
        return

    print(f"Connecting to {target.name} ({target.address})...")
#My addition, sending signals to the arduino so it can start doing its thing
    async with BleakClient(target, timeout=20.0) as client:
        try:
            # Send C to confirm connection and S to start
            print("Sending connection signal 'C'...")
            await client.write_gatt_char(CHAR_UUID, b'C')
            await asyncio.sleep(0.5)

            print("Sending start signal 'S'...")
            await client.write_gatt_char(CHAR_UUID, b'S')

            # Pausing so that the IMU calibration can finish
            print("Keep still, IMU is calibrating")
            await asyncio.sleep(4.0)

            # 3. Python can now start doing its thing and writing CSV files
            print("Starting to receive...")
            await client.start_notify(CHAR_UUID, handle_notification)

            print("Receiving data, control C to stop")

            while True:
                await asyncio.sleep(1)

        except KeyboardInterrupt:
            print("\n Stopped")
        finally:
            await client.stop_notify(CHAR_UUID)
            csv_file.close()
            print("💾 Disconnected and file saved.")

# Real-time plot
def plot_realtime():
    plt.ion()
    fig, (vel_ax, time_ax) = plt.subplots(2, 1, figsize=(10, 6), sharex=True)

    vel_line, = vel_ax.plot([], [], color='steelblue')
    time_line, = time_ax.plot([], [], color='orange')

    vel_ax.set_ylim(-5, 5)
    vel_ax.set_ylabel("Velocity Z (m/s)")
    vel_ax.set_title("Vertical Velocity (vz)")
    vel_ax.grid(True)

    time_ax.set_ylabel("Arduino time (ms as float)")
    time_ax.set_title("now_val from Arduino")
    time_ax.grid(True)
    time_ax.set_xlabel("Sample Index")

    def update_plot():
        while True:
            with buffer_lock:
                if len(data_buffer) > 0:
                    data_np = np.array(data_buffer)
                    t = np.arange(len(data_np))
                    vel_line.set_data(t, data_np[:, 1])   # vz_val
                    time_line.set_data(t, data_np[:, 0])  # now_val
                    vel_ax.set_xlim(0, max(1, len(data_np)))
                    time_ax.set_xlim(0, max(1, len(data_np)))
                    time_ax.relim()
                    time_ax.autoscale_view(scalex=False, scaley=True)
            fig.canvas.draw()
            fig.canvas.flush_events()
            time.sleep(0.05)

    update_plot()
# Start plotting in a thread
plot_thread = Thread(target=plot_realtime, daemon=True)
plot_thread.start()
# Run BLE loop
if __name__ == "__main__":
    asyncio.run(ble_loop())