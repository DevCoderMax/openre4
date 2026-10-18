#ifndef OPENRE4_UI_H
#define OPENRE4_UI_H

#ifdef __cplusplus
extern "C" {
#endif

enum OpenRE4Action {
    OPENRE4_ACTION_NONE = 0,
    OPENRE4_ACTION_START = 1,
    OPENRE4_ACTION_LOAD = 2,
    OPENRE4_ACTION_OPTIONS = 3,
    OPENRE4_ACTION_QUIT = 4
};

void openre4_ui_init(void);
int openre4_ui_update(float dt, int key_up, int key_down, int key_select);
void openre4_ui_on_click(int button_index);
void openre4_ui_on_hover(int button_index, int hovered);
int openre4_ui_get_selected_action(void);
void openre4_ui_reset_action(void);
int openre4_ui_get_cursor(void);
void openre4_ui_set_cursor(int cursor);
int openre4_ui_is_active(void);
void openre4_ui_set_active(int active);
void openre4_ui_get_button_color(int index, float* r, float* g, float* b, float* a);

// Sub-sistema nativo de câmera em Rust (Sanity Corrector + Freecam)
struct OpenRE4CameraInput {
    float dt;
    float move_forward;
    float move_right;
    float move_up;
    float mouse_dx;
    float mouse_dy;
    float fov_delta;
    int boost;
    int toggle_freecam;
    int teleport_preset;
};

struct OpenRE4CameraDebugInfo {
    int freecam_active;
    float pos_x;
    float pos_y;
    float pos_z;
    float target_x;
    float target_y;
    float target_z;
    float yaw_deg;
    float pitch_deg;
    float fovy;
    int sanity_triggered;
};

void openre4_camera_init(void);
void openre4_camera_update(
    float* pos_xyz,
    float* target_xyz,
    float* up_xyz,
    float* fovy,
    const struct OpenRE4CameraInput* input
);
int openre4_camera_is_freecam_active(void);
int openre4_camera_toggle_freecam(void);
void openre4_camera_set_sanity_enabled(int enabled);
void openre4_camera_get_debug_info(struct OpenRE4CameraDebugInfo* out_info);

// Sistema de Teletransporte / Stage Warp
void openre4_warp_to_room(unsigned short room);

#ifdef __cplusplus
}
#endif

#endif // OPENRE4_UI_H
