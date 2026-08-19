#ifndef VOM_WORKER_DISPLAY_BRIDGE_H
#define VOM_WORKER_DISPLAY_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BRIDGE_STRING_MAX    64
#define BRIDGE_QUEUE_MAX     128

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

typedef struct vom_display_bridge_context vom_display_bridge_context_t;

typedef void (*VomPeripheralChangeCallback)(VomPeripheralEventKind event, uint32_t device_id, void *user_data);
typedef void (*VomInputEventCallback)(const vom_ui_input_event_t *event, void *user_data);

vom_display_bridge_context_t* vom_display_bridge_create(void);
void vom_display_bridge_destroy(vom_display_bridge_context_t *ctx);

int32_t vom_display_bridge_enumerate(vom_display_bridge_context_t *ctx, vom_display_descriptor_t *out_list, int32_t max_entries);
int32_t vom_display_bridge_set_mode(vom_display_bridge_context_t *ctx, uint32_t display_index, VomUiMode mode);

int32_t vom_display_bridge_register_callbacks(vom_display_bridge_context_t *ctx, VomPeripheralChangeCallback peripheral_cb, VomInputEventCallback input_cb, void *user_data);
int32_t vom_display_bridge_render_frame(vom_display_bridge_context_t *ctx, uint32_t display_index, const uint8_t *frame_buffer, size_t buffer_size);

int32_t vom_display_bridge_configure_privacy(vom_display_bridge_context_t *ctx, bool block_remote_input, bool obscure_sensitive_workloads);
int32_t vom_display_bridge_poll_events(vom_display_bridge_context_t *ctx);

#endif /* VOM_WORKER_DISPLAY_BRIDGE_H */
