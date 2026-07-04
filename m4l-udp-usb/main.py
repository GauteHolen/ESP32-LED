from pythonosc.dispatcher import Dispatcher
from pythonosc import osc_server
import serial
from queue import Queue
import threading
import time
from collections import deque
from serial.tools import list_ports

ports = list_ports.comports()

for i,p in enumerate(ports):
    print(f"Port {i}:")
    print(f"  Port: {p.device}")
    print(f"  Description: {p.description}")
    print(f"  HWID: {p.hwid}")
    print()

port_number = int(input("Select the port number to use: "))

print(f"Using port: {ports[port_number].device}")

SERIAL_PORT = ports[port_number].device
ser = serial.Serial(SERIAL_PORT, 115200)

stats_history = deque(maxlen=10)


q = Queue(maxsize=10000)  # prevents memory explosion

def handle_fixture(address, *args):


    if len(args) != 3:
        print("wrong number of args")
        return

    fixture, control, value = args

    # clamp + ensure int
    fixture = int(fixture) & 0xFF
    control = int(control) & 0xFF
    value   = int(value) & 0xFFFF  # 16-bit!

    value_hi = (value >> 8) & 0xFF
    value_lo = value & 0xFF

    packet = bytes([255, control, value_hi, value_lo, fixture])

    try:
        q.put_nowait(packet)
    except:
        pass  # queue full → drop old data instead of crashing

def serial_worker():
    global ser
    while True:
        packet = q.get()
        while True:
            try:
                ser.write(packet)
                increment_count()
                break
            except serial.SerialException as e:
                print(f"\nSerial error: {e} — retrying in 1s...")
                time.sleep(1)
                try:
                    ser.close()
                except Exception:
                    pass
                try:
                    ser = serial.Serial(SERIAL_PORT, 115200)
                except serial.SerialException:
                    pass  # still not available, will retry

def increment_count():
    global count, bytes_sent
    count += 1
    bytes_sent += 5

count = 0
bytes_sent = 0
def stats_thread():
    global count, bytes_sent
    while True:
        time.sleep(1)
        line = f"RX: {count} msg/s - {bytes_sent} bytes/s"

        stats_history.append(line)

        # clear screen (optional, makes it nicer)
        #print("\033[H\033[J", end="")  # ANSI clear

        print(line,end="\r")  # print on same line

        count = 0
        bytes_sent = 0

threading.Thread(target=stats_thread, daemon=True).start()
threading.Thread(target=serial_worker, daemon=True).start()

dispatcher = Dispatcher()

# map OSC address
dispatcher.map("/fixture", handle_fixture)

server = osc_server.ThreadingOSCUDPServer(("127.0.0.1", 5005), dispatcher)

print("Listening for OSC on 127.0.0.1:5005")
server.serve_forever()