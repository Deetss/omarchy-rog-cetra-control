#!/usr/bin/env python3
"""Offline native tests for cetra-bt-read helper per contract."""
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile


def run_cmd(cmd, check=True):
    return subprocess.run(cmd, capture_output=True, text=True, check=check)


def get_bluez_flags():
    pkg_config = shutil.which("pkg-config")
    if pkg_config:
        res = subprocess.run([pkg_config, "--cflags", "--libs", "bluez"], capture_output=True, text=True)
        if res.returncode == 0 and res.stdout.strip():
            return res.stdout.strip().split()
    return ["-lbluetooth"]


def build_helper(src_path, bin_path):
    cc = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if not cc:
        raise RuntimeError("No C compiler (cc/gcc/clang) found in PATH")
    bluez_flags = get_bluez_flags()
    cmd = [cc, "-O2", "-Wall", "-Wextra", "-Werror", "-std=c11", src_path, "-o", bin_path] + bluez_flags
    print(f"Compiling: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)


def test_selftest(bin_path):
    res = run_cmd([bin_path, "--selftest"])
    assert res.returncode == 0, f"Expected 0, got {res.returncode}: {res.stderr}"
    assert res.stdout.strip() == "ok", f"Expected 'ok', got: {res.stdout}"
    print("PASS: --selftest passed successfully")


def test_negative_cli(bin_path):
    # 1. No arguments
    res = subprocess.run([bin_path], capture_output=True, text=True)
    assert res.returncode == 1, f"Expected exit 1, got {res.returncode}"
    data = json.loads(res.stdout.strip())
    assert data["status"] == "unavailable"
    assert data["error"] == "protocol"
    assert data["address"] == ""

    # 2. Too many arguments
    res = subprocess.run([bin_path, "AA:BB:CC:DD:EE:FF", "extra"], capture_output=True, text=True)
    assert res.returncode == 1
    data = json.loads(res.stdout.strip())
    assert data["status"] == "unavailable"
    assert data["error"] == "protocol"

    # 3. Invalid address formats (never reaches Bluetooth socket)
    invalid_addrs = [
        "invalid",
        "AA:BB:CC:DD:EE",
        "AA:BB:CC:DD:EE:FF:00",
        "AA-BB-CC-DD-EE-FF",
        "GG:BB:CC:DD:EE:FF",
        "AA:BB:CC:DD:EE:F ",
    ]
    for bad in invalid_addrs:
        res = subprocess.run([bin_path, bad], capture_output=True, text=True)
        assert res.returncode == 1, f"Expected exit 1 for {bad}"
        data = json.loads(res.stdout.strip())
        assert data["status"] == "unavailable"
        assert data["error"] == "protocol"
        assert data["address"] == ""

    print("PASS: negative CLI tests passed")


def test_lock_busy(bin_path):
    uid = os.getuid()
    lock_path = f"\0io.github.pavellizunov.rog-cetra-control.bt.{uid}"
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    try:
        sock.bind(lock_path)
        res = subprocess.run([bin_path, "AA:BB:CC:DD:EE:FF"], capture_output=True, text=True)
        assert res.returncode == 1, f"Expected exit 1 when busy, got {res.returncode}"
        line = res.stdout.strip()
        assert len(line.encode("utf-8")) <= 1024, "JSON line must be <= 1024 bytes"
        data = json.loads(line)
        assert data["status"] == "unavailable"
        assert data["address"] == "AA:BB:CC:DD:EE:FF"
        assert data["error"] == "busy"
        assert data["left"] is None
        assert data["right"] is None
        assert data["case"] is None
        assert data["mode"] == "unknown"
        assert data["left_charging"] is None
        assert data["right_charging"] is None
        assert data["case_charging"] is None
        print("PASS: lock concurrency / busy detection passed (never reaches Bluetooth)")
    finally:
        sock.close()


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(script_dir)
    src_path = os.path.join(root_dir, "cetra-bt-read.c")
    if not os.path.isfile(src_path):
        src_path = os.path.join(os.getcwd(), "cetra-bt-read.c")
    if not os.path.isfile(src_path):
        print(f"Cannot find cetra-bt-read.c in {root_dir} or {os.getcwd()}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        bin_path = os.path.join(tmpdir, "cetra-bt-read")
        build_helper(src_path, bin_path)
        test_selftest(bin_path)
        test_negative_cli(bin_path)
        test_lock_busy(bin_path)

    print("ALL NATIVE TESTS PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
