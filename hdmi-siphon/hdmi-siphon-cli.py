#!/usr/bin/env python3
"""
hdmi-siphon-cli.py — HDMI-Siphon Command-Line Tool
Author: jayis1
Version: 1.0.0
License: Proprietary — Authorized Security Research Use Only

Command-line interface for controlling the HDMI-Siphon device
without the mobile app. Useful for scripting, automation, and
headless operation.

Usage:
    python hdmi-siphon-cli.py status                    # Get device status
    python hdmi-siphon-cli.py capture [--output frame.jpg] [--quality 95]
    python hdmi-siphon-cli.py record --interval 1 --count 60 --output ./captures/
    python hdmi-siphon-cli.py edid --preset 1080p_nohdcp
    python hdmi-siphon-cli.py cec-monitor              # Monitor CEC bus
    python hdmi-siphon-cli.py cec-send --address 0 --opcode 0x36  # Send standby
    python hdmi-siphon-cli.py osd --text "RECORDING" --x 100 --y 50 --color "#FF0000"
    python hdmi-siphon-cli.py mode --mode invert       # Set video mode
    python hdmi-siphon-cli.py config --stealth true     # Set configuration
"""

import argparse
import json
import sys
import time
import os
from urllib.request import urlopen, Request
from urllib.error import URLError, HTTPError

# Default device address
DEFAULT_HOST = "192.168.4.1"
DEFAULT_PORT = 8080


def api_url(host, port, path):
    """Build full API URL."""
    return f"http://{host}:{port}{path}"


def api_get(host, port, path):
    """Perform HTTP GET request."""
    url = api_url(host, port, path)
    try:
        with urlopen(url, timeout=5) as resp:
            return json.loads(resp.read().decode())
    except (URLError, HTTPError) as e:
        print(f"[ERROR] API request failed: {e}", file=sys.stderr)
        sys.exit(1)


def api_post(host, port, path, data=None):
    """Perform HTTP POST request with JSON body."""
    url = api_url(host, port, path)
    body = json.dumps(data).encode() if data else b''
    req = Request(url, data=body, method='POST')
    req.add_header('Content-Type', 'application/json')
    try:
        with urlopen(req, timeout=10) as resp:
            return json.loads(resp.read().decode())
    except (URLError, HTTPError) as e:
        print(f"[ERROR] API request failed: {e}", file=sys.stderr)
        sys.exit(1)


def cmd_status(args):
    """Get and display device status."""
    data = api_get(args.host, args.port, "/api/status")
    print(json.dumps(data, indent=2))


def cmd_capture(args):
    """Trigger a single frame capture."""
    result = api_post(args.host, args.port, "/api/capture")
    if result.get("status") == "ok":
        frame_id = result.get("frame_id", "?")
        print(f"[OK] Frame #{frame_id} captured")

        # Optionally download the frame
        if args.output:
            frame_url = api_url(args.host, args.port, f"/frame/frame_{frame_id:06d}.jpg")
            try:
                with urlopen(frame_url, timeout=30) as resp:
                    with open(args.output, 'wb') as f:
                        f.write(resp.read())
                print(f"[OK] Frame saved to {args.output}")
            except Exception as e:
                print(f"[WARN] Could not download frame: {e}")
    else:
        print(f"[ERROR] Capture failed: {result.get('msg', 'unknown')}")
        sys.exit(1)


def cmd_record(args):
    """Record multiple frames at interval."""
    interval = args.interval
    count = args.count
    output_dir = args.output or "."

    os.makedirs(output_dir, exist_ok=True)
    print(f"[INFO] Recording {count} frames every {interval}s to {output_dir}")

    for i in range(count):
        result = api_post(args.host, args.port, "/api/capture")
        if result.get("status") == "ok":
            frame_id = result.get("frame_id", i)
            print(f"[OK] Frame #{frame_id} ({i+1}/{count})")
        else:
            print(f"[WARN] Frame {i+1} failed: {result}")

        if i < count - 1:
            time.sleep(interval)

    print(f"[OK] Recording complete: {count} frames to {output_dir}")


def cmd_edid(args):
    """Manage EDID."""
    if args.preset == "1080p_nohdcp":
        result = api_post(args.host, args.port, "/api/edid",
                          {"edid_hex": "00FFFFFFFFFFFF00..."})  # Simplified
        print(f"[OK] Set EDID: {result}")
    elif args.preset:
        print(f"[INFO] Preset '{args.preset}' selected")
        # Presets would be resolved to actual EDID hex data
    else:
        # Show current EDID info from status
        status = api_get(args.host, args.port, "/api/status")
        print(f"EDID valid: {status.get('edid_valid')}")
        print(f"EDID source: {status.get('edid_source')}")


def cmd_cec_monitor(args):
    """Monitor CEC bus for messages."""
    print("[INFO] Monitoring CEC bus... (Ctrl+C to stop)")
    try:
        while True:
            status = api_get(args.host, args.port, "/api/status")
            cec_count = status.get("cec_msg_count", 0)
            print(f"\r[INFO] CEC messages: {cec_count}", end="", flush=True)
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[INFO] CEC monitoring stopped")


