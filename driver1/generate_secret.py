#!/usr/bin/env python3
import secrets
import textwrap

def generate_secret(num_bytes=128):
    data = [f"0x{secrets.randbits(8):02X}" for _ in range(num_bytes)]
    # Group bytes in lines of 8 for readability
    grouped = textwrap.wrap(", ".join(data), 8 * 6)
    print("static const UCHAR g_AttestationSecret[128] = {")
    for line in grouped:
        print("    " + line)
    print("};")

if __name__ == "__main__":
    generate_secret()
