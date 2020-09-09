extern crate olc_pixel_game_engine;

use crate::olc_pixel_game_engine as olc;
use rayon::prelude::*;

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
struct Light {
    x: f32,
    y: f32,
    z: f32,
    r: f32,
    g: f32,
    b: f32,
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
fn initial_raycast_axis(d: f32, f: f32, id: f32) -> (f32, f32, f32, isize) {
    if d > 0.0 {
        (
            1.0,
            (1.0 - f) * id,
            1.0,
            1
        )
    }
    else {
        (
            if d < 0.0 { -1.0 } else { 0.0 },
            f * id,
            0.0,
            if d < 0.0 { -1 } else { 0 }
        )
    }
}

fn raycast(world: &RaycastWorld, (x, y, z): (f32, f32, f32), (dx, dy, dz): (f32, f32, f32)) -> RaycastResult {
    let cx: isize = x.floor() as isize;
    let cy: isize = y.floor() as isize;
    let cz: isize = z.floor() as isize;
    let xf: f32 = x - cx as f32;
    let yf: f32 = y - cy as f32;
    let zf: f32 = z - cz as f32;
    let idx: f32 = dx.abs().recip();
    let idy: f32 = dy.abs().recip();
    let idz: f32 = dz.abs().recip();
    let (xi, mut tnx, xs, xi_i): (f32, f32, f32, isize) = initial_raycast_axis(dx, xf, idx);
    let (yi, mut tny, ys, yi_i): (f32, f32, f32, isize) = initial_raycast_axis(dy, yf, idy);
    let (zi, mut tnz, zs, zi_i): (f32, f32, f32, isize) = initial_raycast_axis(dz, zf, idz);
    let max_passes: usize = 16;
    let mut last_dir: HitDirection = HitDirection::X;
    let (mut xp, mut yp, mut zp): (f32, f32, f32) = (0.0, 0.0, 0.0);
    let mut world_index: isize = cx + cy * world.size.0 as isize + cz * (world.size.0 * world.size.1) as isize;
    let mut passes: usize = 0;
    let mut current_value: u8 = 0;
    loop {
        if passes > max_passes {
            break;
        }
        match world.map.get(world_index as usize) {
            Some(value) => {
                current_value = *value;
                if current_value != 0 {
                    break;
                }
            },
            None => {
                current_value = 0;
                break;
            },
        }
        // current_value = world.map[world_index as usize];
        // if current_value != 0 {
        //     break;
        // }
        passes += 1;
        last_dir = match (tny < tnx, tny < tnz, tnx < tnz) {
            (true, true, _) => {
                tny += idy;
                yp += yi;
                world_index += yi_i * world.size.0 as isize;
                HitDirection::Y
            },
            (false, _, true) => {
                tnx += idx;
                xp += xi;
                world_index += xi_i;
                HitDirection::X
            },
            _ => {
                tnz += idz;
                zp += zi;
                world_index += zi_i * (world.size.0 * world.size.1) as isize;
                HitDirection::Z
            }
        };
        // last_dir = if tny < tnx {
        //     if tny < tnz {
        //         HitDirection::Y
        //     }
        //     else {
        //         HitDirection::Z
        //     }
        // }
        // else {
        //     if tnx < tnz {
        //         HitDirection::X
        //     }
        //     else {
        //         HitDirection::Z
        //     }
        // };
        // match last_dir {
        //     HitDirection::Y => {
        //         tny += idy;
        //         yp += yi;
        //         world_index += yi_i * world.size.0 as isize;
        //         // if cy + yp < 0 || cy + yp >= world.size.1 as isize {
        //         //     break;
        //         // }
        //     },
        //     HitDirection::X => {
        //         tnx += idx;
        //         xp += xi;
        //         world_index += xi_i;
        //         // if cx as isize + xp < 0 || cx as isize + xp >= world.size.0 as isize {
        //         //     break;
        //         // }
        //     },
        //     HitDirection::Z => {
        //         tnz += idz;
        //         zp += zi;
        //         world_index += zi_i * (world.size.0 * world.size.1) as isize;
        //         // if cz as isize + zp < 0 || cz as isize + zp >= world.size.2 as isize {
        //         //     break;
        //         // }
        //     },
        // }
    }
    match &last_dir {
        HitDirection::X => xp -= xi,
        HitDirection::Y => yp -= yi,
        HitDirection::Z => zp -= zi,
    }
    let (mut tx, mut ty, mut tz): (f32, f32, f32) = (
        cx as f32 + xp,
        cy as f32 + yp,
        cz as f32 + zp
    );
    let (x_int, y_int);
    match &last_dir {
        HitDirection::X => {
            let x_d = xp + xs - xf;
            x_int = (dy / dx) * x_d - yp + yf;
            y_int = (dz / dx) * x_d - zp + zf;
            tx += xs;
            ty += x_int;
            tz += y_int;
        },
        HitDirection::Y => {
            let y_d = yp + ys - yf;
            x_int = (dx / dy) * y_d - xp + xf;
            y_int = (dz / dy) * y_d - zp + zf;
            tx += x_int;
            ty += ys;
            tz += y_int;
        },
        HitDirection::Z => {
            let z_d = zp + zs - zf;
            x_int = (dx / dz) * z_d - xp + xf;
            y_int = (dy / dz) * z_d - yp + yf;
            tx += x_int;
            ty += y_int;
            tz += zs;
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
fn mult((ax, ay, az): (f32, f32, f32), b: f32) -> (f32, f32, f32) {
    (
        ax * b,
        ay * b,
        az * b
    )
}
struct Sprite {
    pixels: Vec<olc::Pixel>,
    width: usize,
    height: usize,
}
impl Sprite {
    fn from_olc_sprite(spr: &olc::Sprite) -> Sprite {
        let mut pixels: Vec<olc::Pixel> = vec!();
        for j in 0..spr.height() {
            for i in 0..spr.width() {
                pixels.push(spr.get_pixel(i as i32, j as i32));
            }
        }
        Sprite {
            pixels: pixels,
            width: spr.width() as usize,
            height: spr.height() as usize,
        }
    }
    fn get_pixel(&self, x: usize, y: usize) -> &olc::Pixel {
        self.pixels.get(x + y * self.width).unwrap_or(&olc::BLANK)
    }
    fn get_pixel_from_intercept(&self, x_int: f32, y_int: f32) -> &olc::Pixel{
        self.get_pixel((x_int * self.width as f32) as usize, (y_int * self.height as f32) as usize)
    }
}

struct Camera {
    x: f32, y: f32, z: f32,
    pitch: f32, direction: f32,
    fov_x: f32, fov_y: f32,
    res_x: usize, res_y: usize,
}
struct Assets {
    wall_sprite: Sprite,
    ceil_sprite: Sprite,
    floor_sprite: Sprite,
}
struct ExampleProgram {
    world: RaycastWorld,
    camera: Camera,
    assets: Assets,
    lights: Vec<Light>,
}
impl Assets {
    fn new() -> Assets {
        Assets {
            wall_sprite: Sprite::from_olc_sprite(&olc::Sprite::from_image("./resources/bricks.png").unwrap()),
            ceil_sprite: Sprite::from_olc_sprite(&olc::Sprite::from_image("./resources/ceiling.png").unwrap()),
            floor_sprite: Sprite::from_olc_sprite(&olc::Sprite::from_image("./resources/floor.png").unwrap()),
        }
    }
}
const TURN_SPEED: f32 = 2.0;
const MOVE_SPEED: f32 = 3.0;
impl olc::Application for ExampleProgram {
    fn on_user_create(&mut self) -> Result<(), olc::Error> {
        self.lights.push(Light {
            x: 2.0, y: 2.0, z: 2.0,
            r: 1.0, g: 1.0, b: 1.0,
        });
        Ok(())
    }
    fn on_user_update(&mut self, elapsed_time: f32) -> Result<(), olc::Error> {
        // olc::clear(olc::BLACK);
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
fn light_effect(col: olc::Pixel, lights: &Vec<Light>, tx: f32, ty: f32, tz: f32) -> olc::Pixel {
    let mut total_r: f32 = 0.0;
    let mut total_g: f32 = 0.0;
    let mut total_b: f32 = 0.0;
    for light in lights.iter() {
        let (ndx, ndy, ndz) = (tx - light.x, ty - light.y, tz - light.z);
        let magnitude_sqr = ndx * ndx + ndy * ndy + ndz * ndz;
        total_r += light.r / magnitude_sqr;
        total_g += light.g / magnitude_sqr;
        total_b += light.b / magnitude_sqr;
    }
    olc::Pixel {
        r: ((255.0 as f32).min(col.r as f32 * total_r)) as u8,
        g: ((255.0 as f32).min(col.g as f32 * total_g)) as u8,
        b: ((255.0 as f32).min(col.b as f32 * total_b)) as u8,
        a: col.a,
    }
}
impl ExampleProgram {

    fn draw_view(&self) {
        let (pc, ps, dc, ds) = (self.camera.pitch.cos(), self.camera.pitch.sin(), self.camera.direction.cos(), self.camera.direction.sin());
        let view_width_d2 = (self.camera.fov_x / 2.0).tan(); // todo: precalculable
        let view_height_d2 = (self.camera.fov_y / 2.0).tan();
        let tl_vec: (f32, f32, f32) = rotate_vector_z( // todo: should test if functional style is slower or faster than procedural
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
        let nums_to_iter: Vec<usize> = (0..4).collect();
        let width_per_column: usize = self.camera.res_x / nums_to_iter.len();
        nums_to_iter.par_iter().for_each(|&i| {
            let local_tl_vec = add(tl_vec, mult(c_diff, (i * width_per_column) as f32));
            self.draw_view_part(local_tl_vec, l_diff, c_diff, i * width_per_column, 0, i * width_per_column + width_per_column, self.camera.res_y);
        });
    }
    fn draw_view_part(&self, tl_vec: (f32, f32, f32), l_diff: (f32, f32, f32), c_diff: (f32, f32, f32), x_begin: usize, y_begin: usize, x_end: usize, y_end: usize) {
        let mut l_vec = (tl_vec.0, tl_vec.1, tl_vec.2);
        for i in y_begin..y_end {
            let mut r_vec = (l_vec.0, l_vec.1, l_vec.2);
            for j in x_begin..x_end {
                let res = raycast(&self.world, (self.camera.x, self.camera.y, self.camera.z), r_vec);
                let mut draw_color = *match &res.value {
                    1 => self.assets.wall_sprite.get_pixel_from_intercept(res.x_int, res.y_int),
                    2 => self.assets.ceil_sprite.get_pixel_from_intercept(res.x_int, res.y_int),
                    3 => self.assets.floor_sprite.get_pixel_from_intercept(res.x_int, res.y_int),
                    _ => &olc::BLANK,
                };
                draw_color = light_effect(draw_color, &self.lights, res.tx, res.ty, res.tz);
                olc::draw(j as i32, i as i32, draw_color);
                r_vec = add(r_vec, c_diff); // todo: maybe change to mutating?
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
        let vx = (self.camera.x * block_size as f32) as i32;
        let vy = (self.camera.y * block_size as f32) as i32;
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
    let (width, height, length) = (8, 8, 5);
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
            x: (width as f32 / 2.0), y: (height as f32 / 2.0), z: (length as f32 / 2.0),
            direction: 0.0, pitch: 0.0,
            fov_x: (std::f64::consts::PI * (120.0 / 180.0) as f64) as f32,
            fov_y: (std::f64::consts::PI / 2.0 as f64) as f32,
            res_x: resx, res_y: resy,
        },
        assets: Assets::new(),
        lights: vec!(),
    };
    olc::start("Hello, world!", &mut example, resx as i32, resy as i32, 1, 1).unwrap();
}