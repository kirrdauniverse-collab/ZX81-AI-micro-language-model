# DRAW pack v0.1

Canvas 64x32 pixels (2x2 block graphics in 32x16 cells). Commands (English, typed or handed over from the base):
- shapes: CIRCLE (ROUND, BALL), BOX (RECT), SQUARE, LINE, TRIANGLE
- size: a number (2..15) or BIG / LARGE / SMALL / TINY / LITTLE; BIGGER / SMALLER changes the last shape
- place: LEFT, RIGHT, CENTER, TOP, BOTTOM; NEXT (BESIDE, NEAR) puts it to the right of the last shape; nothing said = random place and size
- CLEAR; BACK (returns to the PICO pack)
Scene list (8 shapes) is redrawn after every change, so size and place can change later.
Planned: object definitions as primitive lists (house, car, Jupiter with four bands and a red spot ...), scaled and placed like shapes.
