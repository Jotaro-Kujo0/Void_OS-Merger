# UI scaffold

The master owns the primary user-facing control surface. This directory is a
planning boundary for UI-independent view models and future presentations.

TODO:

- Keep cluster state and scheduler logic independent from rendering.
- Define a machine-readable cluster view model.
- Support the first control surface selected for version one, likely CLI or a
  local terminal UI.
- Leave room for a graphical master UI later.
- Represent optional worker displays as surfaces controlled by master policy.
- Keep accessibility, keyboard operation, and structured output in scope.
