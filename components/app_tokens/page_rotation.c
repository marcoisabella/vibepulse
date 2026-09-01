#include "page_rotation.h"

void tk_page_rotation_reset(tk_page_rotation_state *state, int64_t now_us) {
  state->last_advance_us = now_us;
}

int tk_page_rotation_next(const tk_page_rotation_cfg *cfg,
                          tk_page_rotation_state *state,
                          int current_view,
                          int64_t now_us,
                          uint32_t inactive_ms,
                          bool takeover_visible) {
  /* Configured off, or nothing to rotate to. Neither is a state worth
   * tracking time in. */
  if (cfg->dwell_ms <= 0 || cfg->view_count <= 1) return -1;

  /* The takeover owns the glass, and a finger owns the page it is resting
   * on. Both restart the dwell rather than merely pausing it, so leaving
   * either state costs a full calm dwell instead of snapping immediately to
   * the next page the moment the alert clears or the hand lifts. */
  if (takeover_visible || inactive_ms < (uint32_t)cfg->resume_after_ms) {
    state->last_advance_us = now_us;
    return -1;
  }

  if (now_us - state->last_advance_us < (int64_t)cfg->dwell_ms * 1000LL)
    return -1;

  state->last_advance_us = now_us;
  return (current_view + 1) % cfg->view_count;
}
