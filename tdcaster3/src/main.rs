extern crate olc_pixel_game_engine;

use crate::olc_pixel_game_engine as olc;

struct RaycastWorld {
    map: Vec<u8>,
    size: (usize, usize, usize),
}
struct RaycastResult {
    value: u8,
    x_int: f32,
    y_int: f32,
    tx: f32,
    ty: f32,
    tz: f32,
}
enum HitDirection {
    X, Y, Z,
}
impl RaycastWorld {
    fn new((width, height, length): (usize, usize, usize)) -> RaycastWorld {
        RaycastWorld {
            map: (0..(width * height * length)).map(|_| 0 ).collect(),
            size: (width, height, length),
        }
    }
}
fn initial_raycast_axis(d: f32, f: f32, id: f32) -> (isize, f32, u8) {
    if d > 0.0 {
        (
            1,
            (1.0 - f) * id,
            1
        )
    }
    else {
        (
            if d < 0.0 { -1 } else { 0 },
            f * id,
            0
        )
    }
}

fn raycast(world: &RaycastWorld, (x, y, z): (f32, f32, f32), (dx, dy, dz): (f32, f32, f32)) -> RaycastResult {
    let cx = x.floor() as u32;
    let cy = y.floor() as u32;
    let cz = z.floor() as u32;
    let xf = x - cx as f32;
    let yf = y - cy as f32;
    let zf = z - cz as f32;
    let idx = dx.abs().recip();
    let idy = dy.abs().recip();
    let idz = dz.abs().recip();
    let (xi, mut tnx, xs) = initial_raycast_axis(dx, xf, idx);
    let (yi, mut tny, ys) = initial_raycast_axis(dy, yf, idy);
    let (zi, mut tnz, zs) = initial_raycast_axis(dz, zf, idz);
    let max_passes = 16;
    let mut last_dir = HitDirection::X;
    let (mut xp, mut yp, mut zp) = (0, 0, 0);
    let mut world_index: isize = cx as isize + cy as isize * world.size.0 as isize + cz as isize * (world.size.0 * world.size.1) as isize;
    let mut passes = 0;
    let mut current_value: u8 = 0;
    loop {
        if passes > max_passes {
            break;
        }
        match world.map.get(world_index as usize) {
            Some(value) => {
                current_value = *value;
                if current_value != 0 {
                    // println!("found {}", current_value);
                    break;
                }
            },
            None => {
                current_value = 0;
                break;
            },
        }
        passes += 1;
        last_dir = if tny < tnx {
            if tny < tnz {
                HitDirection::Y
            }
            else {
                HitDirection::Z
            }
        }
        else {
            if tnx < tnz {
                HitDirection::X
            }
            else {
                HitDirection::Z
            }
        };
        match last_dir {
            HitDirection::Y => {
                tny += idy;
                yp += yi;
                world_index += yi * world.size.0 as isize;
                if cy as isize + yp < 0 || cy as isize + yp >= world.size.1 as isize {
                    break;
                }
            },
            HitDirection::X => {
                tnx += idx;
                xp += xi;
                world_index += xi;
                if cx as isize + xp < 0 || cx as isize + xp >= world.size.0 as isize {
                    break;
                }
            },
            HitDirection::Z => {
                // println!("vertical movement");
                tnz += idz;
                zp += zi;
                world_index += zi * (world.size.0 * world.size.1) as isize;
                if cz as isize + zp < 0 || cz as isize + zp >= world.size.2 as isize {
                    break;
                }
            },
        }
    }
    match &last_dir {
        HitDirection::X => xp -= xi,
        HitDirection::Y => yp -= yi,
        HitDirection::Z => zp -= zi,
    }
    let (mut tx, mut ty, mut tz): (f32, f32, f32) = (
        cx as f32 + xp as f32,
        cy as f32 + yp as f32,
        cz as f32 + zp as f32
    );
    let (x_int, y_int);
    match &last_dir {
        HitDirection::X => {
            let x_d = xp as f32 + xs as f32 - xf;
            x_int = (dy / dx) * x_d - yp as f32 + yf;
            y_int = (dz / dx) * x_d - zp as f32 + zf;
            tx += xs as f32;
            ty += x_int;
            tz += y_int;
        },
        HitDirection::Y => {
            let y_d = yp as f32 + ys as f32 - yf;
            x_int = (dx / dy) * y_d - xp as f32 + xf;
            y_int = (dz / dy) * y_d - zp as f32 + zf;
            tx += x_int;
            ty += ys as f32;
            tz += y_int;
        },
        HitDirection::Z => {
            let z_d = zp as f32 + zs as f32 - zf;
            x_int = (dx / dz) * z_d - xp as f32 + xf;
            y_int = (dy / dz) * z_d - yp as f32 + yf;
            tx += x_int;
            ty += y_int;
            tz += zs as f32;
        },
    }
    return RaycastResult {
        tx: tx, ty: ty, tz: tz,
        value: current_value,
        x_int: x_int,
        y_int: y_int,
    };
}
fn rotate_vector_x((x, y, z): (f32, f32, f32), c: f32, s: f32) -> (f32, f32, f32) {
    (x, (y * c) - (z * s), (y * s) + (z * c))
}
fn rotate_vector_y((x, y, z): (f32, f32, f32), c: f32, s: f32) -> (f32, f32, f32) {
    ((x * c) + (z * s), y, (z * c) - (x * s))
}
fn rotate_vector_z((x, y, z): (f32, f32, f32), c: f32, s: f32) -> (f32, f32, f32) {
    ((x * c) + (y * s), (x * s) - (y * c), z)
}
fn add((ax, ay, az): (f32, f32, f32), (bx, by, bz): (f32, f32, f32)) -> (f32, f32, f32) {
    (
        ax + bx,
        ay + by,
        az + bz
    )
}
fn sub((ax, ay, az): (f32, f32, f32), (bx, by, bz): (f32, f32, f32)) -> (f32, f32, f32) {
    (
        ax - bx,
        ay - by,
        az - bz
    )
}
fn div((ax, ay, az): (f32, f32, f32), b: f32) -> (f32, f32, f32) {
    (
        ax / b,
        ay / b,
        az / b
    )
}

