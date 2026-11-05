#pragma once

// Called on CoreWindow's dispatcher thread after the legacy input tick.
void SampleUwpInput();
// Copies a sampled UWP pointer in client-area DIPs, false before first sample.
bool GetSampledUwpPointer(float& x, float& y, float& w, float& h);
// Takes accumulated wheel deltas in Windows WHEEL_DELTA units.
void TakeUwpWheel(int& horizontal, int& vertical);
