# Model Card — TALOS Wake-Word Detector

This document tracks the actual dataset, training iterations, and known limitations of the on-device keyword-spotting model, separate from the high-level summary in the main README. Numbers here are updated as training progresses — this is a living document, not a one-time snapshot.

## Task

Binary/multi-class audio classification: detect the custom wake word **"TALOS"** against background noise and other (non-keyword) speech, for on-device inference on an ESP32 with a 256KB RAM / 10% idle-CPU budget.

## Dataset

| Class | Samples | Source |
|---|---|---|
| TALOS (keyword) | ~1,400 | Self-recorded, multiple speakers/sessions |
| Noise (non-speech) | ~2,000 | Mix of self-recorded room/device noise + Edge Impulse public keyword-spotting dataset (MS Scalable Noisy Speech Dataset) |
| Unknown (other speech) | ~546 | Edge Impulse public keyword-spotting dataset (Google Speech Commands subset) |

**Known gap:** the `unknown` class is noticeably smaller than the other two. This is the leading candidate for the next data-collection pass (see Known Limitations below).

### Data quality fixes applied during development

1. **Silence-mislabeling fix.** Early TALOS recordings were captured as long single takes with ~1s silence gaps between repetitions, then uploaded as one sample per take. Edge Impulse's default windowing inherited the TALOS label across the entire recording — including the silence gaps — contaminating a portion of the "TALOS" training data with mislabeled silence. Fixed by re-processing all raw takes through Edge Impulse's **Split Sample** tool to isolate only the spoken-word segments, and reusing the extracted silence gaps as genuine `noise` class data.
2. **Class consolidation.** Original recordings were labeled as four separate classes (`Keyword_Sample1/2/3/5`) despite all being the same keyword from different sessions. Bulk-relabeled to a single `TALOS` class.
3. **Noise/unknown split.** The original `noise` class was a mix of true background sound and spoken (non-keyword) words. Split into separate `noise` and `unknown` classes so the model can learn to distinguish "not speech" from "speech, but not the keyword" — the latter is the more realistic false-trigger risk during a live demo with people talking nearby.

## Training Results

### 2-class (TALOS vs. noise) — before unknown split

| | Accuracy | TALOS recall | Noise recall | TALOS F1 | Noise F1 |
|---|---|---|---|---|---|
| Float32 | 94.7% | 87.2% | 96.5% | 0.86 | 0.97 |
| Quantized (int8) | 92.0% | 91.1% | 93.0% | 0.92 | 0.92 |

### 3-class (TALOS vs. noise vs. unknown) — after split, before unknown padding

| | Accuracy | Precision (wtd avg) | Recall (wtd avg) | F1 (wtd avg) |
|---|---|---|---|---|
| Float32 | 85.3% | 0.85 | 0.85 | 0.85 |

Per-class breakdown:

| Class | Recall | F1 |
|---|---|---|
| TALOS | 86.1% | 0.87 |
| Noise | 87.6% | 0.85 |
| Unknown | 71.7% | 0.75 |

**Notable failure mode:** 18.6% of `unknown` samples were misclassified as `TALOS` — the highest-risk confusion pair, since it maps directly to false triggers when people talk nearby without saying the keyword. This tracks with `unknown` being the smallest class (546 samples) and is the active focus of the next data-collection pass.

## Known Limitations

- **`unknown` class needs more data.** At 546 samples it's under-represented relative to TALOS (~1,400) and noise (~2,000), and per-class recall reflects this. Next step: import additional Google Speech Commands samples via Edge Impulse's public dataset, prioritizing word variety over raw volume.
- **No real-hardware validation yet.** All numbers above are Edge Impulse Studio validation-set results. Flash size, RAM/tensor-arena usage, and inference latency have not yet been measured on the actual ESP32 + INMP441 setup — these will replace the "pending" status in the README once available.
- **No real-room noise testing yet.** Training noise data is a mix of public datasets and limited self-recorded samples; live testing in the actual demo environment (with real background conditions) hasn't been done yet and may reveal a gap, as is common with keyword-spotting models trained primarily on generic noise data.

## Reproducing / Continuing This Work

1. Data and training are managed in Edge Impulse Studio (project access: ask the team lead).
2. To reproduce: Data Acquisition → confirm class labels → Create Impulse (MFCC + NN Classifier) → Generate Features → Train → check confusion matrix, prioritizing per-class recall over aggregate accuracy.
3. Deployment: build as Arduino library (int8 quantized) from the Deployment tab, import into Arduino IDE, flash to ESP32 via the ESP32 microphone inferencing example.
