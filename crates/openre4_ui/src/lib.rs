// OpenRE4 Native UI Subsystem (Rust)
//
// Manages the native title screen UI state machine, button interactions,
// color transitions, keyboard/mouse input, and direct dispatch to the game engine
// without relying on legacy GameCube 2D sprite emulation or virtual joypad macros.

use std::sync::Mutex;

pub mod camera;
pub use camera::*;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OpenRE4Action {
    None = 0,
    Start = 1,
    Load = 2,
    Options = 3,
    Quit = 4,
}

impl From<i32> for OpenRE4Action {
    fn from(val: i32) -> Self {
        match val {
            1 => OpenRE4Action::Start,
            2 => OpenRE4Action::Load,
            3 => OpenRE4Action::Options,
            4 => OpenRE4Action::Quit,
            _ => OpenRE4Action::None,
        }
    }
}

pub struct MenuState {
    pub cursor: i32,
    pub selected_action: OpenRE4Action,
    pub active: bool,
    pub hovered: [bool; 3],
    pub anim_time: f32,
}

impl Default for MenuState {
    fn default() -> Self {
        Self {
            cursor: 0,
            selected_action: OpenRE4Action::None,
            active: true,
            hovered: [false; 3],
            anim_time: 0.0,
        }
    }
}

impl MenuState {
    pub fn update(&mut self, dt: f32, key_up: bool, key_down: bool, key_select: bool) -> OpenRE4Action {
        if !self.active {
            return OpenRE4Action::None;
        }

        self.anim_time += dt;

        if key_up {
            self.cursor = if self.cursor <= 0 { 2 } else { self.cursor - 1 };
        } else if key_down {
            self.cursor = if self.cursor >= 2 { 0 } else { self.cursor + 1 };
        }

        if key_select {
            self.selected_action = match self.cursor {
                0 => OpenRE4Action::Start,
                1 => OpenRE4Action::Load,
                2 => OpenRE4Action::Options,
                _ => OpenRE4Action::None,
            };
        }

        self.selected_action
    }

    pub fn click(&mut self, button_index: i32) {
        self.cursor = button_index;
        self.selected_action = match button_index {
            0 => OpenRE4Action::Start,
            1 => OpenRE4Action::Load,
            2 => OpenRE4Action::Options,
            _ => OpenRE4Action::None,
        };
    }

    pub fn hover(&mut self, button_index: i32, hovered: bool) {
        if (0..3).contains(&button_index) {
            self.hovered[button_index as usize] = hovered;
            if hovered {
                self.cursor = button_index;
            }
        }
    }

    pub fn get_button_color(&self, index: i32) -> (f32, f32, f32, f32) {
        let is_selected = self.cursor == index;
        let is_hovered = if (0..3).contains(&index) {
            self.hovered[index as usize]
        } else {
            false
        };

        if is_selected || is_hovered {
            // Subtle pulse on selected/active button: RE4 Crimson
            let pulse = (self.anim_time * 4.0).sin() * 0.08;
            let r = (0.75 + pulse).clamp(0.0, 1.0);
            (r, 0.12, 0.12, 0.95)
        } else {
            // Unselected: Deep Slate/Steel Blue
            (0.14, 0.18, 0.25, 0.85)
        }
    }
}

static MENU_STATE: Mutex<MenuState> = Mutex::new(MenuState {
    cursor: 0,
    selected_action: OpenRE4Action::None,
    active: true,
    hovered: [false; 3],
    anim_time: 0.0,
});

#[no_mangle]
pub extern "C" fn openre4_ui_init() {
    if let Ok(mut state) = MENU_STATE.lock() {
        *state = MenuState::default();
    }
}

