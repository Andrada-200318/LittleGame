#!/usr/bin/env python3
"""Fake player for testing the server without phones. Not part of the submission.

Usage: python tools/bot.py <ip> <password> <name> [--port 5000] [--auto]

Connects, sends JOIN, prints every line it receives, and sends every line you type. Type broken
lines on purpose to see how the server reacts. Ctrl+C or Ctrl+Z then Enter quits without LEAVE, which
looks like a dropped connection to the server.

With --auto it also answers every round by itself, 1 to 4 seconds after ROUND_START. Run 3 or 4 in
separate terminals to play full rounds without phones.
"""

import argparse
import os
import random
import socket
import sys
import threading
import time

AUTO_ANSWERS = ["Pizza", "Sarmale", "Seven", "Italy", "Three or four", "Tiramisu", "Not sure", "Japan"]


def send_line(sock, line):
    sock.sendall((line + "\n").encode("utf-8"))


def answer_later(sock, round_id):
    time.sleep(random.uniform(1, 4))
    try:
        send_line(sock, f"ANSWER|{round_id}|{random.choice(AUTO_ANSWERS)}")
    except OSError:
        pass


def receive_lines(sock, auto):
    buffer = b""
    while True:
        try:
            data = sock.recv(4096)
        except OSError:
            data = b""
        if not data:
            print("[connection closed]", flush=True)
            os._exit(0)
        buffer += data
        while b"\n" in buffer:
            line, buffer = buffer.split(b"\n", 1)
            text = line.decode("utf-8", "replace").rstrip("\r")
            print("<- " + text, flush=True)
            if auto and text.startswith("ROUND_START|"):
                round_id = text.split("|", 3)[1]
                threading.Thread(target=answer_later, args=(sock, round_id), daemon=True).start()


def main():
    parser = argparse.ArgumentParser(description="Fake player for the Little Impostor Game server.")
    parser.add_argument("ip")
    parser.add_argument("password")
    parser.add_argument("name")
    parser.add_argument("--port", type=int, default=5000)
    parser.add_argument("--auto", action="store_true", help="answer every round automatically")
    args = parser.parse_args()

    sys.stdin.reconfigure(encoding="utf-8")
    sys.stdout.reconfigure(encoding="utf-8")

    sock = socket.create_connection((args.ip, args.port))
    threading.Thread(target=receive_lines, args=(sock, args.auto), daemon=True).start()

    send_line(sock, f"JOIN|{args.password}|{args.name}")

    try:
        for line in sys.stdin:
            send_line(sock, line.rstrip("\r\n"))
        while args.auto:
            time.sleep(1)
    except (KeyboardInterrupt, OSError):
        pass
    sock.close()


if __name__ == "__main__":
    main()
