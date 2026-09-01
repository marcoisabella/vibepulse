#include <stdio.h>

#include "../components/app_tokens/page_rotation.h"

static int failures;

static void check(const char *what, int condition) {
  if (!condition) {
    printf("FAIL %s\n", what);
    failures++;
  }
}

#define S(us) ((int64_t)(us) * 1000000LL)

/* The shipped shape: six core views plus value-multiple, ten seconds each,
 * rotation resuming three quarters of a minute after the last finger. */
static tk_page_rotation_cfg cfg(void) {
  tk_page_rotation_cfg c = {
    .dwell_ms = 10000,
    .resume_after_ms = 45000,
    .view_count = 7,
  };
  return c;
}

/* Rotation is what makes the panel an ambient display rather than a page one
 * person picked once. It has to actually advance. */
static void test_advances_once_the_dwell_has_elapsed(void) {
  tk_page_rotation_cfg c = cfg();
  tk_page_rotation_state s;
  tk_page_rotation_reset(&s, S(100));

  check("holds while the dwell is still running",
        tk_page_rotation_next(&c, &s, 0, S(100) + 9999999LL, 60000u, false) == -1);
  check("advances to the next view at the dwell boundary",
        tk_page_rotation_next(&c, &s, 0, S(110), 60000u, false) == 1);
}

/* A second advance must wait a full dwell from the FIRST one, not from the
 * moment the caller happened to poll. At 10 Hz that is the difference
 * between a page every ten seconds and a page every hundred milliseconds. */
static void test_advance_restarts_the_dwell(void) {
  tk_page_rotation_cfg c = cfg();
  tk_page_rotation_state s;
  tk_page_rotation_reset(&s, S(100));

  check("first advance fires", tk_page_rotation_next(
      &c, &s, 0, S(110), 60000u, false) == 1);
  check("the very next tick holds", tk_page_rotation_next(
      &c, &s, 1, S(110) + 100000LL, 60000u, false) == -1);
  check("second advance waits a full dwell", tk_page_rotation_next(
      &c, &s, 1, S(120), 60000u, false) == 2);
}

/* The classic carousel failure: the screen keeps moving while someone is
 * reading the page they deliberately swiped to. Touch wins, every time. */
static void test_recent_touch_holds_the_page(void) {
  tk_page_rotation_cfg c = cfg();
  tk_page_rotation_state s;
  tk_page_rotation_reset(&s, S(100));

  check("a page touched one second ago does not move",
        tk_page_rotation_next(&c, &s, 3, S(200), 1000u, false) == -1);
  check("still held just under the resume threshold",
        tk_page_rotation_next(&c, &s, 3, S(300), 44999u, false) == -1);
}

/* ...and once the finger has been gone long enough, it picks up again —
 * after a fresh dwell, not instantly, or letting go would snap the page. */
static void test_rotation_resumes_a_dwell_after_the_finger_leaves(void) {
  tk_page_rotation_cfg c = cfg();
  tk_page_rotation_state s;
  tk_page_rotation_reset(&s, S(100));

  tk_page_rotation_next(&c, &s, 3, S(200), 1000u, false);
  check("does not snap the instant the idle threshold passes",
        tk_page_rotation_next(&c, &s, 3, S(200) + 1LL, 45000u, false) == -1);
  check("advances a full dwell after the finger left",
        tk_page_rotation_next(&c, &s, 3, S(210) + 1LL, 55000u, false) == 4);
}

/* NEEDS YOU owns the whole glass. Rotating underneath it would either steal
 * the alert or resume mid-cycle the moment it clears. */
static void test_takeover_freezes_rotation(void) {
  tk_page_rotation_cfg c = cfg();
  tk_page_rotation_state s;
  tk_page_rotation_reset(&s, S(100));

  check("no advance while the takeover is up",
        tk_page_rotation_next(&c, &s, 2, S(200), 60000u, true) == -1);
  check("no advance the moment it clears either",
        tk_page_rotation_next(&c, &s, 2, S(200) + 1LL, 60000u, false) == -1);
  check("a full dwell after it cleared, rotation returns",
        tk_page_rotation_next(&c, &s, 2, S(210) + 1LL, 60000u, false) == 3);
}

/* The pager is a ring, and the last view is the one most likely to be
 * reached by an off-by-one. */
static void test_wraps_from_the_last_view_to_the_first(void) {
  tk_page_rotation_cfg c = cfg();
  tk_page_rotation_state s;
  tk_page_rotation_reset(&s, S(100));

  check("view 6 of 7 wraps to 0",
        tk_page_rotation_next(&c, &s, 6, S(110), 60000u, false) == 0);
}

/* A single-view build (every optional page compiled out) must not sit there
 * "advancing" to the page it is already on. */
static void test_single_view_never_advances(void) {
  tk_page_rotation_cfg c = cfg();
  c.view_count = 1;
  tk_page_rotation_state s;
  tk_page_rotation_reset(&s, S(100));

  check("one view means nothing to rotate to",
        tk_page_rotation_next(&c, &s, 0, S(500), 60000u, false) == -1);
}

/* Disabled by configuration means disabled, not "dwell of zero". */
static void test_zero_dwell_disables_rotation(void) {
  tk_page_rotation_cfg c = cfg();
  c.dwell_ms = 0;
  tk_page_rotation_state s;
  tk_page_rotation_reset(&s, S(100));

  check("a zero dwell never advances",
        tk_page_rotation_next(&c, &s, 0, S(500), 60000u, false) == -1);
}

int main(void) {
  test_advances_once_the_dwell_has_elapsed();
  test_advance_restarts_the_dwell();
  test_recent_touch_holds_the_page();
  test_rotation_resumes_a_dwell_after_the_finger_leaves();
  test_takeover_freezes_rotation();
  test_wraps_from_the_last_view_to_the_first();
  test_single_view_never_advances();
  test_zero_dwell_disables_rotation();

  if (failures == 0) {
    printf("OK: all page-rotation tests pass\n");
    return 0;
  }
  printf("%d tests failed\n", failures);
  return 1;
}