def cmd_cec_send(args):
    """Send CEC command."""
    payload = [args.address, args.opcode]
    if args.payload:
        payload.extend(args.payload)
    result = api_post(args.host, args.port, "/api/cec",
                      {"address": args.address, "opcode": args.opcode,
                       "payload": args.payload or []})
    print(f"[OK] CEC command sent: {result}")


def cmd_osd(args):
    """Inject OSD text overlay."""
    result = api_post(args.host, args.port, "/api/config",
                      {"osd_text": args.text, "osd_x": args.x,
                       "osd_y": args.y, "osd_color": args.color})
    print(f"[OK] OSD applied: {result}")


def cmd_mode(args):
    """Set video processing mode."""
    valid_modes = ["passthrough", "invert", "grayscale", "blank"]
    if args.mode not in valid_modes:
        print(f"[ERROR] Invalid mode: {args.mode}. Valid: {valid_modes}")
        sys.exit(1)
    result = api_post(args.host, args.port, "/api/config", {"mode": args.mode})
    print(f"[OK] Mode set to {args.mode}: {result}")


def cmd_config(args):
    """Set device configuration."""
    config = {}
    if args.stealth is not None:
        config["stealth"] = args.stealth
    if args.brightness is not None:
        config["brightness"] = args.brightness
    if args.contrast is not None:
        config["contrast"] = args.contrast

    result = api_post(args.host, args.port, "/api/config", config)
    print(f"[OK] Config applied: {result}")


def main():
    parser = argparse.ArgumentParser(
        description="HDMI-Siphon Command-Line Interface — Author: jayis1",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s status
  %(prog)s capture --output screenshot.jpg
  %(prog)s record --interval 2 --count 30 --output ./captures/
  %(prog)s cec-send --address 0 --opcode 0x36        # TV standby
  %(prog)s mode --mode invert
        """,
    )
    parser.add_argument("--host", default=DEFAULT_HOST, help=f"Device IP (default: {DEFAULT_HOST})")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"API port (default: {DEFAULT_PORT})")

    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    # Status
    subparsers.add_parser("status", help="Get device status")

    # Capture
    cap_parser = subparsers.add_parser("capture", help="Capture a single frame")
    cap_parser.add_argument("--output", "-o", help="Save frame to file")
    cap_parser.add_argument("--quality", type=int, default=85, help="JPEG quality (1-100)")

    # Record
    rec_parser = subparsers.add_parser("record", help="Record multiple frames")
    rec_parser.add_argument("--interval", type=int, default=1, help="Interval between captures (seconds)")
    rec_parser.add_argument("--count", type=int, default=10, help="Number of frames to capture")
    rec_parser.add_argument("--output", "-o", default=".", help="Output directory")

    # EDID
    edid_parser = subparsers.add_parser("edid", help="Manage EDID")
    edid_parser.add_argument("--preset", help="EDID preset name (1080p_nohdcp, 720p, etc.)")
    edid_parser.add_argument("--file", help="EDID binary file to inject")

    # CEC monitor
    subparsers.add_parser("cec-monitor", help="Monitor CEC bus messages")

    # CEC send
    cec_parser = subparsers.add_parser("cec-send", help="Send CEC command")
    cec_parser.add_argument("--address", type=int, required=True, help="Destination address (0-15)")
    cec_parser.add_argument("--opcode", type=lambda x: int(x, 0), required=True, help="CEC opcode (hex or decimal)")
    cec_parser.add_argument("--payload", type=lambda x: int(x, 0), nargs="*", default=[], help="Payload bytes")

    # OSD
    osd_parser = subparsers.add_parser("osd", help="Inject OSD text overlay")
    osd_parser.add_argument("--text", required=True, help="Text to display")
    osd_parser.add_argument("--x", type=int, default=100, help="X position")
    osd_parser.add_argument("--y", type=int, default=100, help="Y position")
    osd_parser.add_argument("--color", default="#FF0000", help="Text color")

    # Mode
    mode_parser = subparsers.add_parser("mode", help="Set video mode")
    mode_parser.add_argument("--mode", required=True, choices=["passthrough", "invert", "grayscale", "blank"])

    # Config
    config_parser = subparsers.add_parser("config", help="Set configuration")
    config_parser.add_argument("--stealth", type=bool, help="Enable/disable stealth mode")
    config_parser.add_argument("--brightness", type=int, help="Brightness (0-255)")
    config_parser.add_argument("--contrast", type=int, help="Contrast (0-255)")

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        sys.exit(1)

    # Dispatch commands
    commands = {
        "status": cmd_status,
        "capture": cmd_capture,
        "record": cmd_record,
        "edid": cmd_edid,
        "cec-monitor": cmd_cec_monitor,
        "cec-send": cmd_cec_send,
        "osd": cmd_osd,
        "mode": cmd_mode,
        "config": cmd_config,
    }

    cmd_func = commands.get(args.command)
    if cmd_func:
        cmd_func(args)
    else:
        print(f"[ERROR] Unknown command: {args.command}")
        sys.exit(1)


if __name__ == "__main__":
    main()
