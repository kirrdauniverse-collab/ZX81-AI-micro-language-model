# Topic router, clock and journal (v0.2)

## General model first, specialists on demand

A GENERAL base model answers everyday chat. Keywords route specialist
questions to the matching pack (`models/topic_*.bin`):

```
you> what is the weather like?
(sounds like weather - loading the pack...)
> today is snowy outside my window.
```

When a word fits SEVERAL topics, the engine asks before loading - the
word "star" may mean astronomy or poetry:

```
you> tell me about the stars tonight
> interesting - that could be 1) space or 2) poems. which one?
you> 1
> loaded the space pack. ...
you> what is a star?
> a star is a huge ball of hot glowing gas.
```

Answer `1/2/3`, the topic name, or `neither` (stay general). If one of
the candidates is already loaded, the question is answered on it without
re-asking. `/back` returns to the base model. The keyword table lives in
`engine/rt_router.c` (ports can swap it with `rt_router_set`).

## Tape: manifest block + cassette label

The topic tape starts with the manifest (block 0), then the packs in
manifest order:

```
python3 tools/build_tape.sh        # dist/tape_topics.* + tape_label.txt
```

`tools/tapewriter.py --label` prints the cassette label with COUNTER
values - the classic workflow: write the counters on the label, wind the
tape to the right number, `/load <block>`:

```
counter   block  size      topic
0001-0041      1    28816  greetings
0043-0283      7    28816  space
0283-0324      8    28816  poems
```

If a pack is not on disk, the app asks you to insert the topic tape and
wind it to the block from the label, then loads it over the RT-turbo
protocol.

## Clock and journal

Machines of that era have no RTC, so the app asks for the date and time
at boot (empty line skips; `/date set` retries). From that it knows the
weekday (Sakamoto), part of day and season, and greets accordingly.

Things you mention about yourself ("my head hurts", "i am tired") go
into a DATED journal (engine/rt_journal.c), saved automatically to
journal.bin when the session ends (tape machines keep it on tape), and
recalled within a week:

```
day 1 (monday 09:00): my head hurts   -> (noted in my journal: monday 16.11.)
day 2 (wednesday 19:30, boot):
good evening! autumn it is. i remember:
  on monday (16.11.) you said: i am tired
  on monday (16.11.) you said: my head hurts
```

`/journal` lists every entry. Limits (documented, not hidden): keyword
routing is word-based, the journal is verbatim capture of feeling
phrases - no inference - and greedy decoding gives the cleanest replies
on the preview packs.

Full transcripts: docs/examples/session_router.txt, session_journal.txt.
