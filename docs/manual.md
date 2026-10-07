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
| Working | Claude is about to use a tool (or one of the scenes below) |
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

## Scenes while Claude works

While Claude uses tools, instead of the working face Claudinho plays a little
scene, with a mini Claudinho (arms, legs and all) in the corner:

| Claude is... | Scene |
|---|---|
| editing a file | a code editor: a terminal types `code claudinho.ino`, then colored code is typed line by line and scrolls |
| running a command | a terminal typing commands, with output scrolling |
| reading or searching the code | the Matrix rain |
| delegating to a subagent | an org chart: Claude on top, agents appearing below |

It's all make-believe, stored on the board: the code and commands on screen are
fake. Only the category (editing, terminal, reading, agent) leaves your
computer; file names and commands never do. Each scene stays at least 8
seconds, so the screen doesn't flicker; 12 seconds with no tool, it goes back
to the face. Any other event (done, needs you, error) goes straight to the
face. Tap to see the usage card. To try one: `claudinho.sh cena codando`
(`terminal`, `lendo`, `agente`).

## Plan usage

- **Tap the screen** to see the usage card: 5-hour and 7-day windows, when
  each one resets, how many terminals are open and the board's IP. After 15
  seconds it goes back to the face (or tap outside the buttons).
- **The buttons at the bottom** (on the usage card and on the printer
  panel):
  - **Tokens** and **Printer** switch between the two screens;
    a thin gold frame marks the one you're on. Without a printer set up,
    only Tokens shows.
  - **Keep**: the screen stays on, updating live, and never goes
    back to the sleeping face. The button then reads **Sleep**:
    tap it and Claudinho goes back to sleep. Handy to take it to another
    room as a portable printer monitor. Printer alerts still show up, and
    after your tap it returns to the kept screen.
- **It also shows it on its own** for a few seconds when Claude finishes a
  response and usage grew noticeably, or when usage crosses 50%, 75% or 90%.
  Above 90% the number blinks red. Any new event goes straight back to the
  face.
- The numbers only show up after Claude's first response in a session.

## Touch

| Touch | What it does |
|---|---|
| Short tap | on the face: opens the usage card; on the cards: the buttons (outside them, back to the face) |
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
   buttons:
   - **Save**: keeps the color, even after a restart;
   - **Back**: back to the 12 shades;
   - **Cancel**: back to the previous color.

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

## Bambu Lab printer (optional)

If you have a Bambu Lab 3D printer, Claudinho can watch it too. It connects
straight to the printer over your local network: no cloud, no server, no
tokens. **Tested on the P2S** (with AMS); other Bambu models with local access
(X1, P1, A1) should work but haven't been tested yet.

To turn it on, ask Claude "I have a Bambu printer" (or run `claudinho.sh bambu
PRINTER_IP`). You'll need the printer's IP and **access code** (printer
settings > Network). The access code is a secret like your Wi-Fi password: you
type it hidden, in your own terminal, and it's stored only on the board.

**The panel.** The printer model and its state (printing, paused...), the
AMS humidity and temperature (measured inside the AMS, where the spools are),
three columns (percent printed, time left, current layer), a progress bar in
the color of the filament being printed, nozzle, bed and chamber temperatures, and the
4 AMS colors with the current one framed. (The name you gave the printer in
the app lives only in Bambu's cloud, so the panel shows the model. The print
job name isn't shown either: over the local network the printer only sends
Bambu Studio's "project + plate" or the MakerWorld profile name, which don't
say what the part is.) While printing, it shows up on its own every 5 minutes
for 15 seconds. Open it anytime with the **Printer** button on the usage
card, and use **Keep** to keep it on screen.

**The alerts** show "3D PRINTER" and the model at the top, a
big title in the alert's color, the part name, the detail, and a little
Claudinho at the bottom: jumping for good news, waving its arms when something
needs you. They stay on screen **until you tap** (the tap means "got it"); if
several pile up, they show one after the other:

| Alert | When |
|---|---|
| Started | a print started, with the estimated time |
| Paused | with the reason: out of filament, you paused, clogged nozzle, first layer error, front cover, temperature... |
| Resumed | back to printing after a pause |
| 5 min left | 5 minutes left |
| Filament changed | filament change, with the new color (one alert, kept up to date) |
| Finished! | with how long it took |
| Failed / Cancelled | failed (with the error code) or cancelled |
| HMS warnings | the printer's health warnings, in plain words (see below) |
| AMS is humid | AMS humidity reached 50% |

**HMS warnings, in plain words.** Bambu printers report problems as HMS codes
(like `0500-0200-0002-0005`). Claudinho carries Bambu's whole list (about 2,000
codes), rewritten as short phrases in your language: the title is the area (AMS,
nozzle / extruder, heated bed, network / internet...), the detail says what
happened, and the code stays small at the bottom, to look up in Bambu's wiki.
The color follows how serious it is: blue is informative, yellow needs
attention, red is serious.

**Tap or 20 seconds.** Informative warnings (no internet, clock sync, live
view, filter life...) **leave on their own after 20 seconds** and don't repeat
for 30 minutes: when the internet drops and comes back, it won't pop up every
time. Everything else waits for your tap.

During a game, the color palette or an update, the alerts wait until you're
back on the face. To see what they look like: `claudinho.sh alerta bom` (good
news), `ruim` (a pause), `filamento` or `hms`. To turn it off: `claudinho.sh bambu desligar` (erases the
code from the board).

## Demo mode

Want to film Claudinho or show it to someone? Run `claudinho.sh demo` (or ask
Claude "run Claudinho's demo"). In about 2 minutes it goes through
everything on its own: sleeping and waking up, the faces, plan usage, the
work scenes, the printer panel and alerts, the color calibration, a game of
tic-tac-toe and one of Simon with "ghost" taps, and back to sleep.

It's all make-believe: your real numbers, the printer, the face color and
any pending alerts come back exactly as they were. During the demo, events
from the computer and the printer are ignored, so nothing interrupts it.
**Tap the screen to stop it** (or `claudinho.sh demo parar`). It uses the
screen language you picked. The printer part only shows if a printer is set
up.

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
- "change Claudinho's language" (screen texts in English or Portuguese, and
  [your language, if you translate it](../idiomas/TRANSLATING.md))
- "I have a Bambu printer" / "show the printer panel"

The full command list is in the [README](../README.md#commands), and the most
common problems in [Troubleshooting](../README.md#troubleshooting).
