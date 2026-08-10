/*
 * worker/display_bridge.h — optional worker UI surface planning notes.
 *
 * A worker display is optional. It may show local status, act as a secondary
 * UI, render master-directed output, or contribute input events.
 *
 * TODO:
 *
 * - Enumerate displays and input devices.
 * - Report attach/detach events.
 * - Accept a master-selected UI mode.
 * - Enforce local permission and privacy policy.
 * - Bound input/event queues.
 * - Handle display loss without killing unrelated compute chunks.
 * - Keep rendering/input transport separate from chunk execution transport
 *   unless a later design deliberately combines them.
 */
