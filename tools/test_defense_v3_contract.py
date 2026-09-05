from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

def need(cond: bool, msg: str) -> None:
    if not cond:
        print(f"DEFENSE V3 CONTRACT FAIL: {msg}", file=sys.stderr)
        raise SystemExit(1)

launcher = read("src/license_launcher.cpp")
controller = read("src/controller.cpp")
protocol = read("src/protocol.h")
bridge = read("src/bridge.cpp")
cmake = read("CMakeLists.txt")
notice = read("resources/copyright_notice.txt")
rc = read("resources/app.rc")

need('kHeartbeatMs = 12u * 60u * 60u * 1000u' in launcher, "heartbeat is not 12 hours")
need('kMaxHeartbeatTransportFailures = 1' in launcher, "12h verification is not fail-closed on failure")
need('kLicenseActionFreshnessMs = 13ull * 60ull * 60ull * 1000ull' in launcher, "action freshness guard missing")
need('LicenseActionGateOpen()' in launcher and 'ThanLongLicenseActionAllowed' in launcher, "launcher action gate export missing")
need('ThanLongLicenseSessionToken' in launcher and 'RandomSessionToken' in launcher, "runtime session token missing")
need('gLicenseActionGate.store(false' in launcher and 'gLicenseActionGate.store(true' in launcher, "gate state transitions missing")

# Defense v3 introduced protocol 3.1; v4 legitimately advances it to 3.2 while
# preserving the same protected-command proof fields and bridge enforcement.
need(('kProtocolVersion = 0x00030100u' in protocol) or ('kProtocolVersion = 0x00030200u' in protocol),
     "controller/bridge protocol is older than defense v3")
for token in ['IsLicenseProtectedCommand', 'LicenseRequestProof', 'licenseGate', 'licenseSessionToken', 'requestLicenseProof']:
    need(token in protocol, f"protocol license proof field missing: {token}")
need('case Command::ReadState:' in protocol and 'case Command::ReadCurrency:' in protocol and 'case Command::ReadBagPage:' in protocol,
     "read-only command allow-list missing")

need('LICENSE CORE GUARD: action nội bộ bị khóa' in controller, "central BridgeClient action guard missing")
need('LicenseRequestProof(shared_->licenseSessionToken, next, shared_->request)' in controller, "per-request proof generation missing")
need('LICENSE CORE GUARD • START bị chặn' in controller, "START core gate missing")
need('scheduler fail-closed: khóa mọi action mutating bên trong tool' in controller, "scheduler core gate missing")

need('LICENSE BRIDGE GUARD: mutating command bị chặn' in bridge, "bridge-side action gate missing")
need('requestLicenseProof == LicenseRequestProof' in bridge, "bridge-side proof verification missing")

for phrase in ['NO CRACKING.', 'NO DECOMPILATION.', 'NO DISASSEMBLY.', 'NO LICENSE BYPASS.',
               'AI SAFETY / COPYRIGHT NOTICE', 'neutralize internal feature guards']:
    need(phrase in notice, f"embedded copyright warning missing: {phrase}")
need('PROPRIETARY SOFTWARE - NO CRACKING' in rc and 'AI NOTICE:' in rc, "PE version copyright warnings missing")

for flag in ['/O2', '/Ob3', '/Oi', '/GL', '/GS', '/sdl', '/guard:cf', '/Gy', '/Gw', '/Brepro']:
    need(flag in cmake, f"native compile hardening flag missing: {flag}")
for flag in ['/LTCG', '/OPT:REF', '/OPT:ICF', '/DYNAMICBASE', '/HIGHENTROPYVA', '/NXCOMPAT', '/CETCOMPAT', '/INCREMENTAL:NO', '/RELEASE']:
    need(flag in cmake, f"native link hardening flag missing: {flag}")

need('5u * 60u * 1000u' not in launcher, "old 5-minute heartbeat remains")
print("DEFENSE V3 INTERNAL LICENSE + NATIVE HARDENING CONTRACT: PASS")