struct Camera {
    x: f32, y: f32, z: f32,
    pitch: f32, direction: f32,
    fov_x: f32, fov_y: f32,
    res_x: usize, res_y: usize,
}
struct ExampleProgram {
    world: RaycastWorld,
    camera: Camera,
    wall_sprite: Option<olc::Sprite>,
    ceil_sprite: Option<olc::Sprite>,
    floor_sprite: Option<olc::Sprite>,
}
const TURN_SPEED: f32 = 2.0;
const MOVE_SPEED: f32 = 3.0;
impl olc::Application for ExampleProgram {
    fn on_user_create(&mut self) -> Result<(), olc::Error> {
        self.wall_sprite = Some(olc::Sprite::from_image("./resources/bricks.png").unwrap());
        self.ceil_sprite = Some(olc::Sprite::from_image("./resources/ceiling.png").unwrap());
        self.floor_sprite = Some(olc::Sprite::from_image("./resources/floor.png").unwrap());
        Ok(())
    }
    fn on_user_update(&mut self, elapsed_time: f32) -> Result<(), olc::Error> {
        olc::clear(olc::BLACK);
        self.draw_view();
        // self.draw_top_down();
        olc::draw_string(4, 4, &format!("xya: {}, {}, {}", self.camera.x, self.camera.y, self.camera.direction)[..], olc::BLUE);
        if olc::get_key(olc::Key::LEFT).held {
            self.camera.direction -= TURN_SPEED * elapsed_time;
        }
        if olc::get_key(olc::Key::RIGHT).held {
            self.camera.direction += TURN_SPEED * elapsed_time;
        }
        if olc::get_key(olc::Key::UP).held {
            self.camera.pitch += TURN_SPEED * elapsed_time;
        }
        if olc::get_key(olc::Key::DOWN).held {
            self.camera.pitch -= TURN_SPEED * elapsed_time;
        }
        if olc::get_key(olc::Key::W).held {
            self.camera.x += self.camera.direction.cos() * MOVE_SPEED * elapsed_time;
            self.camera.y += self.camera.direction.sin() * MOVE_SPEED * elapsed_time;
        }
        if olc::get_key(olc::Key::S).held {
            self.camera.x -= self.camera.direction.cos() * MOVE_SPEED * elapsed_time;
            self.camera.y -= self.camera.direction.sin() * MOVE_SPEED * elapsed_time;
        }
        if olc::get_key(olc::Key::A).held {
            self.camera.x += (self.camera.direction - (std::f64::consts::PI as f32 / 2.0)).cos() * MOVE_SPEED * elapsed_time;
            self.camera.y += (self.camera.direction - (std::f64::consts::PI as f32 / 2.0)).sin() * MOVE_SPEED * elapsed_time;
        }
        if olc::get_key(olc::Key::D).held {
            self.camera.x += (self.camera.direction + (std::f64::consts::PI as f32 / 2.0)).cos() * MOVE_SPEED * elapsed_time;
            self.camera.y += (self.camera.direction + (std::f64::consts::PI as f32 / 2.0)).sin() * MOVE_SPEED * elapsed_time;
        }
        if olc::get_key(olc::Key::CTRL).held {
            self.camera.z += MOVE_SPEED * elapsed_time;
        }
        if olc::get_key(olc::Key::SPACE).held {
            self.camera.z -= MOVE_SPEED * elapsed_time;
        }
        Ok(())
    }
    fn on_user_destroy(&mut self) -> Result<(), olc::Error> {
        Ok(())
    }
}
fn get_pixel_from_intercept(spr: &olc::Sprite, x_int: f32, y_int: f32) -> olc::Pixel {
    spr.get_pixel((x_int * spr.width() as f32) as i32, (y_int * spr.height() as f32) as i32)
}
impl ExampleProgram {

