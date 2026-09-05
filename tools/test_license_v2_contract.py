from pathlib import Path
import hashlib
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def need(condition: bool, message: str) -> None:
    if not condition:
        print(f"LICENSE V2 CONTRACT FAIL: {message}", file=sys.stderr)
        raise SystemExit(1)


controller = read("src/controller.cpp")
launcher_path = ROOT / "src/license_launcher.cpp"
launcher = launcher_path.read_text(encoding="utf-8")
edge = read("supabase/functions/license-api/index.ts")
web = read("supabase/web/app.js")
html = read("supabase/web/index.html")
migration = read("supabase/migrations/20260828_license_security_v2.sql")
hardening = read("supabase/migrations/20260828_harden_rls_helper_execute.sql")

# UI requested hidden, while the underlying runtime/log plumbing remains intact.
need('ShowWindow(tradeStatus_, SW_HIDE)' in controller, "coordinator status is not hidden")
need('ShowWindow(logCaption_, SW_HIDE)' in controller and 'ShowWindow(log_, SW_HIDE)' in controller,
     "coordinator log region is not hidden")
need('Dòng mô tả kỹ thuật InputSync/callback được ẩn' in controller,
     "F8/InputSync technical row is still rendered")
need('Dòng mô tả nội bộ MAIN/FIFO/batch được ẩn' in controller,
     "MAIN/FIFO technical row is still rendered")
need('BotTokenProtected/DPAPI được ẩn khỏi giao diện' in controller,
     "Telegram DPAPI technical note is still rendered")

# Countdown is driven by the launcher/server remainingSeconds value.
need('ThanLongLicenseRemainingSeconds' in controller and 'FormatLicenseRemaining' in controller,
     "license countdown bridge/formatter missing")
need('RefreshLicenseTitle();' in controller and 'KEY: ' in controller,
     "window-title countdown refresh missing")
need('ThanLongLicenseRemainingSeconds' in launcher,
     "launcher does not expose countdown seconds")
need('remainingSeconds' in launcher,
     "launcher does not consume server remainingSeconds")

# Lock the reviewed launcher v2/v3/v4 source after canonicalizing Windows line endings.
launcher_bytes = launcher_path.read_bytes().replace(b'\r\n', b'\n')
launcher_sha = hashlib.sha256(launcher_bytes).hexdigest()
need(launcher_sha == '392307732faaebf3241aa66389180e48cc4959dd0d66d37a1a5f061a7ea7a4e0',
     f"license launcher reviewed normalized SHA mismatch: {launcher_sha}")

# Edge routes delegate license state transitions to security-definer RPC v2.
need('new Set([3, 15, 30, 60, 90, 365])' in edge,
     "30-day key type missing from Edge Function")
need('license_validate_v2' in edge and 'license_heartbeat_v2' in edge,
     "Edge Function is not using license v2 RPCs")
need('/admin/deleteKey' in edge, "blocked-key delete endpoint missing")

# Database is authoritative for machine binding and automatic mismatch block.
migration_lower = migration.lower()
need('check (duration_days in (3,15,30,60,90,365))' in migration_lower,
     "database duration constraint does not include 30 days")
for token in ['machine_profile', 'legacy_machine_hash', 'blocked_reason',
              'mismatch_machine_hash', 'mismatch_machine_profile', 'machine_mismatch']:
    need(token in migration, f"database v2 enforcement token missing: {token}")
need('license_validate_v2' in migration and 'license_heartbeat_v2' in migration,
     "database v2 RPC migration missing")
need("status = 'blocked'" in migration_lower,
     "database mismatch path does not set blocked status")
hardening_lower = hardening.lower()
need('revoke all on function public.rls_auto_enable() from public, anon, authenticated;' in hardening_lower and
     'grant execute on function public.rls_auto_enable() to service_role;' in hardening_lower,
     "legacy SECURITY DEFINER helper hardening missing")

# Web Admin: 30 days + delete control rendered only when the row is not allowed;
# backend /admin/deleteKey independently rejects a non-blocked row.
need('value="30"' in html and '>30 ngày<' in html,
     "30-day option missing from Web Admin")
need('data-action="delete"' in web and '/admin/deleteKey' in web,
     "delete-blocked-key UI missing")
need('const allowed = item.status === "allowed";' in web and
     '${allowed ? "" : \'<button class="small danger" data-action="delete">Xóa</button>\'}' in web,
     "delete action is not gated by allowed/blocked state")
need('Tự chặn: phát hiện PC khác' in web and 'fingerprintVersion' in web,
     "Web Admin mismatch/fingerprint display missing")

print("LICENSE V2 UI + SECURITY CONTRACT: PASS")
