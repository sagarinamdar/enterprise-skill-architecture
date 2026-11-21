# Enterprise Skill Architecture: Eliminating AI Context Poisoning in Legacy Codebases

> **A deterministic, machine-verified framework for stopping AI coding assistants from learning and amplifying anti-patterns in enterprise C++ repositories.**

[![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![Sanitizers: ASAN/UBSAN](https://img.shields.io/badge/Sanitizers-ASAN%2FUBSAN-green.svg)](https://github.com/google/sanitizers)
[![CI Verification](https://github.com/sagarinamdar/enterprise-skill-architecture/actions/workflows/verify.yml/badge.svg)](https://github.com/sagarinamdar/enterprise-skill-architecture/actions/workflows/verify.yml)
[![Architecture: 3-File Skill](https://img.shields.io/badge/Architecture-3--File%20Skill-orange.svg)](#core-architecture-the-3-file-separation)
[![Verified on Google Antigravity](https://img.shields.io/badge/Verified-Google%20Antigravity-4285F4.svg)](#)

---

## 1. Executive Summary: The Context Poisoning Dilemma

AI coding agents promise speed, but brownfield enterprise codebases present a harsh reality: **without deterministic guardrails, AI generates negative ROI**. 

Unguided models mimic what they see. When dropped into legacy C++ repositories, in-context learning causes agents to copy historical C++98 anti-patterns (raw owning pointers, manual memory management, ad-hoc singletons). The typing speed gained by junior developers is wiped out by senior engineers spending days debugging subtle heap corruption and sanitizer aborts in production.

This repository provides an open-source, deterministic framework to **immunize coding agents against context poisoning** using three practical mechanisms: a **3-File Skill Architecture**, a **Dual-Pass Context Audit**, and **Deterministic Script Execution**.

---

## 2. Practitioner Origin: Battle Scars from Enterprise AI Adoption

This architecture was forged through the real-world experiences of a systems architect on the front lines of early enterprise AI rollouts. When teams first adopted coding assistants on mature codebases, reality quickly clashed with the hype:

* **The "Million-Dollar Asset" Fallacy**: Teams revere legacy codebases because they cost millions in human coordination. But feeding raw legacy code to an AI doesn't transfer that wisdom—the model cannot distinguish business rules from 15-year-old memory hacks. It simply industrializes technical debt.
* **The Frozen Model vs. Amnesiac Enterprise**: Foundation models arrive with frozen weights and zero awareness of internal architecture. Fine-tuning on legacy code is an expensive trap that bakes bad C++98 into parameters forever. Skills serve as an externalized, dynamic memory bank that enforces modern standards at runtime without touching model weights.
* **The Enterprise IP Citadel (Data Sovereignty)**: CISOs and legal teams will not upload entire proprietary codebases to external cloud AI. The Skill architecture runs all builds, static analysis, and AddressSanitizer checks **100% locally within your security perimeter**, eliminating bulk cloud RAG indexing and restricting model context to minimal policies and scoped diffs (compatible with local/VPC models for zero egress).
* **The Developer Prompting Ceiling**: No developer will write a 2,000-word prompt detailing 20 years of architecture rules for a 20-line bug fix. When developers give realistic, concise prompts, unguided models default to naive pattern matching. The Skill architecture attaches persistent policies automatically.
* **The "Groundhog Day" of Unpersisted Corrections**: When an engineer spends an hour steering an AI through a complex memory fix, that learning is lost the second the chat closes. Tomorrow, the model makes the exact same mistake. Codifying fixes into `skills/pattern-catalog.md` turns individual fixes into permanent repository memory.
* **Finite Context & Attention Dilution**: Dumping massive repos into 1M-token windows causes attention rot; key safety constraints get lost in boilerplate. Progressive disclosure keeps working context lean (~500 tokens), querying catalogs on demand and offloading build logs entirely to local scripts.
* **Verification Over Generation**: Writing syntax was never the enterprise bottleneck—proving that code is free of memory corruption, concurrency hazards, and leaks is where 90% of engineering effort lies. Compilers and sanitizers must deliver the verdict, not the LLM.

---

## 3. Core Architecture: The 3-File Separation

Rather than loading bloated, undifferentiated prompts into agent memory, the framework decouples constraints into three distinct lifecycle roles:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        3-File Skill Separation                         │
├──────────────────────────┬─────────────────────────┬───────────────────┤
│ 1. Policy                │ 2. Execution Playbook   │ 3. Failure Memory │
│ (skills/instructions.md) │ (skills/skill.md)       │ (skills/catalog)  │
├──────────────────────────┼─────────────────────────┼───────────────────┤
│ • Invariant standards    │ • Phased workflow       │ • Known anti-     │
│ • C++17 RAII mandatory   │ • Progressive tool call │   pattern keys    │
│ • Sanitizer clean gates  │ • Invokes local scripts │ • Scoped RAII     │
│ • Zero raw owning ptrs   │ • Zero LLM build flags  │   resolution maps │
└──────────────────────────┴─────────────────────────┴───────────────────┘
```

1. **Policy (`skills/instructions.md`)**  
   *What is non-negotiable.* Immutable rules of engagement: C++17 standard conformance, strict RAII lifecycle management, zero heap-use-after-free, and automated zero-tolerance ASAN exits.
2. **Execution Playbook (`skills/skill.md`)**  
   *How the agent executes.* Step-by-step progressive workflow. Mandates executing local verification scripts rather than improvising compiler flags or sanitizer invocations.
3. **Failure Memory (`skills/pattern-catalog.md`)**  
   *What went wrong before.* Reusable root-cause signatures and canonical modern resolution patterns (e.g. replacing ad-hoc teardown with scoped connection tokens and RAII resource managers).

---

## 4. The Deterministic Script Hook Principle

A frequent failure mode in autonomous coding agents is **command hallucination**: LLMs attempting to guess compiler paths, flags (`-fsanitize=address`, `-fno-omit-frame-pointer`), or CI targets.

Following [Google Cloud's Official Agent Skills Specification](https://cloud.google.com/discover/ai-agent-skills) and Anthropic's guidance on agent tool design, this framework pairs natural-language skills directly with executable bash/powershell scripts:

```
Agent Skill Workflow
         │
         ▼
[skills/skill.md] ──────> Executes: ./scripts/verify.sh
                                  │
         ┌────────────────────────┴────────────────────────┐
         ▼                                                 ▼
1. CMake Debug Build                            2. CTest under ASAN
   (-fsanitize=address,undefined)                  (Halts on leak/UAF)
         │                                                 │
         └────────────────────────┬────────────────────────┘
                                  ▼
                     Machine-Checked Verdict
                     (Zero opinion, 100% truth)
```

The LLM never generates or executes raw compilation strings—it triggers `./scripts/verify.sh` and consumes structured, deterministic compiler/sanitizer exit codes.

---

## 5. Dual-Pass Context Audit (Grounded vs. Ungrounded)

To stop legacy context from corrupting architectural decisions, the agent executes a structured Dual-Pass audit before touching code:

- **Pass 1 (Grounded in Repository Context):** Evaluates how existing code implements the functionality (e.g., raw pointer registries, manual teardown).
- **Pass 2 (Ungrounded / Isolated against Modern Standards):** Evaluates the requirement strictly against Modern C++17 RAII policies in `instructions.md`.
- **The Diff Gate:** Computes the divergence between Pass 1 and Pass 2. Any instance where Pass 1 mimics legacy patterns that violate Pass 2 is halted before code generation begins.

*See [`dual_pass_audit/diff_report.md`](dual_pass_audit/diff_report.md) for a recorded audit on a synthetic legacy test suite.*

---

## 6. Grounded Claims, Engineering Results & Explicit Caveats

To ensure absolute technical credibility, every claim made by this project is anchored to concrete engineering efforts, verified by compiler outputs, and bounded by explicit operational caveats:

| # | Grounded Claim | Known Industry Problem | Our Engineering Effort | Verified Result | Explicit Caveat / Limitation |
|---|---|---|---|---|---|
| **1** | **Elimination of Command Hallucination** | LLMs guess compiler/sanitizer flags and failed in 3/5 (60%) of baseline runs in our fixture. | Bundled deterministic verification hooks (`scripts/verify.sh`, `scripts/verify.ps1`, `scripts/verify.py`) directly inside the Skill. | **0 CLI command errors** across all treatment iterations. The agent invokes scripts instead of improvising CLI syntax. | Requires a working host build toolchain (CMake + Clang/GCC/MSVC or Python 3). Does not fix broken underlying build environments. |
| **2** | **Prevention of Context Poisoning** | In-context learning causes models to statistically mimic raw pointers (`new`, `delete`, `delete this;`) found in brownfield repositories. | Formulated non-negotiable policy (`instructions.md`) and enforced the Dual-Pass Diff Gate (Grounded vs. Ungrounded audit). | **100% elimination of raw owning allocations**; introduced `ScopedConnection` RAII tokens in all 5 evaluation runs. | The policy layer must be explicitly defined and maintained; the framework does not synthesize architectural rules without human policy specification. |
| **3** | **Eradication of Session Memory Amnesia ("Groundhog Day")** | Foundation models have frozen parametric memory; debugging a memory leak in one chat session does not prevent the model from repeating the same bug tomorrow. | Codified diagnosed failure signatures and verified modern C++ solutions into a version-controlled Failure Memory Bank (`pattern-catalog.md`). | Reduced mean turns-to-fix from **4.2 turns to 1.4 turns** on recurring lifecycle and observer failure signatures. | Requires human-in-the-loop review to validate and seed the canonical modern solution into the catalog the first time a bug is solved. |
| **4** | **Deterministic Memory Safety Verification** | LLMs hallucinate code correctness and cannot evaluate dynamic runtime memory safety, claiming buggy code is "clean." | Integrated automated AddressSanitizer (`-fsanitize=address`) and LeakSanitizer runtime gates into CTest scripts. | **0 AddressSanitizer errors** across all treatment runs vs. 4/5 runs failing with heap-use-after-free in the unguided baseline. | Sanitizers only detect memory corruption in code paths actively exercised by tests. Test suite execution coverage remains a prerequisite. |
| **5** | **Mitigation of Attention Dilution & Context Rot** | Dumping large repositories into 1M+ token windows causes attention degradation ("lost in the middle") and 5x latency/cost spikes. | Implemented Progressive Disclosure: working prompt context is constrained to ~500 tokens, loading pattern catalog entries only on-demand. | Prevents prompt bloat, maintains razor-sharp instruction adherence, and lowers per-turn inference costs. | Complex multi-subsystem refactorings spanning dozens of unlinked modules still require structured architectural chunking. |
| **6** | **Zero-Data-Egress Enterprise Security** | Enterprise CISOs and legal boundaries block uploading proprietary repositories to external cloud AI services. | Built local-sandbox execution where all compilation, static analysis, and sanitizers run inside the enterprise firewall. | All builds, static analysis, and ASAN runs execute 100% locally—eliminating bulk cloud RAG indexing and restricting model context to minimal ~500-token policies and scoped diffs (compatible with VPC/local models for zero egress). | Requires local developer machines or self-hosted CI runners with sufficient CPU/memory to execute native compilation. |

---

## 7. Engineering Boundaries & Cost/Latency Tradeoffs

| Mechanism | Production Sweet Spot | Explicit Tradeoff / Anti-Pattern |
|---|---|---|
| **Context Caching** | High-frequency shared policies, large enterprise pattern catalogs, multi-turn test debugging. | [Gemini Context Caching Cost Architecture](https://docs.cloud.google.com/gemini-enterprise-agent-platform/models/context-cache/context-cache-overview): Small or rarely queried skills lose money on storage TTL. Use progressive disclosure for rare skills; cache only high-volume common prefixes. |
| **Dual-Pass Audit** | Architecture changes, legacy refactoring, recurring CI / sanitizer failures. | Doubles input token consumption and model latency. Must be gated to architectural touchpoints, **never applied to routine CRUD or mechanical edits**. |
| **Local Script Hooks** | Repetitive build, test, lint, and sanitizer execution. | Requires properly configured host container toolchains (Clang/GCC, CMake, Ninja). |

---

## 8. Quickstart: 3-Minute Reproducible Verification

Includes a synthetic C++17 legacy fixture (`legacy_fixture/`) with raw observer pointers and a heap-use-after-free triggered during one-shot teardowns.

### Prerequisites
- CMake 3.16+
- Clang / GCC with AddressSanitizer support (`libasan`)
- POSIX Bash or Windows PowerShell

### Reproducing the Issue (The Legacy Baseline)

```bash
# Clone the repository
git clone https://github.com/sagarinamdar/enterprise-skill-architecture.git
cd enterprise-skill-architecture

# 1. Quick Cross-Platform Static Policy Check (Zero Dependencies)
python scripts/verify.py

# 2. Dynamic AddressSanitizer Run (Linux, macOS, WSL, or Docker with CMake & Clang)
./scripts/verify.sh
```

**Expected Dynamic Sanitizer Output:**
AddressSanitizer intercepts the `heap-use-after-free` caused by the one-shot listener deleting itself while `SessionRegistry::triggerEvent` audits its status:
```
=================================================================
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x603000000040
READ of size 8 at 0x603000000040 thread T0
    #0 0x4021a8 in SessionRegistry::triggerEvent(int, std::string) .../session_registry.cpp:42
    #1 0x403512 in main .../test_session_lifecycle.cpp:58
freed by thread T0 here:
    #0 0x7f... in operator delete(void*, unsigned long)
    #1 0x40284a in OneShotSessionListener::onSessionEvent .../test_session_lifecycle.cpp:24
=================================================================
```

### Applying the 3-File Skill Fix
When an agent is guided by `skills/skill.md`, it references `skills/pattern-catalog.md#pattern-1-scoped-connection-guard` to replace raw listener pointer maps with `ScopedConnection` RAII tokens.

Rerunning `./scripts/verify.sh` yields:
```
[100%] Built target session_legacy_test
Test project .../build
    Start 1: TestSessionLifecycle
1/1 Test #1: TestSessionLifecycle .............   Passed    0.04 sec

100% tests passed, 0 tests failed out of 1
AddressSanitizer: 0 errors detected.
```

---

## 9. Machine-Verified A/B Benchmark Results

Across 5 repeated test iterations comparing standard prompting against the 3-File Skill framework:

| Metric | Baseline Prompting (Repo Context Only) | 3-File Skill + `verify.sh` |
|---|---|---|
| **Clean Pass Rate (5 runs)** | 1 / 5 (20%) | **5 / 5 (100%)** |
| **AddressSanitizer Errors** | 4 (heap-use-after-free / leak) | **0** |
| **Command Hallucination Errors** | 3 (invalid compiler/linker flags) | **0 (Offloaded to script)** |
| **Introduced RAII Patterns** | No (duplicated manual `delete`) | **Yes (`ScopedConnection`)** |
| **Mean Turns to Green CI** | 4.2 turns | **1.4 turns** |

*Complete raw data and log traces are available in [`benchmarks/results.md`](benchmarks/results.md).*

---

## 10. References & Citations

1. **GitClear (2026)**: *The AI Code Quality & Maintainability Gap: Code Duplication, Churn, and the Decline of Refactoring*. [gitclear.com/research](https://www.gitclear.com/the_ai_code_quality_maintainability_gap)
2. **Thoughtworks (2026)**: *Context Rot and Context Poisoning in Autonomous Agent Workflows*. [thoughtworks.com/insights](https://www.thoughtworks.com/insights/blog/generative-ai/what-is-context-rot)
3. **Google Cloud (2026)**: *What are Agent Skills? Architecture and Script Bundling*. [cloud.google.com/discover/ai-agent-skills](https://cloud.google.com/discover/ai-agent-skills)
4. **Google Cloud Gemini Platform (2026)**: *Context Caching Architecture, KV Re-use, and Pricing*. [docs.cloud.google.com/gemini-enterprise-agent-platform](https://docs.cloud.google.com/gemini-enterprise-agent-platform/models/context-cache/context-cache-overview)
5. **Ashik et al. (2026)**: *When LLMs Lag Behind: Knowledge Conflicts from Evolving APIs in Code Generation*. [arXiv:2604.09515](https://arxiv.org/html/2604.09515v1)

---

## 11. Related Work & Ecosystem

- [**antigravity-voice-sentinel**](https://github.com/sagarinamdar/antigravity-voice-sentinel): Human-in-the-Loop latency elimination, speech-to-text prompting, and deterministic stall detection state machines for Google Antigravity.

---

## 12. License
MIT License. Free to use, adapt, and build upon for enterprise and academic research.
