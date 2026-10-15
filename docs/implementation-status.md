# TypaType implementation status — 14 October 2026

The agreed first-version features are present in the C++/Qt application. Linux compilation, core rules, desktop integration, and full-length countdowns have been performed. Human interaction/audio checks and builds on other operating systems remain separate verification work; they are not claimed complete.

| Brief section | Current implementation | Evidence / limit |
| --- | --- | --- |
| 1. Objective | Working offline desktop typing game | Play, Results, Progress, and Settings |
| 2. Technology | C++17, Qt 6 Widgets, QPainter, SVG, Multimedia, CMake | Full application compiled on Linux |
| 3. Scope | One local player; agreed modes only | No accounts, networking, multiplayer, shops, or extra languages |
| 4. Visual style | Violet light/dark palettes, rounded cards, borders, soft shadows | Both themes inspected in screenshots |
| 5. Main layout | Left logo, right navigation; Difficulty → kitten → Duration; full-width board | Restart below board at right |
| 6. Removed captions | Removed; short Space hint retained | Forbidden captions/preview controls absent from app sources |
| 7. Kitten | Original face and front paws, three expressions, brief gestures | Ten-word milestone tested once per round; completion celebrates |
| 8. Difficulty dialog | Exactly three choices, highlighted selection, fresh round | Keyboard, close, Escape, outside-click cancellation, restored focus tested |
| 9. Duration dialog | Exactly 15/30/60 seconds | All nine option combinations tested |
| 10. Defaults/counts | Simple, 30 seconds, 30 targets on every launch | Preferences restore independently; token counts exact |
| 11. Vocabulary | Bundled familiar English words, helpers/adjectives regularly included | No adjacent duplicates; fresh sequences; minimal-dictionary cases tested |
| 12. Text types | Lowercase; punctuation when required; short numbers at highest level | Punctuation/numbers remain within target count |
| 13. Timer | Monotonic clock; first printable starts; deadline freezes input | Real 15/30/60-second rounds pass; focus loss continues timer |
| 14. Word advance | Space submits nonempty words, advances past errors, locks exact matches; Enter is an optional alias | Updated per later user request; Space is not an insertion attempt |
| 15. Backspace | Deletes within a word; erased positions return to neutral grey; revisits adjacent editable words; locked boundary | Both correct/incorrect erasure, revisited words, end position, and retained accuracy tested |
| 16. Feedback | Wrong attempts turn expected letters red without replacing them; missing marks, caret, wrapping | Both themes tested; actual attempts retained for scoring/correction |
| 17. Restart/navigation | Fresh waiting round, selected options preserved | Active attempts abandoned without saving |
| 18. WPM | Final matching characters plus exact submitted separators | Selected duration used; erased input cannot inflate speed |
| 19. Accuracy | Correct insertion attempts / all insertion attempts | Corrections retain mistakes; missing characters add no attempts |
| 20. Results | Prominent WPM/accuracy, context, comparison, best indicators | Play Again/View Progress; failed saves still show results |
| 21. Unified history | All difficulties/durations together, separate metrics | Best WPM with accuracy, best accuracy with WPM, count, previous-round comparison |
| 22. Chart | Latest 20 initially, Older/Newer, WPM/accuracy toggle, real segments | Date/context point details, keyboard selection, historical best markers, recent list |
| 23. Settings | Immediate theme/size changes, live preview, sound categories, volume | Locally persisted and restored; muted categories gated |
| 24. Saving | Versioned JSON, atomic writes, validation, canonical IDs, write lock | Unreadable files preserved, newer results merged, understandable failure messages |
| 25. Architecture | Named session/generation/storage/rendering/page/theme/mascot/sound responsibilities | Core logic is independent of Qt painting |
| 26. Verification | Automated core, desktop, and real-time checks | Native human/audio and cross-platform checks still pending |
| 27. Delivery | Source, CMake, assets, dictionary, build/run/data/control/scoring documentation | Persistent local SDK available; platform installers are not included |

## Work completed during the continuation

- Replaced the missing temporary SDK with a persistent local SDK and repeatable setup/build scripts.
- Made launching rebuild changed source and rediscover old SDK-dependent paths.
- Prevented stale app windows from overwriting newer results; rechecked unreadable history at save time.
- Normalized UUIDs so equivalent spellings cannot duplicate a round.
- Added chart history navigation, persistent point details, keyboard inspection, and historical best markers.
- Added brief kitten motion and tested one-time milestones.
- Preserved helping-verb/adjective frequency with small vocabularies and number insertion.
- Moved optional sound initialization after gameplay updates and result delivery.
- Tested real countdowns and failure paths, including an open selector at expiry.
- Changed selectors to reusable asynchronous popups, fixed queued focus interference, and restored the parent window only for interactions inside TypaType.
- Made Space submit words, updated the hints, and retained target letters in red on wrong keystrokes.
- Added soft word-advance, Backspace, and encouragement sounds alongside typing and completion cues, respecting the existing mute/volume settings.
- Fixed inconsistent sound delivery with silent preloading, multiple typing/Backspace voices, and output-device updates. Desktop audio checks cover all cues, rapid repeats, volume, and mute during loading.
- Reset edited words' submission feedback on Backspace so erased letters remain neutral grey until a new attempt or submission.
- Kept the existing player history/preferences unchanged during development; all test profiles use temporary folders.

See [verification.md](verification.md) for the evidence and [README.md](../README.md) for running the project.
