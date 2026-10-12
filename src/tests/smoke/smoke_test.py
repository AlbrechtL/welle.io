#!/usr/bin/env python3
#
# Smoke test of an installed welle.io: plays a recording of the test data
# with the installed welle-io (option --smoke-test) and welle-cli and compares
# what they decoded with manifest.json. Used by the package workflows after
# installing the package the way a user would, see src/tests/README.md.
# Python standard library only, it runs on Linux, macOS and Windows.
#
# This file was generated with the help of an AI assistant (Claude Code).
#
# This file is part of the welle.io.
#
# welle.io is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# welle.io is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with welle.io; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

"""Smoke test of an installed welle.io with a recording of the test data.

Examples:
  smoke_test.py --test-data ../welle.io-test-data --welle-io /usr/bin/welle-io --welle-cli /usr/bin/welle-cli
  smoke_test.py --test-data test-data --flatpak io.welle.welle-gui
  smoke_test.py --test-data test-data --report report.json   (only check a report)

Exit code 0 if everything passed, 1 on failures, 77 if the test data is
missing (CTest's skip code).
"""

import argparse
import json
import os
import socket
import subprocess
import sys
import tempfile
import time
import urllib.request

SKIP = 77


class Checks:
    def __init__(self, title):
        self.title = title
        self.failed = 0
        print(f"\n== {title}", flush=True)

    def check(self, ok, text):
        print(f"  {'PASS' if ok else 'FAIL'}: {text}", flush=True)
        if not ok:
            self.failed += 1
        return ok


def normalized_mode(mode):
    # MP2 encoders switch between stereo and joint stereo from frame to frame
    return (mode or "").replace("Joint Stereo", "Stereo")


def audio_services(expect):
    return [s for s in expect.get("services", []) if s.get("type") != "data"]


def find_clip(test_data, clip_id):
    with open(os.path.join(test_data, "manifest.json"), encoding="utf-8") as f:
        manifest = json.load(f)
    for clip in manifest["clips"]:
        if clip["id"] == clip_id:
            return clip
    sys.exit(f"Clip {clip_id} is not in {test_data}/manifest.json")


def clip_file(test_data, clip):
    path = os.path.join(test_data, "clips", f"{clip['id']}.{clip['format']}.iq")
    if not os.path.exists(path) and os.path.exists(path + ".zst"):
        print(f"Unpacking {path}.zst", flush=True)
        subprocess.run([sys.executable, os.path.join(test_data, "scripts", "prepare_test_data.py"),
                        "--unpack", "--ids", clip["id"]], check=True)
    return os.path.abspath(path)


def tail(path, lines=60):
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            return "".join(f.readlines()[-lines:])
    except OSError:
        return f"({path} not found)"


def check_report(report, expect, checks):
    """Compares the report of welle-io --smoke-test with the manifest."""
    for key in ("version", "qt_version", "os", "cpu", "platform", "graphics_api", "device_name"):
        print(f"  {key}: {report.get(key)}")

    checks.check(report.get("ok"), "welle-io reports success" +
                 ("" if report.get("ok") else f" -- {report.get('error')}"))

    ensemble = report.get("ensemble", {})
    if "ensemble_label" in expect:
        checks.check(ensemble.get("label") == expect["ensemble_label"].strip() and
                     ensemble.get("id", "").lower() == expect.get("ensemble_id", "").lower(),
                     f"Ensemble {ensemble.get('id')} \"{ensemble.get('label')}\" "
                     f"(expected {expect.get('ensemble_id')} \"{expect['ensemble_label']}\")")

    found = {s["sid"].lower(): s for s in report.get("services", [])}
    gui = set(report.get("gui_stations", []))
    for es in audio_services(expect):
        label = es["label"].strip()
        s = found.get(es["sid"].lower())
        if not s:
            checks.check(False, f"{es['sid']} \"{label}\" -- not found")
            continue
        wrong = []
        if s["label"] != label:
            wrong.append(f"label, expected \"{label}\"")
        if not s["played"]:
            wrong.append("not played")
        if s["type"] != es["type"]:
            wrong.append(f"type, expected {es['type']}")
        if "mode" in es and normalized_mode(s["mode"]) != normalized_mode(es["mode"]):
            wrong.append(f"mode, expected {es['mode']}")
        if label not in gui:
            wrong.append("not in the station list of the user interface")
        checks.check(not wrong, f"{s['sid']} \"{s['label']}\": {s['type']}, {s['mode']}" +
                     (f" -- {', '.join(wrong)}" if wrong else ""))

    dls = sum(1 for s in found.values() if s.get("dls"))
    min_dls = expect.get("min_dls_services", 0)
    checks.check(dls >= min_dls, f"DLS of {dls} services (expected at least {min_dls})")

    errors = report.get("qml_compile_errors", [])
    checks.check(not errors, f"{len(errors)} QML compile errors" +
                 "".join(f"\n        {e}" for e in errors))
    warnings = report.get("qml_warnings", [])
    checks.check(not warnings, f"{len(warnings)} QML warnings" +
                 "".join(f"\n        {w}" for w in warnings))
    for w in report.get("warnings", []) + report.get("error_messages", []):
        print(f"  note: {w}")


