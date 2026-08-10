# UI model

The master owns the primary user-facing control surface. The cluster should
look like one logical device even though the UI may show the contributing
workers as diagnostic details.

## Master UI

TODO:

- Show logical-device identity and overall health.
- Show aggregate CPU, memory, storage, and accelerator capacity.
- Show worker health and contribution.
- Show workloads and chunk progress.
- Show scheduling explanations without exposing unstable internals.
- Show degraded state when workers are missing.
- Allow workload submission, cancellation, and inspection.
- Allow worker drain, resume, approval, and removal.
- Provide a machine-readable view model for future graphical UIs.

The CLI may be the first UI, but it should consume a master-owned view model
rather than directly manipulating scheduler internals.

## Worker UI

A worker may have a screen, keyboard, touch surface, pointer, audio device, or
none of these. A display is an advertised capability and an optional surface,
not a second master by default.

Possible policy modes:

- `disabled` — worker does not present cluster UI;
- `statusOnly` — worker shows local health and assignment status;
- `secondaryUi` — master permits a secondary control surface;
- `renderTarget` — master sends visual output to the worker display;
- `inputSurface` — worker contributes input events.

TODO:

- Define display attach/detach events.
- Define master permission for each mode.
- Define input ownership and routing.
- Define behavior when a display disappears during a workload.
- Keep computation independent from optional display availability.

```c
/*
 * Comment-only view-model outline:
 *
 * struct vom_cluster_ui_snapshot {
 *     struct vom_logical_device_summary logical_device;
 *     struct vom_worker_summary workers[64];
 *     struct vom_workload_summary workloads[256];
 *     struct vom_resource_summary aggregate_resources;
 * };
 */
```
