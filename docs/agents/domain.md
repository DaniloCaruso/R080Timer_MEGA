# Domain Docs

How the engineering skills should consume this repo's domain documentation when exploring the codebase.

## Before exploring, read these

- **`GLOSSARY.md`** at the repo root: the project's single glossary.
- **`docs/<feature-name>/`**: read the folder of every feature that touches the area you're about to work in. Its `decisions.md` holds the decisions taken for that feature.

If any of these files don't exist, **proceed silently**. Don't flag their absence; don't suggest creating them upfront. The `/domain-modeling` skill (reached via `/grill-with-docs` and `/improve-codebase-architecture`) creates them lazily when terms or decisions actually get resolved.

## File structure

Single-context repo. This repo does **not** use ADRs: never create `docs/adr/`. Everything about a feature lives in one folder named after the feature, in kebab-case.

```
/
├── GLOSSARY.md
└── docs/
    ├── agents/                ← this configuration
    └── <feature-name>/
        └── decisions.md
```

## Recording a decision

Where a skill would write an ADR, append the decision to `docs/<feature-name>/decisions.md` instead (create the folder and file if needed). One `## <short title>` section per decision: context, decision, consequences. Pick the feature the decision belongs to; if it is unclear, ask.

## Use the glossary's vocabulary

When your output names a domain concept (in an issue title, a refactor proposal, a hypothesis, a test name), use the term as defined in `GLOSSARY.md`. Don't drift to synonyms the glossary explicitly avoids.

If the concept you need isn't in the glossary yet, that's a signal: either you're inventing language the project doesn't use (reconsider) or there's a real gap (note it for `/domain-modeling`).

## Flag decision conflicts

If your output contradicts a recorded decision, surface it explicitly rather than silently overriding:

> _Contradicts "<decision title>" in `docs/<feature-name>/decisions.md`, but worth reopening because…_
