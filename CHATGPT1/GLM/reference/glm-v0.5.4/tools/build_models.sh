#!/bin/sh
# tools/build_models.sh - full pipeline: corpus -> train -> pack -> models/
# Runs on a plain CPU with numpy. Adjust --steps for quality vs. time;
# the defaults below give the checked-in preview quality in ~3 minutes.
set -e
cd "$(dirname "$0")/.."

STEPS_BASE=${STEPS_BASE:-6000}
STEPS_TOPIC=${STEPS_TOPIC:-3000}
STEPS_A=${STEPS_A:-2500}

echo "== corpus =="
python3 training/corpus_gen.py

echo "== base models =="
python3 training/train.py --class D --data training/data/base.txt \
    --out runs/base_d --steps "$STEPS_BASE"
python3 training/pack.py --npz runs/base_d.npz --bin models/base_d.bin \
    --golden tests/golden_d.txt --finetune-ok --stats runs/stats_d.json

python3 training/train.py --class C --data training/data/base.txt \
    --out runs/base_c --steps "$STEPS_BASE"
python3 training/pack.py --npz runs/base_c.npz --bin models/base_c.bin \
    --golden tests/golden_c.txt --finetune-ok --stats runs/stats_c.json

echo "== class A preview (full training needs a GPU, see docs/ROADMAP.md) =="
python3 training/train.py --class A --data training/data/base.txt \
    --out runs/base_a --steps "$STEPS_A"
python3 training/pack.py --npz runs/base_a.npz --bin models/classa_preview.bin \
    --finetune-ok --stats runs/stats_a.json

echo "== topic packs (class C fine-tunes, load-on-demand) =="
for topic in greetings weather animals retro numbers jokes; do
    python3 training/train.py --class C \
        --data "training/data/topics/$topic.txt" \
        --out "runs/topic_$topic" --steps "$STEPS_TOPIC"
    python3 training/pack.py --npz "runs/topic_$topic.npz" \
        --bin "models/topic_$topic.bin" --finetune-ok
done

echo "== done =="
ls -la models models/topics 2>/dev/null || ls -la models
