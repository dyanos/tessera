"""Regenerate the .npy fixtures used by tests/stdlib/positive/31 (needs numpy).

    python tests/stdlib/data/make_npy_fixtures.py
"""

import os

import numpy as np

HERE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "npy")


def save(name, array, version=None, allow_pickle=False):
    with open(os.path.join(HERE, name), "wb") as f:
        np.lib.format.write_array(f, array, version=version, allow_pickle=allow_pickle)


def main():
    os.makedirs(HERE, exist_ok=True)
    save("f64_2d.npy", np.arange(12, dtype="<f8").reshape(4, 3) / 2)
    save("i32_fortran_be.npy", np.asfortranarray(np.array([[1, -2], [3, -4], [5, -6]], dtype=">i4")))
    save("u8.npy", np.array([0, 255, 7], dtype=np.uint8))
    save("i64_v2.npy", np.array([-(2**62), 0, 2**62], dtype="<i8"), version=(2, 0))
    save("scalar.npy", np.array(3.5))
    save("bool.npy", np.array([True, False, True]))
    save("f32.npy", np.array([1.5, -0.25, 0.1], dtype=np.float32))
    save("f16.npy", np.array([1.5, -0.25, 65504.0], dtype=np.float16))
    dt = np.dtype(
        [("id", "<i8"), ("flag", "?"), ("score", "<f4"), ("name", "<U5"), ("tag", "S3"), ("when", "<M8[s]")],
        align=True,
    )
    rec = np.array(
        [
            (1, True, 0.5, "alice", b"ab", np.datetime64(10, "s")),
            (2, False, -1.25, "\uac00\ub098", b"xyz", np.datetime64("NaT", "s")),
            (3, True, 2.0, "", b"", np.datetime64(0, "s")),
        ],
        dtype=dt,
    )
    save("structured.npy", rec)
    save("big_endian_rec.npy", np.array([(7, 1.0)], dtype=[("a", ">i2"), ("b", ">f8")]))
    save("u64_overflow.npy", np.array([2**63], dtype=np.uint64))
    save("object.npy", np.array([1, "a"], dtype=object), allow_pickle=True)
    save("cube.npy", np.zeros((2, 2, 2)))
    save("complex.npy", np.array([1 + 2j]))


if __name__ == "__main__":
    main()