def run_welle_io(args, clip, path, expect):
    checks = Checks("welle-io")
    work = args.work_dir
    report_file = os.path.join(work, "welle-io-report.json")
    log_file = os.path.join(work, "welle-io.log")
    output_file = os.path.join(work, "welle-io.out")
    for f in (report_file, log_file):
        if os.path.exists(f):
            os.remove(f)

    options = ["--raw-file", path, "--raw-format", clip["format"],
               "--smoke-test", report_file, "--smoke-timeout", str(args.timeout),
               "--log-file", log_file]

    env = dict(os.environ)
    if sys.platform.startswith("linux") and not (env.get("DISPLAY") or env.get("WAYLAND_DISPLAY")):
        env.setdefault("QT_QPA_PLATFORM", "offscreen")

    if args.flatpak:
        command = ["flatpak", "run",
                   f"--filesystem={os.path.dirname(path)}:ro", f"--filesystem={work}"]
        for name in ("QT_QPA_PLATFORM", "QT_QUICK_BACKEND", "QT_DEBUG_PLUGINS"):
            if name in env:
                command.append(f"--env={name}={env[name]}")
        command += [args.flatpak] + options
    else:
        command = [args.welle_io] + options

    print("  " + " ".join(command), flush=True)
    start = time.monotonic()
    with open(output_file, "w") as out:
        try:
            result = subprocess.run(command, stdout=out, stderr=subprocess.STDOUT, env=env,
                                    timeout=args.timeout + 60)
            exit_code = result.returncode
        except subprocess.TimeoutExpired:
            exit_code = None
    print(f"  Finished after {time.monotonic() - start:.1f} s, exit code {exit_code}")

    checks.check(exit_code == 0, f"Exit code {exit_code}")
    if os.path.exists(report_file):
        with open(report_file, encoding="utf-8") as f:
            check_report(json.load(f), expect, checks)
    else:
        checks.check(False, f"No report {report_file}")

    if checks.failed:
        print(f"\n-- Output of welle-io (last lines of {output_file}):\n{tail(output_file)}")
    return checks.failed


def free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def http_get(port, url, size=None, timeout=5):
    with urllib.request.urlopen(f"http://127.0.0.1:{port}{url}", timeout=timeout) as reply:
        return reply.read(size) if size else reply.read()