    fn draw_view(&mut self) {
        let (pc, ps, dc, ds) = (self.camera.pitch.cos(), self.camera.pitch.sin(), self.camera.direction.cos(), self.camera.direction.sin());
        let view_width_d2 = (self.camera.fov_x / 2.0).tan();
        let view_height_d2 = (self.camera.fov_y / 2.0).tan();
        let tl_vec: (f32, f32, f32) = rotate_vector_z(
            rotate_vector_y(
                (1.0, view_width_d2, -view_height_d2),
                pc, ps
            ),
            dc, ds
        );
        let bl_vec: (f32, f32, f32) = rotate_vector_z(
            rotate_vector_y(
                (1.0, view_width_d2, view_height_d2),
                pc, ps
            ),
            dc, ds
        );
        let tr_vec: (f32, f32, f32) = rotate_vector_z(
            rotate_vector_y(
                (1.0, -view_width_d2, -view_height_d2),
                pc, ps
            ),
            dc, ds
        );
        let l_diff = div(sub(bl_vec, tl_vec), self.camera.res_y as f32);
        let c_diff = div(sub(tr_vec, tl_vec), self.camera.res_x as f32);
        let mut l_vec = (tl_vec.0, tl_vec.1, tl_vec.2);
        for i in 0..self.camera.res_y {
            let mut r_vec = (l_vec.0, l_vec.1, l_vec.2);
            for j in 0..self.camera.res_x {
                let res = raycast(&self.world, (self.camera.x, self.camera.y, self.camera.z), r_vec);
                let draw_color = match &res.value {
                    1 => if let Some(spr) = &self.wall_sprite {
                        get_pixel_from_intercept(spr, res.x_int, res.y_int)
                    } else { olc::GREY },
                    2 => if let Some(spr) = &self.ceil_sprite {
                        get_pixel_from_intercept(spr, res.x_int, res.y_int)
                    } else { olc::RED },
                    3 => if let Some(spr) = &self.floor_sprite {
                        get_pixel_from_intercept(spr, res.x_int, res.y_int)
                    } else { olc::BLUE },
                    _ => olc::BLANK,
                };
                olc::draw(j as i32, i as i32, draw_color);
                r_vec = add(r_vec, c_diff);
            }
            l_vec = add(l_vec, l_diff);
        }
    }
    fn draw_top_down(&self) {
        let block_size = 16;
        let layer = 1;
        for i in 0..self.world.size.0 {
            for j in 0..self.world.size.1 {
                let value = self.world.map.get(i + j * self.world.size.0 + layer * self.world.size.0 * self.world.size.1).unwrap();
                if *value == 1 {
                    olc::draw_rect((i * block_size) as i32, (j * block_size) as i32, block_size as i32, block_size as i32, olc::WHITE);
                }
            }
        }
        let cam_radius = 4;
        let (vx, vy) = (
            (self.camera.x * block_size as f32) as i32,
            (self.camera.y * block_size as f32) as i32
        );
        olc::draw_rect(
            vx - cam_radius,
            vy - cam_radius,
            cam_radius * 2,
            cam_radius * 2,
            olc::BLUE
        );
        let vam = self.camera.pitch.cos();
        let col = raycast(
            &self.world,
            (self.camera.x, self.camera.y, self.camera.z),
            (
                self.camera.direction.cos() * vam,
                self.camera.direction.sin() * vam,
                self.camera.pitch.sin()
            )
        );
        let hit_col = match col.value {
            1 => olc::YELLOW,
            2 => olc::GREEN,
            3 => olc::CYAN,
            _ => olc::WHITE,
        };
        olc::draw_line(
            vx, vy,
            vx + (self.camera.direction.cos() * (block_size * 2) as f32) as i32,
            vy + (self.camera.direction.sin() * (block_size * 2) as f32) as i32,
            olc::RED
        );
        olc::draw_line(
            vx, vy,
            (col.tx * block_size as f32) as i32,
            (col.ty * block_size as f32) as i32,
            hit_col
        );
    }
}

fn main() {
    let (width, height, length) = (8, 8, 8);
    let (resx, resy) = (1024, 768);
    let mut example = ExampleProgram {
        world: RaycastWorld {
            map: (0..(width * height * length)).map(|i| -> u8 {
                let (x, y, z) = (
                    i % width,
                    (i / width) % height,
                    (i / (width * height)) % length
                );
                if x == 0 || x == width - 1 {
                    1
                }
                else if y == 0 || y == height - 1 {
                    1
                }
                else if z == 0 {
                    2
                }
                else if z == length - 1{
                    3
                }
                else {
                    0
                }
            }).collect(),
            size: (width, height, length),
        },
        camera: Camera {
            x: 4.0, y: 4.0, z: 4.0,
            direction: 0.0, pitch: 0.0,
            fov_x: (std::f64::consts::PI * (120.0 / 180.0) as f64) as f32,
            fov_y: (std::f64::consts::PI / 2.0 as f64) as f32,
            res_x: resx, res_y: resy,
        },
        wall_sprite: None,
        ceil_sprite: None,
        floor_sprite: None,
    };
    olc::start("Hello, world!", &mut example, resx as i32, resy as i32, 1, 1).unwrap();
}