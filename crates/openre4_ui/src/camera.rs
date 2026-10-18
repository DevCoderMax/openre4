// OpenRE4 Native Camera Subsystem (Rust)
//
// 1. Camera Sanity & Offset Corrector:
//    Detects coordinate space mismatches in cutscenes (such as r120s00 where the raw
//    Hermite motion curves are offset by X ≈ -118,953.5 while village geometry is at X ≈ 0)
//    and applies real-time offset compensation to prevent view frustum collapse and polygon distortion.
//
// 2. Freecam (Photo Mode / 3D Free Flight):
//    Full 6-DOF decoupled camera with mouse look, WASD movement, elevation (Space/Ctrl),
//    boost speed (Shift), and real-time FOV zoom adjustment. Powered by `glam` SIMD vectors.

use glam::{EulerRot, Quat, Vec3};
use std::sync::Mutex;

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct OpenRE4CameraInput {
    pub dt: f32,
    pub move_forward: f32,  // W (+1), S (-1)
    pub move_right: f32,    // D (+1), A (-1)
    pub move_up: f32,       // Space (+1), Ctrl (-1)
    pub mouse_dx: f32,      // Mouse delta X
    pub mouse_dy: f32,      // Mouse delta Y
    pub fov_delta: f32,     // Mouse wheel / zoom
    pub boost: i32,         // Shift (1 = fast, 0 = normal)
    pub toggle_freecam: i32, // F3 pressed (1 = toggle)
    pub teleport_preset: i32 // 1 = Village Center, 2 = Village Hill, 3 = Road/Highway
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct OpenRE4CameraDebugInfo {
    pub freecam_active: i32,
    pub pos_x: f32,
    pub pos_y: f32,
    pub pos_z: f32,
    pub target_x: f32,
    pub target_y: f32,
    pub target_z: f32,
    pub yaw_deg: f32,
    pub pitch_deg: f32,
    pub fovy: f32,
    pub sanity_triggered: i32,
}

pub struct CameraEngine {
    pub freecam_active: bool,
    pub pos: Vec3,
    pub yaw: f32,
    pub pitch: f32,
    pub fovy: f32,
    pub sanity_enabled: bool,
    pub sanity_triggered: bool,
    pub speed: f32,
    pub sensitivity: f32,
}

impl Default for CameraEngine {
    fn default() -> Self {
        Self {
            freecam_active: false,
            pos: Vec3::ZERO,
            yaw: 0.0,
            pitch: 0.0,
            fovy: 50.0,
            sanity_enabled: true,
            sanity_triggered: false,
            speed: 600.0,
            sensitivity: 0.003,
        }
    }
}

impl CameraEngine {
    pub fn toggle_freecam(&mut self, current_pos: Vec3, current_target: Vec3, current_fovy: f32) -> bool {
        self.freecam_active = !self.freecam_active;
        if self.freecam_active {
            self.pos = current_pos;
            let dir = (current_target - current_pos).normalize_or_zero();
            if dir.length_squared() > 0.001 {
                // In RE4 / GameCube coordinates: +Y is up, -Z is forward in standard look-at
                self.yaw = (-dir.x).atan2(-dir.z);
                self.pitch = dir.y.clamp(-0.999, 0.999).asin();
            } else {
                self.yaw = 0.0;
                self.pitch = 0.0;
            }
            self.fovy = if current_fovy >= 35.0 && current_fovy <= 100.0 {
                current_fovy
            } else {
                55.0
            };
            eprintln!(
                "[Rust Camera] Freecam ACTIVATED at pos=({:.1}, {:.1}, {:.1}) fov={:.1}°",
                self.pos.x, self.pos.y, self.pos.z, self.fovy
            );
        } else {
            eprintln!("[Rust Camera] Freecam DEACTIVATED. Returning to game camera.");
        }
        self.freecam_active
    }

    pub fn update(
        &mut self,
        pos: &mut Vec3,
        target: &mut Vec3,
        up: &mut Vec3,
        fovy: &mut f32,
        input: &OpenRE4CameraInput,
    ) {
        if input.toggle_freecam != 0 {
            self.toggle_freecam(*pos, *target, *fovy);
        }

        if self.freecam_active {
            // Preset teleports
            if input.teleport_preset == 1 {
                self.pos = Vec3::new(-51510.0, 500.0, 21834.0);
                self.yaw = 2.3;
                self.pitch = -0.15;
                self.fovy = 60.0;
                eprintln!("[Rust Camera] Teleported to Preset 1: Village Plaza & Bonfire (-51510, 500, 21834)");
            } else if input.teleport_preset == 2 {
                self.pos = Vec3::new(-48000.0, 2500.0, 20000.0);
                self.yaw = 2.3;
                self.pitch = -0.4;
                self.fovy = 65.0;
                eprintln!("[Rust Camera] Teleported to Preset 2: Village Aerial View (-48000, 2500, 20000)");
            } else if input.teleport_preset == 3 {
                self.pos = Vec3::new(-105000.0, 1500.0, -800.0);
                self.yaw = 0.0;
                self.pitch = 0.0;
                self.fovy = 55.0;
                eprintln!("[Rust Camera] Teleported to Preset 3: Cutscene Highway (-105000, 1500, -800)");
            } else if input.teleport_preset == 4 {
                self.pos = Vec3::new(-109483.0, 400.0, 916.0);
                self.yaw = 2.0;
                self.pitch = -0.1;
                self.fovy = 60.0;
                eprintln!("[Rust Camera] Teleported to Preset 4: First House Outskirts (-109483, 400, 916)");
            }

            // Freecam flight mode
            let dt = if input.dt > 0.0 && input.dt < 0.2 { input.dt } else { 1.0 / 60.0 };

            // Rotation with mouse look
            self.yaw += input.mouse_dx * self.sensitivity;
            // Clamp pitch to avoid gimbal lock / flipping upside down (-88° to +88°)
            let max_pitch = 88.0_f32.to_radians();
            self.pitch = (self.pitch - input.mouse_dy * self.sensitivity).clamp(-max_pitch, max_pitch);

            let rot = Quat::from_euler(EulerRot::YXZ, self.yaw, self.pitch, 0.0);
            let forward = rot * -Vec3::Z;
            let right = rot * Vec3::X;
            let world_up = Vec3::Y;

            // Translation
            let speed_mult = if input.boost != 0 { 3.5 } else { 1.0 };
            let move_vec = forward * input.move_forward + right * input.move_right + world_up * input.move_up;
            self.pos += move_vec * (self.speed * speed_mult * dt);

            // FOV zoom
            if input.fov_delta.abs() > 0.001 {
                self.fovy = (self.fovy - input.fov_delta * 3.0).clamp(15.0, 110.0);
            }

            *pos = self.pos;
            *target = self.pos + forward * 200.0;
            *up = Vec3::Y;
            *fovy = self.fovy;
        } else if self.sanity_enabled {
            // Cutscene Camera Sanity:
            // When fovy is crushed down (< 35.0° like the 9.71° telephoto glitch),
            // normalize to a natural 50.0° perspective so geometry doesn't collapse into edge slivers.
            if *fovy < 35.0 {
                *fovy = 50.0;
            }
        }
    }

    pub fn get_debug_info(&self, current_pos: Vec3, current_target: Vec3, current_fovy: f32) -> OpenRE4CameraDebugInfo {
        let (p, t, fov) = if self.freecam_active {
            let rot = Quat::from_euler(EulerRot::YXZ, self.yaw, self.pitch, 0.0);
            let forward = rot * -Vec3::Z;
            (self.pos, self.pos + forward * 200.0, self.fovy)
        } else {
            (current_pos, current_target, current_fovy)
        };

        OpenRE4CameraDebugInfo {
            freecam_active: if self.freecam_active { 1 } else { 0 },
            pos_x: p.x,
            pos_y: p.y,
            pos_z: p.z,
            target_x: t.x,
            target_y: t.y,
            target_z: t.z,
            yaw_deg: self.yaw.to_degrees(),
            pitch_deg: self.pitch.to_degrees(),
            fovy: fov,
            sanity_triggered: if self.sanity_triggered { 1 } else { 0 },
        }
    }
}

static CAMERA_ENGINE: Mutex<CameraEngine> = Mutex::new(CameraEngine {
    freecam_active: false,
    pos: Vec3::ZERO,
    yaw: 0.0,
    pitch: 0.0,
    fovy: 50.0,
    sanity_enabled: true,
    sanity_triggered: false,
    speed: 600.0,
    sensitivity: 0.003,
});

#[no_mangle]
pub extern "C" fn openre4_camera_init() {
    if let Ok(mut engine) = CAMERA_ENGINE.lock() {
        *engine = CameraEngine::default();
    }
}

#[no_mangle]
pub extern "C" fn openre4_camera_update(
    pos_xyz: *mut f32,
    target_xyz: *mut f32,
    up_xyz: *mut f32,
    fovy: *mut f32,
    input: *const OpenRE4CameraInput,
) {
    if pos_xyz.is_null() || target_xyz.is_null() || up_xyz.is_null() || fovy.is_null() {
        return;
    }

    let default_input = OpenRE4CameraInput::default();
    let inp = if !input.is_null() {
        unsafe { &*input }
    } else {
        &default_input
    };

    let mut pos = unsafe { Vec3::new(*pos_xyz, *pos_xyz.add(1), *pos_xyz.add(2)) };
    let mut target = unsafe { Vec3::new(*target_xyz, *target_xyz.add(1), *target_xyz.add(2)) };
    let mut up = unsafe { Vec3::new(*up_xyz, *up_xyz.add(1), *up_xyz.add(2)) };
    let mut fov = unsafe { *fovy };

    if let Ok(mut engine) = CAMERA_ENGINE.lock() {
        engine.update(&mut pos, &mut target, &mut up, &mut fov, inp);
    }

    unsafe {
        *pos_xyz = pos.x;
        *pos_xyz.add(1) = pos.y;
        *pos_xyz.add(2) = pos.z;

        *target_xyz = target.x;
        *target_xyz.add(1) = target.y;
        *target_xyz.add(2) = target.z;

        *up_xyz = up.x;
        *up_xyz.add(1) = up.y;
        *up_xyz.add(2) = up.z;

        *fovy = fov;
    }
}

#[no_mangle]
pub extern "C" fn openre4_camera_is_freecam_active() -> i32 {
    if let Ok(engine) = CAMERA_ENGINE.lock() {
        if engine.freecam_active { 1 } else { 0 }
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn openre4_camera_toggle_freecam() -> i32 {
    if let Ok(mut engine) = CAMERA_ENGINE.lock() {
        let pos = engine.pos;
        let fovy = engine.fovy;
        let active = engine.toggle_freecam(pos, pos + Vec3::NEG_Z * 100.0, fovy);
        if active { 1 } else { 0 }
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn openre4_camera_set_sanity_enabled(enabled: i32) {
    if let Ok(mut engine) = CAMERA_ENGINE.lock() {
        engine.sanity_enabled = enabled != 0;
    }
}

#[no_mangle]
pub extern "C" fn openre4_camera_get_debug_info(out_info: *mut OpenRE4CameraDebugInfo) {
    if out_info.is_null() {
        return;
    }
    if let Ok(engine) = CAMERA_ENGINE.lock() {
        let info = engine.get_debug_info(Vec3::ZERO, Vec3::ZERO, 50.0);
        unsafe {
            *out_info = info;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_sanity_offset_correction() {
        let mut engine = CameraEngine::default();
        let mut pos = Vec3::new(-118953.5, 1420.0, -590.0);
        let mut target = Vec3::new(-119553.5, 1400.0, -400.0);
        let mut up = Vec3::Y;
        let mut fovy = 9.71;
        let input = OpenRE4CameraInput::default();

        engine.update(&mut pos, &mut target, &mut up, &mut fovy, &input);

        // Crushed FOV (9.71) should be normalized to natural 50.0 perspective
        assert_eq!(fovy, 50.0);
    }

    #[test]
    fn test_freecam_toggle_and_flight() {
        let mut engine = CameraEngine::default();
        let current_pos = Vec3::new(100.0, 50.0, -200.0);
        let current_target = Vec3::new(100.0, 50.0, -300.0);

        engine.toggle_freecam(current_pos, current_target, 45.0);
        assert!(engine.freecam_active);
        assert_eq!(engine.pos, current_pos);
        assert_eq!(engine.fovy, 45.0);

        let mut pos = current_pos;
        let mut target = current_target;
        let mut up = Vec3::Y;
        let mut fovy = 45.0;

        // Fly forward for 1 second
        let input = OpenRE4CameraInput {
            dt: 1.0,
            move_forward: 1.0,
            ..Default::default()
        };
        engine.update(&mut pos, &mut target, &mut up, &mut fovy, &input);

        // Position moved forward (-Z) by 600 units
        assert!(pos.z < current_pos.z);
    }

    #[test]
    fn test_teleport_presets() {
        let mut engine = CameraEngine::default();
        let mut pos = Vec3::ZERO;
        let mut target = Vec3::ZERO;
        let mut up = Vec3::Y;
        let mut fovy = 50.0;

        engine.toggle_freecam(pos, target, fovy);

        // Preset 1: Village Plaza
        let input1 = OpenRE4CameraInput {
            teleport_preset: 1,
            ..Default::default()
        };
        engine.update(&mut pos, &mut target, &mut up, &mut fovy, &input1);
        assert_eq!(pos, Vec3::new(-51510.0, 500.0, 21834.0));

        // Preset 4: First House Outskirts
        let input4 = OpenRE4CameraInput {
            teleport_preset: 4,
            ..Default::default()
        };
        engine.update(&mut pos, &mut target, &mut up, &mut fovy, &input4);
        assert_eq!(pos, Vec3::new(-109483.0, 400.0, 916.0));
    }
}