def run_welle_cli(args, clip, path, expect):
    checks = Checks("welle-cli")
    output_file = os.path.join(args.work_dir, "welle-cli.out")
    port = free_port()
    command = [args.welle_cli]
    if clip.get("channel"):
        command += ["-c", clip["channel"]]
    command += ["-f", path, "-w", str(port)]
    print("  " + " ".join(command), flush=True)

    with open(output_file, "w") as out:
        process = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=out, stderr=subprocess.STDOUT)
        try:
            # The recording loops, everything should be there after one pass
            deadline = time.monotonic() + clip.get("duration_s", 30) + 30
            wanted = {s["sid"].lower(): s["label"].strip() for s in audio_services(expect)}
            mux, found = {}, {}
            while time.monotonic() < deadline and process.poll() is None:
                try:
                    mux = json.loads(http_get(port, "/mux.json"))
                    found = {s["sid"].lower(): s["label"]["label"].strip() for s in mux.get("services", [])}
                    if (mux.get("ensemble", {}).get("label", {}).get("label", "").strip() == expect.get("ensemble_label", "").strip()
                            and all(found.get(sid) == label for sid, label in wanted.items())):
                        break
                except (OSError, ValueError, KeyError, TypeError):
                    pass
                time.sleep(0.5)

            ensemble = mux.get("ensemble", {})
            label = ensemble.get("label", {}).get("label", "").strip() if isinstance(ensemble.get("label"), dict) else ""
            checks.check(label == expect.get("ensemble_label", "").strip(),
                         f"Ensemble {ensemble.get('id')} \"{label}\" in mux.json")
            for sid, wanted_label in wanted.items():
                checks.check(found.get(sid) == wanted_label, f"{sid} \"{found.get(sid)}\" in mux.json"
                             + ("" if found.get(sid) == wanted_label else f" -- expected \"{wanted_label}\""))

            # Decoding and the MP3 encoder (LAME)
            for sid in wanted:
                try:
                    data = http_get(port, f"/mp3/{sid}", 8192, timeout=20)
                except OSError as e:
                    data = b""
                    print(f"  /mp3/{sid}: {e}")
                frame = any(data[i] == 0xFF and data[i + 1] & 0xE0 == 0xE0 for i in range(len(data) - 1))
                checks.check(len(data) >= 4096 and frame, f"/mp3/{sid}: {len(data)} bytes of MP3")

            checks.check(process.poll() is None, "welle-cli still running")
        finally:
            process.terminate()
            try:
                process.wait(10)
            except subprocess.TimeoutExpired:
                process.kill()

    if checks.failed:
        print(f"\n-- Output of welle-cli (last lines of {output_file}):\n{tail(output_file)}")
    return checks.failed


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--test-data", default=os.environ.get("WELLE_TEST_DATA_DIR"),
                        help="checkout of welle.io-test-data (default: $WELLE_TEST_DATA_DIR)")
    parser.add_argument("--clip", default="odr_dabmod", help="clip id of manifest.json (default: odr_dabmod)")
    parser.add_argument("--welle-io", metavar="PATH", help="installed welle-io executable")
    parser.add_argument("--flatpak", metavar="APP_ID", help="run welle-io from this installed Flatpak")
    parser.add_argument("--welle-cli", metavar="PATH", help="installed welle-cli executable")
    parser.add_argument("--report", metavar="FILE", help="only check this report of welle-io --smoke-test")
    parser.add_argument("--work-dir", help="for the report and logs (default: a new temporary directory; "
                        "with --flatpak in the current directory, the Flatpak has its own /tmp)")
    parser.add_argument("--timeout", type=int, default=180, help="seconds for welle-io (default: 180)")
    args = parser.parse_args()

    if not (args.welle_io or args.flatpak or args.welle_cli or args.report):
        parser.error("nothing to test: give --welle-io, --flatpak, --welle-cli or --report")
    if not args.test_data or not os.path.exists(os.path.join(args.test_data, "manifest.json")):
        print("Skipped: no test data, set --test-data or WELLE_TEST_DATA_DIR")
        return SKIP

    test_data = os.path.abspath(args.test_data)
    clip = find_clip(test_data, args.clip)
    expect = clip["expect"]

    if args.report:
        checks = Checks(f"Report {args.report}")
        with open(args.report, encoding="utf-8") as f:
            check_report(json.load(f), expect, checks)
        return 1 if checks.failed else 0

    path = clip_file(test_data, clip)
    if not os.path.exists(path):
        print(f"Skipped: {path} is missing")
        return SKIP
    print(f"Recording {clip['id']}: {path}")

    if args.work_dir:
        os.makedirs(args.work_dir, exist_ok=True)
    else:
        args.work_dir = tempfile.mkdtemp(prefix="welle-smoke-", dir=os.getcwd() if args.flatpak else None)
    args.work_dir = os.path.abspath(args.work_dir)
    print(f"Reports and logs: {args.work_dir}")

    failed = 0
    if args.welle_io or args.flatpak:
        failed += run_welle_io(args, clip, path, expect)
    if args.welle_cli:
        failed += run_welle_cli(args, clip, path, expect)

    print(f"\n{'FAILED' if failed else 'PASSED'}: {failed} failed checks")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
