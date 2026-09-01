#include <stdio.h>
#include <string.h>

#include "../components/app_tokens/models_presenter.h"

static int failures;

static void check(const char *what, int condition) {
  if (!condition) { printf("FAIL %s\n", what); failures++; }
}

static int close_to(double a, double b) {
  double d = a - b;
  return (d < 0 ? -d : d) < 1e-9;
}

static tk_tokens with_models(int count, const char *const *names,
                             const double *tokens, const double *usd) {
  tk_tokens t = {0};
  t.model_count = count;
  for (int i = 0; i < count; i++) {
    snprintf(t.models[i].name, sizeof t.models[i].name, "%s", names[i]);
    t.models[i].tokens = tokens[i];
    t.models[i].usd = usd[i];
  }
  return t;
}

/* The page exists to show that the model you use most and the model that
 * costs you most are often different. Both shares must be computed, and
 * against the rows actually shown, or they will not sum on screen. */
static void test_shares_are_computed_for_both_measures(void) {
  const char *names[] = {"OPUS 5", "SONNET 5"};
  const double tok[] = {750, 250};
  const double usd[] = {90, 10};
  tk_tokens t = with_models(2, names, tok, usd);

  tk_models_view view;
  tk_models_build(&t, &view);

  check("two rows", view.count == 2);
  check("cost share of the dear model", close_to(view.rows[0].cost_share, 0.9));
  check("token share of the dear model",
        close_to(view.rows[0].token_share, 0.75));
  check("cost share of the cheap model",
        close_to(view.rows[1].cost_share, 0.1));
  check("token share of the cheap model",
        close_to(view.rows[1].token_share, 0.25));
  check("name carried through", strcmp(view.rows[0].name, "OPUS 5") == 0);
  check("totals carried", close_to(view.total_usd, 100));
}

static void test_no_models_means_no_rows(void) {
  tk_tokens t = {0};
  tk_models_view view;
  tk_models_build(&t, &view);
  check("no rows", view.count == 0);
  check("not usable", !view.has_data);
}

/* A month that cost nothing yet (only unpriced models) still has token
 * volume worth showing. Dividing by a zero total must not produce NaN on
 * the glass. */
static void test_zero_cost_does_not_divide_by_zero(void) {
  const char *names[] = {"MYSTERY"};
  const double tok[] = {1000};
  const double usd[] = {0};
  tk_tokens t = with_models(1, names, tok, usd);

  tk_models_view view;
  tk_models_build(&t, &view);
  check("one row", view.count == 1);
  check("cost share is zero, not NaN",
        close_to(view.rows[0].cost_share, 0.0));
  check("token share still meaningful",
        close_to(view.rows[0].token_share, 1.0));
}

/* Zero tokens AND zero cost is not data; it must not claim a full bar. */
static void test_all_zero_rows_claim_no_share(void) {
  const char *names[] = {"A", "B"};
  const double tok[] = {0, 0};
  const double usd[] = {0, 0};
  tk_tokens t = with_models(2, names, tok, usd);

  tk_models_view view;
  tk_models_build(&t, &view);
  check("rows survive", view.count == 2);
  check("no invented cost share", close_to(view.rows[0].cost_share, 0.0));
  check("no invented token share", close_to(view.rows[0].token_share, 0.0));
}

static void test_row_count_is_bounded_by_the_contract(void) {
  const char *names[] = {"A", "B", "C", "D", "E"};
  const double tok[] = {5, 4, 3, 2, 1};
  const double usd[] = {5, 4, 3, 2, 1};
  tk_tokens t = with_models(TK_MODEL_ROWS_CAP, names, tok, usd);

  tk_models_view view;
  tk_models_build(&t, &view);
  check("all contract rows are shown", view.count == TK_MODEL_ROWS_CAP);
  check("has data", view.has_data);
}

int main(void) {
  test_shares_are_computed_for_both_measures();
  test_no_models_means_no_rows();
  test_zero_cost_does_not_divide_by_zero();
  test_all_zero_rows_claim_no_share();
  test_row_count_is_bounded_by_the_contract();

  if (failures == 0) {
    printf("OK: all model-presenter tests pass\n");
    return 0;
  }
  printf("%d tests failed\n", failures);
  return 1;
}
