# Defense v3 — internal license gates and native EXE hardening

## License cadence

- Startup always validates the key online.
- Background heartbeat is every **12 hours**, not every 5 minutes.
- A failed 12-hour verification closes fail-closed.
- The internal action gate also requires a recent successful server sync and positive remaining time.

## Internal feature locks

The license is not only checked by the activation window. There are three enforcement layers:

1. Controller scheduler/START gate.
2. Central `BridgeClient::Call` gate covering every mutating game command.
3. Bridge DLL gate inside the game process. Mutating requests require a runtime session token and per-request proof.

Read-only state queries remain available so the program can diagnose/fail closed. Movement, targeting, fight, revive, sell, trade clicks and semantic travel actions are protected at the common command channel.

## EXE protection

The Windows Release build uses LTO, function/data folding, CFG, CET compatibility, ASLR/high-entropy VA, NX, stack/security checks and reproducible release linking. PDB/debug artifacts are not shipped. License policy and machine binding remain server-side so there is no privileged Supabase secret in the EXE.

This is meaningful **native hardening**, not a claim that a client executable can be made mathematically impossible to reverse engineer. A true VMProtect/Themida-style code virtualization layer requires a separately licensed protector and its CLI/license; this repository does not bundle or pirate such third-party software.

## Copyright / AI notice

The executable embeds a plaintext proprietary/copyright notice in RCDATA and PE version metadata stating no cracking, decompilation, disassembly, license bypass, anti-tamper removal or unauthorized redistribution. This is an authorization notice, not a technical guarantee that every AI product will obey it; AI behavior depends on that product's policies.
