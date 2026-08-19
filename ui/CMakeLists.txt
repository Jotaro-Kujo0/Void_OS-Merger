#ifndef VOM_UI_BOUNDARY_H
#define VOM_UI_BOUNDARY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define UI_OUTPUT_MAX_BYTES 65536
#define UI_TOKEN_MAX_LEN    128

typedef enum {
    UI_RENDER_CLI_RAW = 0,
    UI_RENDER_CLI_JSON = 1,
    UI_RENDER_TUI_NCURSES = 2,
    UI_RENDER_WEB_IPC = 3
} VomUiPresentationTech;

typedef enum {
    UI_INTENT_KIND_SUBMIT = 0,
    UI_INTENT_KIND_CANCEL = 1,
    UI_INTENT_KIND_DRAIN  = 2,
    UI_INTENT_KIND_RESUME = 3,
    UI_INTENT_KIND_CONFIG = 4
} VomUiIntentKind;

typedef struct {
    VomUiIntentKind kind;
    char target_id_token[UI_TOKEN_MAX_LEN];
    char structural_args_json[UI_TOKEN_MAX_LEN * 4];
    uint64_t generated_timestamp_ms;
} vom_ui_user_intent_envelope_t;

typedef struct {
    bool is_validated;
    uint32_t rejected_error_category;
    char error_explanation_token[UI_TOKEN_MAX_LEN];
} vom_ui_intent_validation_t;

typedef struct vom_ui_boundary_context vom_ui_boundary_context_t;

/* --- Presentation Engineering Function Hooks --- */

vom_ui_boundary_context_t* vom_ui_boundary_initialize(VomUiPresentationTech technique);
void vom_ui_boundary_shutdown(vom_ui_boundary_context_t *ctx);

/**
 * Consumes an unprivileged, frozen immutable view-model state snapshot record.
 * Serializes or builds the view structural layout maps based on target technology.
 * This guarantees zero direct queries reach down into internal cluster queues.
 */
int32_t vom_ui_boundary_generate_presentation(vom_ui_boundary_context_t *ctx, const void *immutable_view_model_ptr, uint8_t *out_render_buffer, size_t max_buffer_bytes);

/**
 * Validates, strips, and translates raw asynchronous presentation user actions.
 * Emits decoupled control payloads back up to the master cluster state scheduler coordinator.
 */
vom_ui_intent_validation_t vom_ui_boundary_process_user_intent(vom_ui_boundary_context_t *ctx, const vom_ui_user_intent_envelope_t *raw_intent, void *out_master_command_payload);

#endif /* VOM_UI_BOUNDARY_H */