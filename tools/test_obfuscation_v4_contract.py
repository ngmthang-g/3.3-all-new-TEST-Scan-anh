from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding='utf-8')


def need(cond: bool, msg: str) -> None:
    if not cond:
        print(f'OBFUSCATION V4 CONTRACT FAIL: {msg}', file=sys.stderr)
        raise SystemExit(1)


header = read('src/native_obfuscation.h')
launcher = read('src/license_launcher.cpp')
protocol = read('src/protocol.h')
cmake = read('CMakeLists.txt')
build_config = read('src/license_build_config.h.in')

need('EncryptedLiteral' in header and 'TL_OBF_W' in header and 'TL_OBF_NOINLINE' in header,
     'compile-time encrypted literal helper missing')
need('volatile std::uint64_t runtimeKey' in header and 'RuntimeMask' in header,
     'generic decryptor can be trivially folded back to plaintext')

need('string(HEX "${TL_LICENSE_API_URL}"' in cmake and 'TL_LICENSE_API_URL_ENC' in cmake,
     'license API URL is not encoded during configure')
need('/GR-' in cmake and '/GF' in cmake,
     'release metadata/string hardening flags missing')
need('TL_LICENSE_API_URL L"' not in build_config and 'TlLicenseApiUrl()' in build_config,
     'generated build config still emits plaintext API URL macro')
need('volatile unsigned char runtimeKey' in build_config,
     'public API URL decoder is constant-foldable')

need('DecodeWideXor' in launcher and 'volatile std::uint16_t runtimeKey = 0x5A37u' in launcher,
     'numeric runtime string decoder missing')
for token in ['ValidateRoute()', 'HeartbeatRoute()', 'NetworkUserAgent()', 'HttpPostMethod()',
              'HttpJsonHeaders()', 'MachineGuidRegistryPath()', 'MachineGuidRegistryName()',
              'BiosRegistryPath()']:
    need(token in launcher, f'numeric string helper missing: {token}')
for plaintext in ['L"/validate"', 'L"/heartbeat"', 'L"AUTOThanLongPro/3.4"',
                  'L"Content-Type: application/json; charset=utf-8',
                  'L"SOFTWARE\\\\Microsoft\\\\Cryptography"',
                  'L"HARDWARE\\\\DESCRIPTION\\\\System\\\\BIOS"']:
    need(plaintext not in launcher, f'critical plaintext literal remains in launcher: {plaintext}')
need('23064,23105,23126,23131,23134,23123,23126,23107,23122' in launcher,
     'numeric /validate payload missing')
need('23064,23135,23122,23126,23109,23107,23125,23122,23126,23107' in launcher,
     'numeric /heartbeat payload missing')

need('gLicenseGuardCanary' in launcher and 'LicenseGuardCanary' in launcher,
     'runtime license canary missing')
need('LicenseGateLinear' in launcher and 'LicenseGateStateMachine' in launcher,
     'dual-path license gate missing')
need('return linear && stateMachine;' in launcher,
     'action gate does not require both independent paths')

# v4 introduced protocol 3.2. The current main baseline already advanced to 3.3
# for the isolated TestOpenBag command without changing the v4 proof contract.
need(('kProtocolVersion = 0x00030200u' in protocol) or
     ('kProtocolVersion = 0x00030300u' in protocol),
     'protocol older than v3.2')
need('LicenseProofPepper()' in protocol and 'volatile std::uint64_t p0' in protocol,
     'proof pepper is still one obvious static constant')
need('LicenseProofMulA()' in protocol and 'LicenseProofMulB()' in protocol,
     'runtime proof multipliers missing')
need('kLicenseProofPepper' not in protocol,
     'old static proof pepper still present')

print('OBFUSCATION V4 CONTRACT: PASS')
