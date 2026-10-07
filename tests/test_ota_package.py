# SPDX-License-Identifier: MIT
import importlib.util
from pathlib import Path
import struct
import zlib

spec = importlib.util.spec_from_file_location('make_ota', Path(__file__).parents[1]/'tools/make_ota.py')
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)

# Independent bit-by-bit reader checks framing and every output byte.
def read_literals(data):
    bits = ''.join(f'{b:08b}' for b in data)
    out = bytearray()
    for pos in range(0, len(bits)-8, 9):
        assert bits[pos] == '1'
        out.append(int(bits[pos+1:pos+9], 2))
    return bytes(out)

for data in [bytes(range(256)), b'12345678', bytes(range(256))*1024]:
    packed = tool.package(data)
    length, crc, magic = struct.unpack('<III', packed[:12])
    assert length == len(packed)-8
    assert crc == zlib.crc32(packed[8:])
    assert magic == 0x23411002 and packed[12:20] == bytes([0]*7+[0x40])
    assert read_literals(packed[20:]) == data
for size in [0, 7, 262145]:
    try:
        tool.package(bytes(size))
        raise AssertionError('Invalid binary accepted')
    except ValueError:
        pass
print('OTA packet length, CRC, board ID, LZSS framing and size limits passed.')
