#!/bin/sh
# Bring THIS Mac up as a VibePulse host, to the same state as any other.
#
# Written because a panel outlives the computer it was first paired with: the
# screen moves to a new desk, a new machine takes over, and everything the
# first host learned -- the shared device key, the provider choice, the hooks,
# the launchd service -- has to be reproduced exactly or the glass quietly
# loses half its abilities.
#
# Everything here is derived from the repository's own gitignored secrets.h,
# so no secret is ever printed, copied between machines by hand, or pasted
# into a chat. The one thing that MUST match across hosts is the device key,
# because it is compiled into the panel's firmware; this script reports a
# short fingerprint of it so two machines can be compared without either one
# revealing the value.
#
# Idempotent: safe to re-run. Reads nothing it does not own.
set -e
cd "$(dirname "$0")/.."
ROOT=$(pwd)

say() { printf '%s\n' "$*"; }
ok()  { printf 'PASS %s\n' "$*"; }
bad() { printf 'FIX  %s\n' "$*" >&2; }

say "VibePulse host setup — $(scutil --get LocalHostName 2>/dev/null || hostname)"
say "repo: $ROOT"
say ""

# ---------------------------------------------------------------- secrets.h
if [ ! -f secrets.h ]; then
  bad "secrets.h missing. Copy it from the host that built the running"
  bad "     firmware -- a different device key means the panel will refuse"
  bad "     this machine's answers. See docs/agent-setup.md step 1."
  exit 1
fi
ok "secrets.h present"

KEY=$(sed -n 's/.*TK_VIBEPULSE_DEVICE_KEY[^"]*"\([0-9a-fA-F]\{64\}\)".*/\1/p' secrets.h | head -1)
if [ -n "$KEY" ]; then
  printf '%s' "$KEY" > "$HOME/.vibepulse-device-key.tmp"
  # trailing newline: the reader tolerates it, and a file without one is
  # awkward to inspect
  printf '\n' >> "$HOME/.vibepulse-device-key.tmp"
  mv "$HOME/.vibepulse-device-key.tmp" "$HOME/.vibepulse-device-key"
  chmod 600 "$HOME/.vibepulse-device-key"
  FP=$(printf '%s' "$KEY" | shasum -a 256 | cut -c1-8)
  ok "device key installed (fingerprint $FP — must match every other host)"
else
  say "note: no TK_VIBEPULSE_DEVICE_KEY in secrets.h; the panel stays"
  say "      display-only on this host and answers fall back to the terminal."
fi

# ------------------------------------------------------------------- python
PY=python3
if [ ! -x .venv/bin/python ]; then
  for c in python3.11 python3.12 python3; do
    command -v "$c" >/dev/null 2>&1 && { PY=$c; break; }
  done
  "$PY" -m venv .venv
fi
.venv/bin/python -m pip install --quiet --upgrade pip >/dev/null 2>&1 || true
ok "venv: $(.venv/bin/python -V 2>&1)"

# The advertiser is what lets the panel FIND this host without a rebuild.
# Without it the panel can only reach whatever hostname was compiled in.
.venv/bin/python -m pip install --quiet -r requirements-discovery.txt
ok "discovery advertiser installed (zeroconf)"

# ---------------------------------------------------- provider choice (Claude)
# vibepulse_setup.py install refuses to run without the Codex binary even for
# a Claude-only choice (upstream issue #65), so write the same configuration
# the CLI would produce, through the project's own config layer.
.venv/bin/python - <<'PY'
import sys, pathlib
sys.path.insert(0, "tools")
import vibepulse_setup as vs
from tokenserver.vibepulse_config import load_config, save_config, config_lock

path = vs.default_config_path()
with config_lock(path):
    before = load_config(path)
    target = vs._chosen_config("claude", False, False, before)
    save_config(path, target)
print("PASS Claude interactions enabled (detail stays on this computer)")
PY

# ------------------------------------------------------------------ service
.venv/bin/python tools/vibepulse_macos_service.py install
.venv/bin/python tools/vibepulse_macos_service.py validate

# -------------------------------------------------------- Claude Code hooks
# Loopback only: Claude Code refuses HTTP hooks that resolve to the LAN, which
# is why the bridge splits loopback-in from LAN-out.
.venv/bin/python - <<'PY'
import json, pathlib, shutil, time
p = pathlib.Path.home() / ".claude" / "settings.json"
if not p.exists():
    print("FIX  ~/.claude/settings.json not found — is Claude Code installed?")
    raise SystemExit(0)
data = json.loads(p.read_text())
want = {
    "PreToolUse": [{"matcher": "AskUserQuestion", "hooks": [{
        "type": "http", "url": "http://127.0.0.1:8737/api/hook/question",
        "timeout": 120, "statusMessage": "Waiting for VibePulse…"}]}],
    "PermissionRequest": [{"matcher": ".*", "hooks": [{
        "type": "http", "url": "http://127.0.0.1:8737/api/hook/permission",
        "timeout": 120, "statusMessage": "Waiting for VibePulse…"}]}],
}
if data.get("hooks") == want:
    print("PASS Claude Code hooks already correct")
else:
    shutil.copy(p, p.with_suffix(f".json.bak.{time.strftime('%Y%m%d-%H%M%S')}"))
    data["hooks"] = want
    p.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n")
    print("PASS Claude Code hooks installed (previous settings backed up)")
PY

# ------------------------------------------------------------------- verify
# launchd returns before the socket is up; smoke-testing into that gap
# reports a dead service that is merely still starting.
say ""
say "waiting for the service to answer..."
i=0
while [ $i -lt 30 ]; do
  curl -s -m 2 http://localhost:8737/ >/dev/null 2>&1 && break
  i=$((i + 1)); sleep 1
done
.venv/bin/python tools/tokenserver/smoke.py || true
say ""
say "This host advertises as VibePulse-$(scutil --get LocalHostName 2>/dev/null || hostname)-local."
say "If more than one VibePulse host is on the LAN, the panel pins whichever"
say "it discovered first — that is DNS-SD ordering, not intent. Run only one,"
say "or enable the encrypted interaction relay on every host that must be able"
say "to reach the glass (docs/interaction-relay.md)."
