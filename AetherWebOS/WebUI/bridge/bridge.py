#!/usr/bin/env python3
# The line above allows Linux servers to run this as an executable

import asyncio
import websockets
import json
import sys

# Configuration
QEMU_IP = '127.0.0.1'
QEMU_PORT = 1234
WS_PORT = 8080

connected_clients = set()
qemu_writer = None  # Global storage for the kernel's write pipe
standby_mode = True # Bridge starts in standby until F10 Wizard activation

async def qemu_to_websocket():
    global qemu_writer, standby_mode
    while True:
        try:
            print(f"[*] Connecting to Kernel at {QEMU_IP}:{QEMU_PORT}...")
            reader, writer = await asyncio.open_connection(QEMU_IP, QEMU_PORT)
            qemu_writer = writer 
            print("[+] Kernel Link established. Waiting for Setup Wizard (F10)...")

            while True:
                data = await reader.readline()
                if not data: break
                
                line = data.decode('utf-8', errors='ignore').strip()
                if not line: continue

                # Detect Activation Signal from F10 Wizard
                if line == "CMD_START_BRIDGE":
                    print("[!] AetherServer Setup Wizard: ACTIVATED.")
                    standby_mode = False
                    # Notify any connected WebUI clients that the bridge is now hot
                    if connected_clients:
                        msg = json.dumps({"type": "LOG", "payload": ">>> BRIDGE ACTIVATED BY KERNEL WIZARD <<<"})
                        websockets.broadcast(connected_clients, msg)
                    continue

                # If in standby, we do not forward data to the WebUI
                if standby_mode:
                    continue

                # Normal Data Processing
                if line.startswith("WEBUI_DATA:"):
                    try:
                        json_data = json.loads(line[11:])
                        msg = json.dumps({"type": "DATA", "payload": json_data})
                    except: continue
                else:
                    msg = json.dumps({"type": "LOG", "payload": line})

                if connected_clients:
                    # Manual broadcast for Python 3.14 stability
                    for client in list(connected_clients):
                        try:
                            await client.send(msg)
                        except:
                            connected_clients.remove(client)

        except (ConnectionRefusedError, OSError):
            print("[-] Kernel not found. Retrying...")
            qemu_writer = None
            standby_mode = True # Reset to standby if link drops
            await asyncio.sleep(2)
        except Exception as e:
            print(f"[!] Logic Error: {e}")
            qemu_writer = None
            await asyncio.sleep(1)

async def ws_handler(websocket):
    print(f"[+] WebUI Client Handshake Successful.")
    connected_clients.add(websocket)
    
    # Send current bridge status to new client
    status_msg = "Bridge Standby: Use F10 Setup Wizard in Kernel" if standby_mode else "Bridge Active"
    await websocket.send(json.dumps({"type": "LOG", "payload": f"[*] {status_msg}"}))

    try:
        async for message in websocket:
            if message == "SHUTDOWN_F7":
                print("[!] Shutdown signal received from WebUI.")
                if qemu_writer:
                    qemu_writer.write(b'\x1b[18~')
                    await qemu_writer.drain()
                    print("[OK] Sequence \\x1b[18~ transmitted to Kernel.")
                else:
                    print("[ERR] Kernel link not active.")
    except websockets.ConnectionClosed:
        pass
    finally:
        connected_clients.remove(websocket)
        print("[-] Client Disconnected.")

async def main():
    print(f"--- Aether Bridge v0.1.4 (Python {sys.version.split()[0]}) ---")
    async with websockets.serve(ws_handler, "localhost", WS_PORT):
        print(f"[*] WebSocket Server live on ws://localhost:{WS_PORT}")
        await qemu_to_websocket()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[!] Bridge Stopped.")