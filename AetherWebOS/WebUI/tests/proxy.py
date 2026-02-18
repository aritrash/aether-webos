import socket
import threading

# CONFIGURATION
LISTEN_PORT = 9090      # Port Chrome will hit
TARGET_IP = '127.0.0.1' # QEMU Loopback
TARGET_PORT = 8080       # Port Aether is listening on (inside QEMU)

def hex_dump(data):
    return ' '.join(f'{b:02x}' for b in data)

def proxy_flow(source, destination, direction):
    while True:
        try:
            data = source.recv(4096)
            if not data:
                break
            
            print(f"\n[{direction}] Size: {len(data)} bytes")
            if b'GET' in data or b'HTTP' in data:
                print(f"--- CONTENT ---\n{data.decode(errors='ignore')}\n---------------")
            else:
                print(f"--- HEX DUMP ---\n{hex_dump(data[:64])}...")

            destination.sendall(data)
        except Exception as e:
            print(f"[{direction}] Connection Closed: {e}")
            break

def start_proxy():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind(('0.0.0.0', LISTEN_PORT))
    server.listen(5)
    print(f"[*] Aether Debug Proxy active on port {LISTEN_PORT}...")
    print(f"[*] Point Chrome to http://localhost:{LISTEN_PORT}")

    while True:
        client_sock, addr = server.accept()
        print(f"[*] Browser Connected: {addr}")
        
        # Connect to QEMU
        try:
            qemu_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            qemu_sock.connect((TARGET_IP, TARGET_PORT))
        except:
            print("[!] Error: Could not connect to QEMU. Is the kernel running?")
            client_sock.close()
            continue

        threading.Thread(target=proxy_flow, args=(client_sock, qemu_sock, "BROWSER -> AETHER")).start()
        threading.Thread(target=proxy_flow, args=(qemu_sock, client_sock, "AETHER -> BROWSER")).start()

if __name__ == "__main__":
    start_proxy()