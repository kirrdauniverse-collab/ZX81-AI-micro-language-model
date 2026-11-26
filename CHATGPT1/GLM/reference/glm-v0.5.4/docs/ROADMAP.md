# Roadmap (v0.1 -> v0.2+)

This repo implements phase 1-2 of the 24-week plan in
`docs/plan/AI_retro_gepeken_fejlesztesi_terv.pdf` (Hungarian). The phases
below track the same milestones with the v0.1 baseline filled in.

## v0.1 (this release) - DONE

- [x] Portable C89 engine: int8 weights, Q7 activations, per-row shifts,
      no heap, no floats anywhere on the inference path
- [x] Char-level MLP trainer (numpy, CPU) + deterministic English corpus
      generator with topic slices
- [x] Model packer with calibration; float-vs-int agreement measured at
      pack time (97.5-100%)
- [x] Golden test: C engine == numpy simulator, bit-exact
- [x] Chat REPL shared by all ports: /teach /mem /forget /save /topics
      /load /temp /greedy /finetune /sim
- [x] Learning: fact memory (all ports) + output-layer fine-tuning
      (host/Amiga), tested
- [x] RT-turbo tape protocol + encoder (rtp/tap/tzx) + host tape HAL
- [x] C64 .d64 writer/reader, verified
- [x] Port sources: Amiga, C64, ZX Spectrum, ZX81, C16, Enterprise 128,
      TVC (statuses in STATUS.md)

## v0.2 - real model quality

- [ ] Train on a real corpus (TinyStories-style) on Colab/GPU; the
      trainer and packer already accept any .npz from train.py
- [ ] Quantization-aware fine-tuning pass in the trainer (the intsim
      exists; needs the STE loop)
- [ ] 2-layer transformer variant behind the same blob format
      (architecture byte in the header, version 3)
- [ ] cc65 CI job for the C64 port (apt install cc65 in CI)

## v0.3 - hardware bring-up

- [ ] Compile + run on real Amiga 500 (vbcc), C64 (cc65) and ZX Spectrum
      (z88dk); calibrate /sim timings from measurements
- [ ] Validate RT-turbo pulse reader on real tape (ZX first, then C64)
- [ ] Enterprise 128 EXOS verification; TVC ROM vectors
- [ ] Serial-stream "tape" loader for the Amiga

## v0.4+ - per the plan

- [ ] Hungarian charset variant (needs a 45+ accented charset; the
      format supports it via the embedded charset block)
- [ ] Speech-free UI polish: 40-column layouts, per-machine banners
- [ ] Community model zoo: per-machine topic packs, checksummed
