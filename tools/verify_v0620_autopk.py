from pathlib import Path

root = Path(__file__).resolve().parents[1]
controller = (root / "src/controller.cpp").read_text(encoding="utf-8")
bridge = (root / "src/bridge.cpp").read_text(encoding="utf-8")
protocol = (root / "src/protocol.h").read_text(encoding="utf-8")
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")

checks = {
    "AUTO PK tab": 'L"AUTO PK"' in controller,
    "safe tab entry": "EnterAutoPkTabSafetyStop" in controller and "StopAutoPk" in controller,
    "group coordinator": "TickAutoPk" in controller and "AllActivePk" in controller,
    "hidden click reuse": "CoordinatorInternalPointAction" in controller and "capturePkClickIndex_" in controller,
    "travel guard reuse": "HandleRobustTravel" in controller and "AUTO PK BARRIER" in controller,
    "treatment ResID": "kTreatmentNpcResId = 339" in controller,
    "treatment commands": all(x in protocol for x in ["BeginBackgroundTreatment", "AdvanceBackgroundTreatment", "CloseBackgroundTreatment"]),
    "treatment bridge": all(x in bridge for x in ["BeginBackgroundTreatment", "AdvanceBackgroundTreatment", "CloseBackgroundTreatment"]),
    "protocol": "0x00010620u" in protocol,
    "auto pk test": "auto_pk_logic_tests" in cmake,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("AUTO PK verifier FAIL: " + ", ".join(failed))
print("AUTO PK verifier PASS")
