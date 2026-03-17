#!/usr/bin/env python3
import socket

HOST = "localhost"  # Listen on all interfaces
PORT = 5000       # Port to listen on

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # <-- allow reuse
    s.bind((HOST, PORT))
    s.listen()

    print(f"Server listening on {HOST}:{PORT}...")
    while True:
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            while True:
                data = conn.recv(1024)
                if not data:
                    break
                message = data.decode("utf-8", errors="ignore")
                print(f"Received: {message}")
                reply = f"You sent: {message}"
                conn.sendall(reply.encode("utf-8"))

