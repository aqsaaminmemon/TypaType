# Verification — 14 October 2026

## Performed

- Configured and compiled the full C++17 application with CMake 4.2, GCC 15.2, and Qt 6.10.2 (Widgets, SVG, Multimedia, Test) on Ubuntu x86-64.
- Qt development packages now live persistently in `.deps/qt`; installed Qt runtime libraries are reused. The earlier `/tmp` SDK was removed between sessions, and the updated build was verified without it. System packages were not changed.
- Previously ran the full suite: **3/3 suites passed**. After the Space control, target-letter feedback, and sound changes, rebuilt the application and reran the core and desktop suites: **2/2 passed**. Countdown logic was unchanged, so the full-length timing suite was not repeated for these changes.
- The core executable reported **63,552 assertions passed**, covering 900 bundled-vocabulary rounds, 720 minimal-vocabulary rounds across 80 seeds, and input/scoring scenarios. Generation now preserves category frequency when repairing duplicates and inserting numbers.
- Rapid selector/focus regression passed five consecutive runs, including keyboard, cancellation, and outside-app dismissal.
- The latest desktop QtTest run reported **21 passed, 0 failed, 0 skipped**, including setup/cleanup and 19 substantive test cases. New coverage checks Space behavior, target-preserving red rendering in both themes, neutral Backspace feedback for correct/incorrect input in revisited words, correction/scoring, sound-cue events, and all five bundled PCM WAV assets.
- In the initial implementation, started the real desktop executable using `./run.sh --data-dir "$PWD/.local-data"`.
- Ran the dedicated desktop audio test outside the restricted sandbox at 20% game volume: **6 passed, 0 failed** (including setup/cleanup). All five cues and their nine reusable voices reached `QSoundEffect::Ready`; preload stayed silent; each cue started and completed naturally; rapid repeated input played; category mute, zero volume, re-enabling, and mute-before-loading passed. The system default speaker output was unmuted. Actual speaker sound/loudness was not assessed by listening.

## Automated coverage

| Area | Evidence |
| --- | --- |
| Startup | Simple, 30 seconds, exactly 30 targets, Ready state, typing focus, empty initial history |
| Generation | Exactly 15/30/60 tokens, all difficulties, punctuation/numbers present only as required, no immediate duplicates, consecutive sequences differ, regular helpers/adjectives |
| Timer | Empty Space/Enter/Backspace/modifiers/clicks/shortcuts do not start it; printable/incorrect input does; expiry occurs at the monotonic deadline; later input is rejected |
| Input rules | Space advances past errors and locks exact words; Enter remains an optional alias; repeated empty spaces do nothing; Backspace revisits editable adjacent words, deletes on return, stops at locked boundaries, applies from the end position |
| Feedback/scoring | Different wrong keystrokes render the same red target; erasing correct/incorrect input in revisited words renders identically to a fresh matching prefix, with erased positions grey in both themes; corrections retain mistake attempts; Space is not an attempt; erased characters earn no WPM; unfinished matching characters count; exact submitted words earn separators; zero attempts are safe |
| Sounds | Five bundled mono 16-bit PCM cues preload silently; nine reusable voices support rapid input; all cues load, start, and finish on the desktop output; repeating a playing cue restarts instead of skipping it; category mute/zero volume stop playback; mute-before-loading does not replay stale input |
| Fixed word sets | Submitting all targets leaves Running state without additional words |
| Restart/navigation | Ready reset, focus restoration, option preservation, discarded attempts excluded from history |
| Completion | Frozen result, one completion emission, one saved record even after repeated polling; expiry closes an open selector; failed saves preserve the original file and still show results without a personal-best claim; Play Again retains options; reopening restores history with launch options reset |
| Selectors | All combinations; current highlight; selection applies immediately; Escape, close button, and outside click preserve the cancelled choice; arrow keys/Enter; focus restoration |
| History | Full-precision round trip, unified difficulties, canonical duplicate-ID suppression, invalid-entry skipping, invalid result rejection, unreadable-container preservation both at startup and after startup, lock contention, and merging newer records saved by another instance |
| Settings | Theme, 40px text, both muted categories, and zero volume persist; selector defaults reset after reopening |
| Progress | Empty history, a single result, 25/45 isolated fixture results, latest-20 initial chart, Older/Newer navigation, both metric toggles, persistent selected-point details, keyboard inspection, and minimum-size layout |
| Kitten | Tenth locked word triggers encouragement; it returns to Welcome and does not repeat that milestone during timer refreshes; Restart resets milestones; short motion replaces continuous movement |

