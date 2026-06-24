# Fork Sharing Guide

> **Repository:** https://github.com/MartinXTX/flecs
> **Visibility:** ✅ Public
> **License:** MIT (same as upstream Flecs)
> **Status:** Production-ready Tier-v19.5 unified

---

## Ready-to-Share Texts

### Twitter / X (280 characters)

```
🚀 Flecs Tier-v19.5 fork released!

+93.4% sparse-data add/remove
-100% archetype transitions (LAZY)
+33% to +1357% with PGO
24 Tier patches + 14 upstream fixes + 3 C++17 templates

Production-ready: https://github.com/MartinXTX/flecs
```

### Reddit r/programming

**Title:**
```
[Open Source] Flecs Tier-v19.5 fork — +93.4% sparse-data + PGO bonus
```

**Body:**
```
I've been working on a high-performance fork of Flecs (https://github.com/SanderMertens/flecs)
focused on large-scale projects (>1M entities, >100 components).

Key results:
- +93.4% sparse-data add/remove (8.73 M → 16.89 M ops/s)
- -100% archetype transitions for data-only components (LAZY auto-heuristic)
- +33% to +1357% with Profile-Guided Optimization
- 24 Tier patches + 14 critical upstream fixes integrated
- 3 C++17 templates (view, system, simd_filter)
- 228+ tests PASS, 0 leak
- Production lib: 2.47 MB, PGO-optimized: 3.57 MB

All Tier patches are additive — zero API changes, 100% backward compatible.

Key insight: Tier-A architecture discovery discovered that Flecs already had sparse storage
(cr->sparse) but no public API to expose it. We added ECS_DATA_COMPONENT macro +
LAZY auto-heuristic + dispatch logic — 0 lines of architectural source changes.

Repo: https://github.com/MartinXTX/flecs
Quickstart: https://github.com/MartinXTX/flecs/blob/master/QUICKSTART.md
Performance: https://github.com/MartinXTX/flecs/blob/master/PERFORMANCE_COMPARISON.md

Honest findings documented (C2 -%3.4, B1 -%18 random, C3 1.0x — all default OFF).
```

### Hacker News

**Title:**
```
Show HN: Flecs Tier-v19.5 fork — +93% sparse-data + PGO bonus
```

**Body:**
```
Hi HN,

I've been working on a performance fork of Flecs (ECS framework) over the
past months. Tier-A architecture discovery discovered that Flecs already had sparse
storage infrastructure (cr->sparse) but no public API to expose it.

Key insight: we added ECS_DATA_COMPONENT macro + LAZY auto-heuristic that
makes data-only components use sparse storage automatically. Zero architectural
source changes — reuses existing infrastructure.

Results (measured, MSVC 19.50 /O2):
- +93.4% sparse-data add/remove (8.73 M → 16.89 M ops/s)
- -100% archetype transitions (data-only LAZY)
- +33% to +1357% with Profile-Guided Optimization
- archetype_churn: 1 M → 10.13 M ops/s (+913%)
- lifecycle_create: 3 M → 18.30 M ent/s (+510%)
- large_world_iter: 250 G → 1697 G ent/s (+579%)

24 Tier patches (Tier-v17 4 + Tier-v18 2 + Tier-v19 12) + 14 critical upstream
fixes integrated + 3 C++17 templates (view, system, simd_filter).

All additive — zero API changes, 100% backward compatible.
Production-ready library: 2.47 MB.

Honest findings: some Tier patches didn't deliver expected gains
(C2 software prefetch -%3-4, B1 hot/cold random -%18, C3 SIMD gather 0.61x).
These are documented and default-OFF in the build.

Repo: https://github.com/MartinXTX/flecs
Quickstart: https://github.com/MartinXTX/flecs/blob/master/QUICKSTART.md
```

### LinkedIn

