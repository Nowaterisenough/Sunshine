"""Render the real panel against a loopback HTTPS fixture, never a user's host.

Usage: python3 smoke.py --binary /path/to/sunshine-gui --output /path/to/screenshots
Requires Python 3 and the openssl CLI. Every screenshot is labelled as test data.
"""
import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import ssl
import subprocess
import tempfile
import threading


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def do_GET(self):
        self.server.paths.append(self.path)
        if self.server.setup and self.path == "/api/config":
            self.send_response(307)
            self.send_header("Location", "/welcome")
            self.end_headers()
            return
        responses = {
            "/api/config": {"status": "true", "platform": "windows", "version": "UI test fixture",
                "sunshine_name": "Sunshine · 界面测试主机", "active_encoder": "nvenc",
                "encoder": "nvenc", "port": str(self.server.server_port - 1), "min_log_level": "2",
                "max_bitrate": "80000", "display_devices": [], "adapters": []},
            "/api/apps": {"apps": [{"name": "桌面", "cmd": ""}, {"name": "Steam Big Picture", "cmd": "steam://open/bigpicture"}]},
            "/api/clients/list": {"status": "true", "named_certs": [{"uuid": "test-tv", "name": "客厅电视", "enabled": True, "streaming": True}, {"uuid": "test-laptop", "name": "笔记本", "enabled": False}]},
            "/api/runtime/sessions": {"success": True, "sessions": [{"client_name": "客厅电视", "app_name": "桌面", "state": "RUNNING", "width": 2560, "height": 1440, "fps": 120, "bitrate": 60000, "enable_hdr": True}]},
            "/api/logs": "[Info] UI test fixture: no real stream is running.\n[Info] Local API connection accepted.\n[Info] Video encoder: nvenc\n",
        }
        if self.path not in responses:
            self.send_error(404)
            return
        value = responses[self.path]
        data = value.encode() if isinstance(value, str) else json.dumps(value, ensure_ascii=False).encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/plain" if isinstance(value, str) else "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="sunshine-qt-smoke-") as directory:
        cert, key = Path(directory) / "cert.pem", Path(directory) / "key.pem"
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
            "-subj", "/CN=localhost", "-keyout", str(key), "-out", str(cert)], check=True, capture_output=True)
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.paths, server.setup = [], False
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(cert, key)
        server.socket = context.wrap_socket(server.socket, server_side=True)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            cases = [(str(page), page, []) for page in range(8)]
            cases += [(f"settings-{section}", 3, ["--section", str(section)]) for section in range(1, 4)]
            cases += [("compact", 3, ["--size", "820x580"]), ("dark", 0, ["--dark"]), ("setup", 6, [])]
            cases += [("light", 0, ["--light"]), ("compact-apps", 1, ["--size", "720x520"])]
            for name, page, extra in cases:
                server.setup = name == "setup"
                output = args.output / f"fixture-{name}.png"
                result = subprocess.run([str(args.binary.resolve()), "--smoke-test", "--port", str(server.server_port),
                    "--page", str(page), "--screenshot", str(output), "--quit-after", "2600", *extra],
                    capture_output=True, text=True, timeout=20)
                diagnostic = result.stdout + result.stderr
                if result.returncode or not output.exists() or any(word in diagnostic for word in
                    ["ReferenceError", "TypeError", "Binding loop", "Unable to assign", "failed to load", "is not a type", "Cannot assign"]):
                    raise RuntimeError(f"Page {name} failed ({result.returncode}):\n{diagnostic}")
                print(f"PASS {name}: {output.name}", flush=True)
            assert all(path.startswith("/api/") and "/ai/" not in path for path in server.paths)
            for required in ["/api/config", "/api/apps", "/api/clients/list", "/api/runtime/sessions", "/api/logs"]:
                assert required in server.paths, required
            print(f"PASS HTTPS integration: {len(server.paths)} real requests, no AI endpoints")
        finally:
            server.shutdown()
            server.server_close()


if __name__ == "__main__":
    main()