Core timing tests inject monotonic time for precise deadline boundaries. The separate real-time Qt tests additionally waited through full 15-, 30-, and 60-second rounds with the typing board unfocused, checking rejected late input, unchanged targets, final-state WPM, retained error attempts, one completion event, and one saved result:

| Selected duration / difficulty | Measured elapsed time |
| --- | --- |
| 15 seconds / Simple | 15,004 ms |
| 30 seconds / Punctuation | 30,005 ms |
| 60 seconds / Punctuation + Numbers | 60,003 ms |

All three passed. These measurements describe this run and are not a guaranteed latency on every machine. Desktop tests disable actual audio using `TYPATYPE_DISABLE_AUDIO=1`, which has no effect in normal launches unless explicitly set.

## Visual inspection

Captured and inspected application screenshots at 1120×820 and 820×700, including light Play, character feedback, dark Settings, dark Play at 40px, both dialogs, Results, empty Progress, single-point Progress, and populated Progress with range navigation and point details. Point details were moved above the chart to keep them visible. Fixed a partial text-row clip at minimum size. Confirmed:

- Header logo at left, navigation at right.
- Difficulty → small kitten → Duration, above the board.
- Full-width board and right-aligned Restart beneath it.
- Original white face and front paws, pink cheeks/pads, violet bow; no body/tail.
- Readable violet/coral feedback and both palettes.
- Inspected `target-feedback-light.png` and `target-feedback-dark.png`: typing a wrong letter for `cat` leaves the `c` visible in red. The updated Play screenshot shows the Space hint; corrected-letter screenshots show the restored violet `c`.
- Inspected `backspace-neutral-light.png` and `backspace-neutral-dark.png`: erased positions use the same neutral shade as untouched targets and have no red highlight or missing underline.
- No overlap in essential controls at the supported minimum size.
- Selectors use two reusable asynchronous popups. Parent activation is restored after closure only when the interaction remained in TypaType; clicks outside the app do not request activation. Queued typing-focus restoration is guarded against a newly opened popup. Tests cover repeated rapid use and wait for asynchronous restoration.
- No removed captions or preview-only controls in application sources/assets.

`docs/screenshots/progress-fixture-*.png` use test records in a temporary folder. `results-completed-test.png` uses an artificially expired test round. These are verification artifacts, not preloaded player scores. A new profile starts empty; existing player history is restored.

## Remaining checks and limits

- Human play-throughs remain useful for comfort, rapid correction behavior, and long extra input; all three real countdown durations have now passed automated desktop checks.
- Native window-manager focus, outside-click behavior, and resizing should be tried on the intended deployment desktop; automated interaction and image inspection used Qt's offscreen platform.
- Human listening for comfortable volume remains useful; automated desktop audio checks verify readiness, playback state, natural completion, rapid requests, and mute behavior.
- Windows/macOS compilation, deployment, high-DPI layouts, and installer packaging are unverified.
- The minimum supported content size is 820×700. Smaller displays may require system scaling or a later compact layout.
- The supplied text brief provided layout and colors; no separate approved-design image was available for exact visual comparison.
- Current app windows coordinate history writes with a lock. External editors/older app versions that do not use this lock are outside that coordination.
- The SDK is now persistent within the project, but it reuses this host's installed runtimes. It is not a portable installer; see the README for setup on another machine.
