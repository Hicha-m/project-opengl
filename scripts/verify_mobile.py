#!/usr/bin/env python3
"""Install the real app, run its mobile integration test and capture its screen."""
import argparse
import json
import subprocess
import time
from pathlib import Path

BUNDLE = "io.github.hicha-m.project-opengl"
ANDROID = "io.github.hicha_m.space"

def run(*args):
    return subprocess.check_output(args, text=True)

def wait_for(check, seconds=480):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        value = check()
        if value:
            return value
        time.sleep(2)
    raise RuntimeError("Mobile test timed out")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("platform", choices=("android", "ios"))
    parser.add_argument("application", type=Path)
    parser.add_argument("--output", type=Path, default=Path("build/mobile-evidence"))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    if args.platform == "android":
        wait_for(lambda: run("adb", "shell", "getprop", "sys.boot_completed").strip() == "1", 300)
        run("adb", "install", "-r", str(args.application))
        run("adb", "shell", "am", "force-stop", ANDROID)
        run("adb", "logcat", "-c")
        run("adb", "shell", "am", "start", "-n", ANDROID + "/.MainActivity", "--ez", "mobile_test", "true")
        def logs(): return run("adb", "logcat", "-d", "-v", "brief")
        wait_for(lambda: "MOBILE_MVP_TEST_PASSED" in logs())
        screenshot = subprocess.check_output(["adb", "exec-out", "screencap", "-p"])
        (args.output / "android.png").write_bytes(screenshot)
        process = run("adb", "shell", "pidof", ANDROID).strip()
        run("adb", "shell", "input", "keyevent", "KEYCODE_HOME")
        time.sleep(2)
        run("adb", "shell", "am", "start", "-n", ANDROID + "/.MainActivity")
        time.sleep(3)
        if run("adb", "shell", "pidof", ANDROID).strip() != process:
            raise RuntimeError("Android application did not survive background/foreground")
        (args.output / "android.log").write_text(logs())
    else:
        devices = json.loads(run("xcrun", "simctl", "list", "devices", "available", "--json"))
        candidates = [device for runtime, group in devices["devices"].items() if "iOS" in runtime for device in group if "iPhone" in device["name"]]
        if not candidates: raise RuntimeError("No iPhone simulator available")
        device = candidates[0]["udid"]
        if candidates[0]["state"] != "Booted": run("xcrun", "simctl", "boot", device)
        run("xcrun", "simctl", "bootstatus", device, "-b")
        run("xcrun", "simctl", "install", device, str(args.application))
        log = args.output / "ios.log"
        with log.open("w") as output:
            process = subprocess.Popen(["xcrun", "simctl", "launch", "--console", "--terminate-running-process", device, BUNDLE, "--mobile-test", "--keep-running"], stdout=output, stderr=subprocess.STDOUT)
            try:
                def passed():
                    content = log.read_text()
                    if "MOBILE_MVP_TEST_PASSED" in content: return True
                    if process.poll() is not None: raise RuntimeError("iOS application exited: " + content[-4000:])
                    return False
                wait_for(passed)
                run("xcrun", "simctl", "io", device, "screenshot", str(args.output / "ios.png"))
                run("xcrun", "simctl", "launch", device, "com.apple.Preferences")
                time.sleep(2)
                run("xcrun", "simctl", "launch", device, BUNDLE)
                time.sleep(3)
                if process.poll() is not None: raise RuntimeError("iOS application exited during background/foreground")
            finally:
                run("xcrun", "simctl", "terminate", device, BUNDLE)
                process.wait(timeout=20)
    print(args.platform + " mobile MVP verified; evidence in " + str(args.output))

if __name__ == "__main__": main()