#[no_mangle]
pub extern "C" fn openre4_ui_update(
    dt: f32,
    key_up: bool,
    key_down: bool,
    key_select: bool,
) -> i32 {
    if let Ok(mut state) = MENU_STATE.lock() {
        state.update(dt, key_up, key_down, key_select) as i32
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn openre4_ui_on_click(button_index: i32) {
    if let Ok(mut state) = MENU_STATE.lock() {
        state.click(button_index);
    }
}

#[no_mangle]
pub extern "C" fn openre4_ui_on_hover(button_index: i32, hovered: bool) {
    if let Ok(mut state) = MENU_STATE.lock() {
        state.hover(button_index, hovered);
    }
}

#[no_mangle]
pub extern "C" fn openre4_ui_get_selected_action() -> i32 {
    if let Ok(state) = MENU_STATE.lock() {
        state.selected_action as i32
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn openre4_ui_reset_action() {
    if let Ok(mut state) = MENU_STATE.lock() {
        state.selected_action = OpenRE4Action::None;
    }
}

#[no_mangle]
pub extern "C" fn openre4_ui_get_cursor() -> i32 {
    if let Ok(state) = MENU_STATE.lock() {
        state.cursor
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn openre4_ui_set_cursor(cursor: i32) {
    if let Ok(mut state) = MENU_STATE.lock() {
        if (0..=2).contains(&cursor) {
            state.cursor = cursor;
        }
    }
}

#[no_mangle]
pub extern "C" fn openre4_ui_is_active() -> bool {
    if let Ok(state) = MENU_STATE.lock() {
        state.active
    } else {
        false
    }
}

#[no_mangle]
pub extern "C" fn openre4_ui_set_active(active: bool) {
    if let Ok(mut state) = MENU_STATE.lock() {
        state.active = active;
        if !active {
            state.selected_action = OpenRE4Action::None;
        }
    }
}

#[no_mangle]
pub extern "C" fn openre4_ui_get_button_color(
    index: i32,
    r: *mut f32,
    g: *mut f32,
    b: *mut f32,
    a: *mut f32,
) {
    if r.is_null() || g.is_null() || b.is_null() || a.is_null() {
        return;
    }

    if let Ok(state) = MENU_STATE.lock() {
        let (cr, cg, cb, ca) = state.get_button_color(index);
        unsafe {
            *r = cr;
            *g = cg;
            *b = cb;
            *a = ca;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_menu_initial_state() {
        let state = MenuState::default();
        assert_eq!(state.cursor, 0);
        assert_eq!(state.selected_action, OpenRE4Action::None);
        assert!(state.active);
    }

    #[test]
    fn test_keyboard_navigation() {
        let mut state = MenuState::default();
        // Down moves 0 -> 1 -> 2 -> 0
        state.update(0.016, false, true, false);
        assert_eq!(state.cursor, 1);

        state.update(0.016, false, true, false);
        assert_eq!(state.cursor, 2);

        state.update(0.016, false, true, false);
        assert_eq!(state.cursor, 0);

        // Up wraps 0 -> 2
        state.update(0.016, true, false, false);
        assert_eq!(state.cursor, 2);

        // Select confirms
        let action = state.update(0.016, false, false, true);
        assert_eq!(action, OpenRE4Action::Options);
    }

    #[test]
    fn test_click_and_reset_action() {
        let mut state = MenuState::default();
        state.click(0);
        assert_eq!(state.selected_action, OpenRE4Action::Start);

        state.selected_action = OpenRE4Action::None;
        assert_eq!(state.selected_action, OpenRE4Action::None);
    }

    #[test]
    fn test_hover_updates_cursor() {
        let mut state = MenuState::default();
        state.hover(2, true);
        assert_eq!(state.cursor, 2);
    }

    #[test]
    fn test_button_colors() {
        let state = MenuState::default();

        // Button 0 is selected by default -> Crimson (high red)
        let (r0, g0, _b0, _a0) = state.get_button_color(0);
        assert!(r0 > 0.6);
        assert!(g0 < 0.2);

        // Button 1 is unselected -> Slate Blue (low red, higher blue)
        let (r1, _g1, b1, _a1) = state.get_button_color(1);
        assert!(r1 < 0.3);
        assert!(b1 > 0.2);
    }
}
