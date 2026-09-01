#ifndef TK_MODELS_PRESENTER_H
#define TK_MODELS_PRESENTER_H

#include <stdbool.h>

#include "tokens.h"

/*
 * Where the month went, per model.
 *
 * Two shares, not one, because they routinely disagree: a cheap model can
 * dominate the token volume and cost almost nothing, and that gap is the
 * only interesting thing the page has to say. Both are computed against the
 * rows actually shown, so the bars sum to the bar behind them.
 *
 * Pure, and out of the LVGL layer for the usual reason: division by a total
 * that can legitimately be zero is exactly the arithmetic worth pinning down
 * on a host rather than discovering as a NaN-wide bar on the glass.
 */

typedef struct {
  const char *name;
  double tokens;
  double usd;
  double token_share; /* 0..1 of the shown rows' tokens */
  double cost_share;  /* 0..1 of the shown rows' cost */
} tk_models_row;

typedef struct {
  tk_models_row rows[TK_MODEL_ROWS_CAP];
  int count;
  double total_tokens;
  double total_usd;
  /* False when the service sent no split at all: the page shows dashes
   * rather than an empty chart that looks like "you used nothing". */
  bool has_data;
} tk_models_view;

void tk_models_build(const tk_tokens *tokens, tk_models_view *out);

#endif
