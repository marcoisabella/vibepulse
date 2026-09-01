#include "models_presenter.h"

#include <string.h>

void tk_models_build(const tk_tokens *tokens, tk_models_view *out) {
  memset(out, 0, sizeof *out);
  if (!tokens || tokens->model_count <= 0) return;

  int count = tokens->model_count;
  if (count > TK_MODEL_ROWS_CAP) count = TK_MODEL_ROWS_CAP;

  for (int index = 0; index < count; index++) {
    out->total_tokens += tokens->models[index].tokens;
    out->total_usd += tokens->models[index].usd;
  }

  for (int index = 0; index < count; index++) {
    const tk_model_row *src = &tokens->models[index];
    tk_models_row *row = &out->rows[index];
    row->name = src->name;
    row->tokens = src->tokens;
    row->usd = src->usd;
    /* A zero total is a real state -- a month of unpriced models, or a
     * freshly reset counter -- not an error. It yields no share rather
     * than a NaN, so the bar is empty instead of undefined. */
    row->token_share =
        out->total_tokens > 0 ? src->tokens / out->total_tokens : 0.0;
    row->cost_share = out->total_usd > 0 ? src->usd / out->total_usd : 0.0;
  }

  out->count = count;
  out->has_data = true;
}
