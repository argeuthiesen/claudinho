# Claudinho manual

**English** · [Português](manual.pt-BR.md)

How to use Claudinho day to day, once it's built and set up. To build and
install it, see the [README](../README.md).

## What the face shows

Claudinho reacts on its own to what Claude Code is doing:

| Face | When it shows |
|---|---|
| Happy | a session started |
| Thinking | you sent a prompt |
| Excited | prompt with "thanks", "it works", "perfect", "awesome"... |
| Worried | prompt with "error", "bug", "doesn't work", "failed"... |
| Startled | prompt with a swear word |
| Working | Claude is about to use a tool |
| Suspicious | 5 tool calls in a row |
| Angry | a tool call failed |
| Waiting for you | Claude needs your permission or an answer |
| Dizzy | compacting the context |
| Done | Claude just finished responding |
| Tired | idle, with 5-hour usage above 75% |
| Sweating | idle, with usage above 90% |
| Sleeping | no open session, or 3 minutes without news from the computer |

Mood words are matched in English and Portuguese, and your prompt text never
leaves your computer: only the label ("happy", "worried", "startled") goes to
the board.

## Plan usage

- **Tap the screen** to see the usage card: 5-hour and 7-day windows, when
  each one resets, how many terminals are open and the board's IP. After 15
  seconds it goes back to the face (or tap again).
- **It also shows it on its own** for a few seconds when Claude finishes a
  response and usage grew noticeably, or when usage crosses 50%, 75% or 90%.
  Above 90% the number blinks red. Any new event goes straight back to the
  face.
- The numbers only show up after Claude's first response in a session.

## Touch

| Touch | What it does |
|---|---|
| Short tap | switches between the face and the usage card |
| Long press (1.5 s) | toggles brightness between 100% and 15% |
| Tap when it asks to allow an update | authorizes the update |

When sleeping, it dims itself after 20 seconds.

## Changing the face color

Handy for matching the face to your case's filament color.

1. Ask Claude "change Claudinho's color" (or run `claudinho.sh cor`).
2. The screen shows **6 base colors**. Tap the one closest to your filament.
3. **12 shades of it** appear around the edges, with the tapped shade **shown
   large in the middle**. With the screen mounted in the case, the edge shades
   sit right against the printed frame, so you can compare each one directly
   with the filament, side by side. (Outside the case, hold a piece of
   filament next to the middle.)
4. Found it? **Tap the middle.** You get the face in that color with three
   buttons (labels are in Portuguese):
   - **Gravar** (save): keeps the color, even after a restart;
   - **Voltar** (back): back to the 12 shades;
   - **Cancelar** (cancel): back to the previous color.

If nobody touches it for 1 minute, it cancels on its own. The eyes always
stay black. If you know the exact color: `claudinho.sh cor R G B salvar`.

## Tic-tac-toe

Just for fun: ask Claude "let's play tic-tac-toe" (or run `claudinho.sh
velha`). Your opponent is Claudinho itself, running on the board, spending no
tokens, and each game it's in a different mood: sometimes it plays perfectly,
sometimes it slips.

- You are **X**: tap a square. It answers right after with **O**.
- At the end it strikes the winning line and reacts: **sad** if you won,
  **excited** if it won, **suspicious** on a draw. Then a new game starts on
  its own, and who goes first alternates.
- To quit: **3 quick taps on the same square**, or 2 minutes without touching.
- If Claude Code needs you mid-game, a notice shows up in the corner.

## Genius (Simon)

Ask Claude "let's play Genius" (or run `claudinho.sh genius`). It also runs on
the board, spending no tokens.

- Claudinho lights up a sequence of colors on the 4 quadrants; you repeat it
  by tapping in the same order. The number in the middle is the round.
- Every time you get it right, the sequence gets one more color and speeds up
  a little.
- Miss one: it shows your score with a face that depends on how far you got,
  and a new game starts on its own.
- To quit: **3 quick taps on the same quadrant** (correct taps in the sequence
  don't count, so play without worry), or 2 minutes without touching.

## Updating

Ask Claude "update Claudinho". The screen will ask to allow the update: **tap
it within 1 minute** (or press the board's BOOT button). Without the tap,
nothing is sent. That keeps anyone from swapping the firmware over the network
without being in front of it.

The update takes about 40 seconds and it restarts on its own. If the network
drops halfway, it keeps the current version; just ask again.

## Asking Claude

No need to memorize commands. Inside Claude Code, just ask:

- "update Claudinho"
- "change Claudinho's color"
- "show the usage on Claudinho"
- "Claudinho stopped reacting" (the skill runs a diagnosis)
- "I changed my Wi-Fi, reconfigure Claudinho"

The full command list is in the [README](../README.md#commands), and the most
common problems in [Troubleshooting](../README.md#troubleshooting).
