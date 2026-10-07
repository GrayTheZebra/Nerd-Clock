#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Package a UNO R4 WiFi raw binary as an Arduino .ota file; optionally serve it."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import struct
import zlib


def literal_lzss(data):
    # Okumura/Arduino bitstream: MSB-first 1 + 8 data bits per literal.
    # Literal-only encoding keeps this helper portable and dependency-free.
    result = bytearray()
    bits = count = 0
    for value in data:
        bits = (bits << 9) | 0x100 | value
        count += 9
        while count >= 8:
            count -= 8
            result.append((bits >> count) & 0xFF)
        bits &= (1 << count) - 1
    if count:
        result.append(bits << (8 - count))
    return bytes(result)


def package(data):
    if not 8 <= len(data) <= 262144:
        raise ValueError('UNO R4 binary must contain 8..262144 bytes')
    payload = struct.pack('<I', 0x23411002) + bytes([0, 0, 0, 0, 0, 0, 0, 0x40]) + literal_lzss(data)
    return struct.pack('<II', len(payload), zlib.crc32(payload)) + payload


def serve(data, port):
    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path != '/Nerd-Clock.ota':
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header('Content-Type', 'application/octet-stream')
            self.send_header('Content-Length', str(len(data)))
            self.send_header('Cache-Control', 'no-store')
            self.end_headers()
            self.wfile.write(data)
    print(f'In Nerd-Clock eintragen: http://IP-DIESES-PCs:{port}/Nerd-Clock.ota', flush=True)
    print('Server beenden mit Strg+C. Es wird nur diese OTA-Datei angeboten.', flush=True)
    with ThreadingHTTPServer(('0.0.0.0', port), Handler) as server:
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path, help='UNO R4 WiFi .bin (oder vorhandene .ota mit --serve)')
    parser.add_argument('output', type=Path, nargs='?', help='Ausgabedatei .ota')
    parser.add_argument('--serve', action='store_true', help='OTA-Datei lokal per HTTP bereitstellen')
    parser.add_argument('--port', type=int, default=8000)
    args = parser.parse_args()
    if not 1 <= args.port <= 65535:
        parser.error('Port muss 1..65535 sein')
    raw = args.input.read_bytes()
    if args.input.suffix.lower() == '.ota':
        if not args.serve or args.output:
            parser.error('Vorhandene .ota nur mit --serve und ohne Ausgabe verwenden')
        data = raw
    else:
        if args.input.suffix.lower() != '.bin':
            parser.error('Eine exportierte UNO R4 WiFi .bin-Datei verwenden')
        try:
            data = package(raw)
        except ValueError as error:
            parser.error(str(error))
        output = args.output or args.input.with_suffix('.ota')
        if output.resolve() == args.input.resolve():
            parser.error('Eingabe und Ausgabe muessen verschieden sein')
        output.write_bytes(data)
        print(f'{output}: {len(data)} bytes', flush=True)
    if args.serve:
        serve(data, args.port)


if __name__ == '__main__':
    main()