```
Just released Flecs Tier-v19.5 — a performance-focused fork of Flecs ECS
targeting large-scale projects (>1M entities, >100 components).

🎯 Key Results:
• +93.4% sparse-data add/remove
• -100% archetype transitions (data-only LAZY auto-heuristic)
• +33% to +1357% with Profile-Guided Optimization
• 24 Tier patches + 14 upstream fixes + 3 C++17 templates
• 228+ tests PASS, 0 leak

Key innovation: Tier-A architecture discovery — discovered Flecs already had sparse
storage but no public API. Added ECS_DATA_COMPONENT macro + LAZY heuristic
to expose it. Zero architectural source changes.

All Tier patches are additive — zero API changes, 100% backward compatible.

🔗 https://github.com/MartinXTX/flecs

#Flecs #ECS #GameDev #Performance #OpenSource #CPP
```

### Discord / Slack

```
🚀 Flecs Tier-v19.5 fork released!

Tier-A architecture discovery: +93.4% sparse-data, -100% archetype transitions
24 Tier patches + 14 upstream fixes + 3 C++17 templates
PGO bonus: +33% to +1357%
228+ tests PASS, 0 leak
Production lib: 2.47 MB

https://github.com/MartinXTX/flecs
```

---

## Sharing Channels

### GitHub
- **Repository:** https://github.com/MartinXTX/flecs
- **README:** Appears automatically on the main page
- **About section:** Can be updated via GitHub repo Settings → General → Description
- **Topics:** Settings → General → Topics (recommended: `flecs, ecs, performance, c99, cpp17, pgo, game-development`)

### Social Media
- Twitter/X — tweet text above
- Reddit — r/programming, r/cpp, r/gamedev, r/optimization
- Hacker News — Show HN format
- LinkedIn — professional network
- Discord — Flecs, C++ GameDev, Game Engines servers

### Communities
- Flecs Discord: https://discord.gg/BEzP5Rgrrp
- r/flecs (or mention in r/gamedev if it doesn't exist)
- Conferences like CppCon, Meeting C++

### Blog / Dev.to
```markdown
# Flecs Tier-v19.5: +93.4% Sparse-Data Performance for ECS

[Write 1500+ word article on dev.to or personal blog]

Topics:
1. Problem statement — large ECS scaling issues
2. Tier-A architecture discovery — finding sparse storage in plain sight
3. Tier-v17/v18/v19 — incremental hot-path optimizations
4. PGO bonus — +33% to +1357% measured
5. Honest findings — what didn't work and why
6. Production results — 228+ tests, 0 leak, 6 perf wins

Repository: https://github.com/MartinXTX/flecs
```

---

## GitHub Repo Settings (Recommended)

### General
- **Description:** `Performance-focused Flecs fork: +93.4% sparse-data, -100% archetype transitions, +33% to +1357% PGO bonus. 24 Tier patches + 14 upstream fixes + 3 C++17 templates.`
- **Website:** `https://www.flecs.dev/`
- **Topics:** `flecs`, `ecs`, `performance`, `cpp17`, `c99`, `pgo`, `game-development`, `entity-component-system`

### Features
- ✅ Issues (for bug reports)
- ✅ Pull requests (for Tier-v20 contributions)
- ✅ Wiki (optional, can move QUICKSTART + docs here)
- ✅ Discussions (optional, for community Q&A)

### Social Preview
GitHub Settings → Social preview → upload a screenshot of the Tier-v19.5
benchmark results or Performance Gains table.

---

## Quick Reference Card

```
📦 Repo:    https://github.com/MartinXTX/flecs
📜 License: MIT
🏷️ Topics:  flecs, ecs, performance, cpp17, pgo
📊 Tier:    v19.5 (production-ready)
📚 Docs:    QUICKSTART.md, TIER_V19_FORK.md, PERFORMANCE_COMPARISON.md
🔬 Tests:   228+ PASS, 0 FAIL, 0 leak
⚡ PGO:     +33% to +1357% measured
🎯 Use:     Clone, build_v19_quick.bat, link v19_flecs_patched.lib
```

---

## After Sharing

After sharing the fork:
1. **Issues** — if opened, address Tier-v20 deferred items
2. **PRs** — if received, evaluate Tier-A patches for upstream merge
3. **Stars** — if the count grows, add more workload test scenarios
4. **Forks** — if any, community-driven Tier-v20 development can begin
```