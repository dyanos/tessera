"""Writes malformed .npy files under npy/ that the reader must reject without crashing."""
import os
import struct

HERE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "npy")


def write(name, header, body=b"", version=1):
    text = header.encode("latin1")
    prefix = 10 if version == 1 else 12
    pad = (-(prefix + len(text) + 1)) % 64
    text += b" " * pad + b"\n"
    length = struct.pack("<H", len(text)) if version == 1 else struct.pack("<I", len(text))
    with open(os.path.join(HERE, name), "wb") as f:
        f.write(b"\x93NUMPY" + bytes([version, 0]) + length + text + body)


write("bad_zero_item.npy", "{'descr': '|S0', 'fortran_order': False, 'shape': (1000000000000,), }")
write("bad_huge_dtype.npy", "{'descr': '<U99999999999999999999999', 'fortran_order': False, 'shape': (1,), }", b"\0" * 8)
write("bad_field_overflow.npy",
      "{'descr': [('a', '|S4611686018427387903'), ('b', '|S4611686018427387903'), ('c', '|S4611686018427387903'), "
      "('d', '|S4611686018427387903'), ('e', '|S8')], 'fortran_order': False, 'shape': (1,), }", b"\0" * 8)
write("bad_deep_header.npy", "{'descr': " + "[" * 100000 + "]" * 100000 + ", 'fortran_order': False, 'shape': (1,), }",
      version=2)
