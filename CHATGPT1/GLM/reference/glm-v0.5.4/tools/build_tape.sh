#!/bin/sh
# build_tape.sh - assemble the topic tape into dist/:
#   block 0 = manifest.cfg (the app lists topics from it)
#   blocks 1..n = topic packs in manifest order
# plus the cassette LABEL with counter values - print it and stick it on
# the cassette, exactly like we did in the 80s.
set -e
cd "$(dirname "$0")/.."
mkdir -p dist

PACKS="greetings weather animals retro numbers jokes space poems"

python3 tools/tapewriter.py \
    --out dist/tape_topics.rtp \
    --tzx dist/tape_topics.tzx \
    --zx  dist/tape_topics.tap \
    --label dist/tape_label.txt \
    models/manifest.cfg \
    $(for t in $PACKS; do echo models/topic_$t.bin; done)

echo
echo "cassette label:"
cat dist/tape_label.txt
echo
echo "host simulator:"
echo "  RT_TAPE=dist/tape_topics.rtp ./host/host_chat models/base_c.bin"
echo "then: /topics (lists tape blocks), /load 7 (space), /load 8 (poems)"
