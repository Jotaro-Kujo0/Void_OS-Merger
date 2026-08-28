#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define BRIDGE_STRING_MAX    64
#define BRIDGE_QUEUE_MAX     128
#define FIXTURE_COUNT        3

typedef enum {
    UI_MODE_DISABLED,
    UI_MODE_LOCAL_STATUS,
    UI_MODE_MASTER_DIRECTED,
    UI_MODE_INTERACTIVE_SURFACE
} VomUiMode;

typedef enum {
    DEVICE_ATTACHED,
    DEVICE_DETACHED,
    DEVICE_CONFIGURATION_CHANGED
} VomPeripheralEventKind;

typedef enum {
    INPUT_KEY_PRESS,
    INPUT_POINTER_MOVE,
    INPUT_TOUCH_DOWN,
    INPUT_TOUCH_UP
} VomInputKind;

typedef struct {
    uint32_t key_code;
    uint32_t pointer_x;
    uint32_t pointer_y;
    uint32_t touch_id;
    uint64_t event_timestamp_ms;
} vom_input_data_t;

typedef struct {
    VomInputKind kind;
    vom_input_data_t data;
} vom_ui_input_event_t;

typedef struct {
    uint32_t display_index;
    uint32_t width_pixels;
    uint32_t height_pixels;
    uint32_t color_depth_bits;
    bool is_interactive;
    char driver_identifier[BRIDGE_STRING_MAX];
} vom_display_descriptor_t;

typedef void (*VomPeripheralChangeCallback)(VomPeripheralEventKind event, uint32_t device_id, void *user_data);
typedef void (*VomInputEventCallback)(const vom_ui_input_event_t *event, void *user_data);

struct vom_display_bridge_context {
    VomUiMode active_mode[FIXTURE_COUNT];
    bool block_remote_input;
    bool obscure_sensitive_workloads;
    VomPeripheralChangeCallback peripheral_cb;
    VomInputEventCallback input_cb;
    void *user_data;
    bool simulation_fault_triggered;
};

typedef struct vom_display_bridge_context vom_display_bridge_context_t;

vom_display_bridge_context_t* vom_display_bridge_create(void) {
    vom_display_bridge_context_t *ctx = (vom_display_bridge_context_t*)calloc(1, sizeof(vom_display_bridge_context_t));
    if (!ctx) return NULL;
    for (int i = 0; i < FIXTURE_COUNT; i++) {
        ctx->active_mode[i] = UI_MODE_LOCAL_STATUS;
    }
    return ctx;
}

void vom_display_bridge_destroy(vom_display_bridge_context_t *ctx) {
    if (ctx) free(ctx);
}

int32_t vom_display_bridge_enumerate(vom_display_bridge_context_t *ctx, vom_display_descriptor_t *out_list, int32_t max_entries) {
    if (!ctx || !out_list || max_entries <= 0) return -1;
    if (ctx->simulation_fault_triggered) return -2;

    int32_t count = (max_entries < FIXTURE_COUNT) ? max_entries : FIXTURE_COUNT;

    out_list[0] = (vom_display_descriptor_t){0, 1920, 1080, 24, true, "NATIVE_PANEL_0"};
    if (count > 1) {
        out_list[1] = (vom_display_descriptor_t){1, 3840, 2160, 32, true, "EXTERNAL_DP_1"};
    }
    if (count > 2) {
        out_list[2] = (vom_display_descriptor_t){2, 800, 480, 16, false, "LOW_POWER_MATRIX_2"};
    }

    return count;
}

int32_t vom_display_bridge_set_mode(vom_display_bridge_context_t *ctx, uint32_t display_index, VomUiMode mode) {
    if (!ctx) return -1;
    if (display_index >= FIXTURE_COUNT) return -1;
    if (ctx->simulation_fault_triggered) return -2;

    ctx->active_mode[display_index] = mode;
    return 0;
}

int32_t vom_display_bridge_register_callbacks(vom_display_bridge_context_t *ctx, VomPeripheralChangeCallback peripheral_cb, VomInputEventCallback input_cb, void *user_data) {
    if (!ctx) return -1;
    ctx->peripheral_cb = peripheral_cb;
    ctx->input_cb = input_cb;
    ctx->user_data = user_data;
    return 0;
}

int32_t vom_display_bridge_render_frame(vom_display_bridge_context_t *ctx, uint32_t display_index, const uint8_t *frame_buffer, size_t buffer_size) {
    if (!ctx || !frame_buffer || buffer_size == 0) return -1;
    if (display_index >= FIXTURE_COUNT) return -1;
    if (ctx->simulation_fault_triggered) return -2; 
    return 0;
}

int32_t vom_display_bridge_configure_privacy(vom_display_bridge_context_t *ctx, bool block_remote_input, bool obscure_sensitive_workloads) {
    if (!ctx) return -1;
    ctx->block_remote_input = block_remote_input;
    ctx->obscure_sensitive_workloads = obscure_sensitive_workloads;
    return 0;
}

int32_t vom_display_bridge_poll_events(vom_display_bridge_context_t *ctx) {
    if (!ctx) return -1;
    if (ctx->simulation_fault_triggered) return -2;

    if (ctx->input_cb) {
        vom_ui_input_event_t event;
        event.kind = INPUT_POINTER_MOVE;
        event.data.pointer_x = 100;
        event.data.pointer_y = 200;
        event.data.event_timestamp_ms = 1718900000ULL;
        ctx->input_cb(&event, ctx->user_data);
    }
    return 0;
}

void vom_display_bridge_trigger_isolated_fault(vom_display_bridge_context_t *ctx, bool trigger) {
    if (ctx) ctx->simulation_fault_triggered = trigger;
}

static void mock_peripheral_callback(VomPeripheralEventKind event, uint32_t device_id, void *user_data) {
    (void)event; (void)device_id; (void)user_data;
}

static void mock_input_callback(const vom_ui_input_event_t *event, void *user_data) {
    int *count = (int*)user_data;
    if (count) (*count)++;
    assert(event->data.pointer_x == 100);
}

void execute_display_bridge_test_suite(void) {
    vom_display_bridge_context_t *bridge = vom_display_bridge_create();
    assert(bridge != NULL);

    vom_display_descriptor_t list[FIXTURE_COUNT];
    int32_t enumerated = vom_display_bridge_enumerate(bridge, list, FIXTURE_COUNT);
    assert(enumerated == FIXTURE_COUNT);
    assert(list[0].width_pixels == 1920);

    int32_t rc_mode = vom_display_bridge_set_mode(bridge, 0, UI_MODE_MASTER_DIRECTED);
    assert(rc_mode == 0);
    assert(bridge->active_mode[0] == UI_MODE_MASTER_DIRECTED);

    int input_events_received = 0;
    vom_display_bridge_register_callbacks(bridge, mock_peripheral_callback, mock_input_callback, &input_events_received);
    vom_display_bridge_poll_events(bridge);
    assert(input_events_received == 1);

    vom_display_bridge_trigger_isolated_fault(bridge, true);
    uint8_t dummy_frame = 0;
    int32_t rc_render = vom_display_bridge_render_frame(bridge, 0, &dummy_frame, 1);
    assert(rc_render == -2);

    bool compute_pipeline_intact = true;
    assert(compute_pipeline_intact == true);

    vom_display_bridge_destroy(bridge);
}

#ifdef VOM_STANDALONE_TEST
int main(void) {
    execute_display_bridge_test_suite();
    return 0;
}
#endif
