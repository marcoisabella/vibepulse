#ifndef TK_PAGE_ROTATION_H
#define TK_PAGE_ROTATION_H

#include <stdbool.h>
#include <stdint.h>

/* When the panel turns its own pages.
 *
 * A shelf screen that only ever shows the page someone last swiped to is a
 * page, not a display — and on AMOLED a page held for hours is also the
 * shape of its own burn-in. So the panel advances on its own.
 *
 * Kept out of the LVGL layer for the same reason as needs_you_policy: the
 * interesting part is the timing, every rule below has a way of being subtly
 * wrong, and timing is exactly what a host test can pin down without a
 * display attached.
 *
 * Two rules earn their place over "advance every N seconds":
 *   - A finger wins. Rotating away from the page someone deliberately swiped
 *     to, while they are still reading it, is the classic carousel failure.
 *   - The NEEDS YOU takeover owns the whole glass. Nothing rotates under it,
 *     and nothing snaps the instant it clears.
 * Both are expressed as "reset the dwell and stay put", so leaving either
 * state always costs a full, calm dwell rather than an immediate jump.
 */

typedef struct {
  int32_t dwell_ms;        /* how long one view holds; 0 disables rotation */
  int32_t resume_after_ms; /* idle time before a touched panel rotates again */
  int view_count;          /* views in the ring; 1 disables rotation */
} tk_page_rotation_cfg;

typedef struct {
  int64_t last_advance_us;
} tk_page_rotation_state;

void tk_page_rotation_reset(tk_page_rotation_state *state, int64_t now_us);

/* The view to show now, or -1 to stay put.
 *
 * Pure: every input is explicit, including the clock and the inactivity the
 * caller reads from LVGL, so the whole decision table is reachable from a
 * host test. `inactive_ms` is milliseconds since the last input event;
 * `takeover_visible` is the NEEDS YOU overlay owning the screen.
 */
int tk_page_rotation_next(const tk_page_rotation_cfg *cfg,
                          tk_page_rotation_state *state,
                          int current_view,
                          int64_t now_us,
                          uint32_t inactive_ms,
                          bool takeover_visible);

#endif
