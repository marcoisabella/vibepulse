#ifndef APP_TOKENS_CONFIG_H
#define APP_TOKENS_CONFIG_H

#ifdef ESP_PLATFORM
#include "secrets.h"
#endif

/*
 * Reläets adresser, härledda ur EN valfri bas i secrets.h.
 *
 * Reläet är en brevlåda på internet dit tjänsten LÄGGER sina färdiga
 * siffror, så panelen kan hämta dem utan att dela nät med värden. Utan
 * TK_VIBEPULSE_RELAY_URL blir varje adress NULL och hämtningarna beter sig
 * exakt som förut — LAN eller ingenting.
 *
 * GRÄNSEN, medveten och testad: reläet bär SIFFROR, aldrig AKTIVITET.
 * Kvot, burn rate, Max Tracker och GitHub går den vägen. Agentstatus och
 * Needs You gör det INTE — de bär projektnamn, frågetexter och kommandon,
 * och de lämnar aldrig LAN:et. Enhetsnyckelns svarsväg är av samma skäl
 * LAN-only: panelen kan svara på en fråga hemma, aldrig via en brevlåda.
 */
#ifdef TK_VIBEPULSE_RELAY_URL
#define TK_TOKENS_RELAY_URL      TK_VIBEPULSE_RELAY_URL "/api/tokens"
#define TK_MAX_TRACKER_RELAY_URL TK_VIBEPULSE_RELAY_URL "/api/max-tracker"
#define TK_GITHUB_RELAY_URL      TK_VIBEPULSE_RELAY_URL "/api/github"
#else
#define TK_TOKENS_RELAY_URL      NULL
#define TK_MAX_TRACKER_RELAY_URL NULL
#define TK_GITHUB_RELAY_URL      NULL
#endif

/* The heaviest-model weekly window. Not every plan exposes it -- the field
 * arrives null and the page is dashes forever -- so it is switchable. On by
 * default; turn it off in secrets.h when your account never reports one. */
#ifndef TK_MODEL_WEEK_PAGE_ENABLED
#define TK_MODEL_WEEK_PAGE_ENABLED 1
#endif

#if TK_MODEL_WEEK_PAGE_ENABLED != 0 && TK_MODEL_WEEK_PAGE_ENABLED != 1
#error "TK_MODEL_WEEK_PAGE_ENABLED must be 0 or 1"
#endif

/* Codex's own pages — its weekly quota and its Max Tracker heatmap — for
 * people who run Codex. Someone who only runs Claude is otherwise made to
 * swipe past two permanently dashed pages forever, and on a rotating panel
 * to WAIT on them. On by default so a fresh clone still shows both
 * providers; turn it off in secrets.h.
 *
 * This governs the PAGES only. Codex's live agent rows and its Needs You
 * takeover are driven by whether Codex is actually running and are not
 * affected — a panel with the pages off still announces a waiting Codex. */
#ifndef TK_CODEX_SCREENS_ENABLED
#define TK_CODEX_SCREENS_ENABLED 1
#endif

#if TK_CODEX_SCREENS_ENABLED != 0 && TK_CODEX_SCREENS_ENABLED != 1
#error "TK_CODEX_SCREENS_ENABLED must be 0 or 1"
#endif

/* Quota pages: Claude's model week and Claude's whole week, plus Codex's
 * week when its pages are on. Tracker pages: Claude's, plus Codex's. */
#define TK_QUOTA_PAGES   (1 + TK_MODEL_WEEK_PAGE_ENABLED + \
                          TK_CODEX_SCREENS_ENABLED)
#define TK_TRACKER_PAGES (1 + TK_CODEX_SCREENS_ENABLED)

/* The GitHub page and star popup are deliberately independent. A fresh clone
 * remains Claude/Codex-only until the user opts in through secrets.h. */
#ifndef TK_GITHUB_SCREEN_ENABLED
#define TK_GITHUB_SCREEN_ENABLED 0
#endif

#ifndef TK_GITHUB_NOTIFICATIONS_ENABLED
#define TK_GITHUB_NOTIFICATIONS_ENABLED 0
#endif

/* Sound is a third, independent opt-in. It still requires a platform backend
 * that has passed the display-DMA and physical-speaker gates. */
#ifndef TK_GITHUB_SOUND_ENABLED
#define TK_GITHUB_SOUND_ENABLED 0
#endif

#if TK_GITHUB_SCREEN_ENABLED != 0 && TK_GITHUB_SCREEN_ENABLED != 1
#error "TK_GITHUB_SCREEN_ENABLED must be 0 or 1"
#endif

#if TK_GITHUB_NOTIFICATIONS_ENABLED != 0 && \
    TK_GITHUB_NOTIFICATIONS_ENABLED != 1
#error "TK_GITHUB_NOTIFICATIONS_ENABLED must be 0 or 1"
#endif

#if TK_GITHUB_SOUND_ENABLED != 0 && TK_GITHUB_SOUND_ENABLED != 1
#error "TK_GITHUB_SOUND_ENABLED must be 0 or 1"
#endif

#endif
