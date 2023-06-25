const std = @import("std");
const math = std.math;
const Allocator = std.mem.Allocator;

const img = @import("img");
const Image = img.Image;

const pge = @import("pge");
const VF2D = pge.VF2D;
const VU2D = pge.VU2D;

const VF3D = pge.VF3D;
const VU3D = pge.VU3D;
const VI3D = pge.VI3D;

pub fn rotVecX(v: VF3D, c: f32, s: f32) VF3D {
    return .{
        .x = v.x,
        .y = (v.y * c) - (v.z * s),
        .z = (v.y * s) + (v.z * c),
    };
}
pub fn rotVecY(v: VF3D, c: f32, s: f32) VF3D {
    return .{
        .x = (v.x * c) + (v.z * s),
        .y = v.y,
        .z = (v.z * c) - (v.x * s),
    };
}
pub fn rotVecZ(v: VF3D, c: f32, s: f32) VF3D {
    return .{
        .x = (v.x * c) + (v.y * s),
        .y = (v.x * s) - (v.y * c),
        .z = v.z,
    };
}

pub const Sprite = struct {
    pixels: []pge.Pixel,
    size: VU2D,

    pub fn atF(self: *const Sprite, intercept: VF2D) pge.Pixel {
        return self.pixels[
            @intFromFloat(u32, @min(1.0 - math.floatEps(f32), intercept.x) * @floatFromInt(f32, self.size.x)) +
                (@intFromFloat(u32, @min(1.0 - math.floatEps(f32), intercept.y) * @floatFromInt(f32, self.size.y)) * self.size.x)
        ];
    }
    pub fn fromImage(alloc: Allocator, image: Image) !Sprite {
        var pixels = try alloc.alloc(pge.Pixel, image.width * image.height);
        var iter = image.iterator();
        var i: usize = 0;
        while (iter.next()) |p| {
            pixels[i] = pge.Pixel{ .n = p.toU32Rgba() };
            i += 1;
        }
        return .{
            .pixels = pixels,
            .size = .{
                .x = @intCast(u32, image.width),
                .y = @intCast(u32, image.height),
            },
        };
    }

    pub fn deinit(self: *Sprite, alloc: Allocator) void {
        alloc.free(self.pixels);
        self.* = undefined;
    }
};

pub const Camera = struct {
    pos: VF3D,
    dir: VF2D,
    fov: VF2D,

    fn depthEffect(
        cam: *const Camera,
        rr: *const World.RaycastResult,
        from: pge.Pixel,
    ) pge.Pixel {
        const dist_x = rr.t.x - cam.pos.x;
        const dist_y = rr.t.y - cam.pos.y;
        const dist_z = rr.t.z - cam.pos.z;
        const dist_sqr = (dist_x * dist_x) + (dist_y * dist_y) + (dist_z * dist_z);
        if (dist_sqr == 0) return from;
        const a = @min(4.0 / dist_sqr, 1.0);
        return .{ .c = .{
            .r = @intFromFloat(u8, a * @floatFromInt(f32, from.c.r)),
            .g = @intFromFloat(u8, a * @floatFromInt(f32, from.c.g)),
            .b = @intFromFloat(u8, a * @floatFromInt(f32, from.c.b)),
            .a = from.c.a,
        } };
    }

    fn lightEffect(
        world: *const World,
        rr: *const World.RaycastResult,
        from: pge.Pixel,
    ) pge.Pixel {
        var total_r: f32 = 0.0;
        var total_g: f32 = 0.0;
        var total_b: f32 = 0.0;

        for (world.lights.items) |light| {
            var result: World.RaycastResult = undefined;
            world.raycast(light.pos, .{
                .x = rr.t.x - light.pos.x,
                .y = rr.t.y - light.pos.y,
                .z = rr.t.z - light.pos.z,
            }, &result);

            const max_diff = 0.05;
            if (math.fabs(result.t.x - rr.t.x) < max_diff and
                math.fabs(result.t.y - rr.t.y) < max_diff and
                math.fabs(result.t.z - rr.t.z) < max_diff)
            {
                const dist_x = rr.t.x - light.pos.x;
                const dist_y = rr.t.y - light.pos.y;
                const dist_z = rr.t.z - light.pos.z;
                const mag_sqr = (dist_x * dist_x) + (dist_y * dist_y) + (dist_z * dist_z);
                total_r += light.brightness *
                    @floatFromInt(f32, light.color.c.r) / mag_sqr;
                total_g += light.brightness *
                    @floatFromInt(f32, light.color.c.g) / mag_sqr;
                total_b += light.brightness *
                    @floatFromInt(f32, light.color.c.b) / mag_sqr;
            }
        }
        return .{ .c = .{
            .r = @intFromFloat(u8, @min(255.0, @floatFromInt(f32, from.c.r) * total_r)),
            .g = @intFromFloat(u8, @min(255.0, @floatFromInt(f32, from.c.g) * total_g)),
            .b = @intFromFloat(u8, @min(255.0, @floatFromInt(f32, from.c.b) * total_b)),
            .a = from.c.a,
        } };
    }

    pub fn drawView(
        cam: *const Camera,
        world: *const World,
        game: *const Game,
        res: VU2D,
        engine: *pge.EngineState,
    ) void {
        const yc = math.cos(cam.dir.y);
        const ys = math.sin(cam.dir.y);
        const xc = math.cos(cam.dir.x);
        const xs = math.sin(cam.dir.x);
        const sized2 = VF2D{
            .x = math.tan(cam.fov.x / 2.0),
            .y = math.tan(cam.fov.y / 2.0),
        };
        const tl = rotVecZ(rotVecY(.{ .x = 1.0, .y = sized2.x, .z = -sized2.y }, yc, ys), xc, xs);
        const bl = rotVecZ(rotVecY(.{ .x = 1.0, .y = sized2.x, .z = sized2.y }, yc, ys), xc, xs);
        const tr = rotVecZ(rotVecY(.{ .x = 1.0, .y = -sized2.x, .z = -sized2.y }, yc, ys), xc, xs);

        const l_diff = VF3D{
            .x = (bl.x - tl.x) / @floatFromInt(f32, res.y),
            .y = (bl.y - tl.y) / @floatFromInt(f32, res.y),
            .z = (bl.z - tl.z) / @floatFromInt(f32, res.y),
        };
        const c_diff = VF3D{
            .x = (tr.x - tl.x) / @floatFromInt(f32, res.x),
            .y = (tr.y - tl.y) / @floatFromInt(f32, res.x),
            .z = (tr.z - tl.z) / @floatFromInt(f32, res.x),
        };

        var draw_target = engine.drawTarget();
        var p_i: usize = 0;
        var l = tl;
        for (0..res.y) |_| {
            var r = l;
            for (0..res.x) |_| {
                var result: World.RaycastResult = undefined;
                world.raycast(cam.pos, r, &result);
                const draw_color = if (result.value) |v| switch (v) {
                    1 => game.brick_image.atF(result.intercept),
                    2 => pge.Pixel.Yellow,
                    3 => pge.Pixel.Blue,
                    else => pge.Pixel.Blank,
                } else pge.Pixel.Blank;
                // directly write to pixel array :D
                //draw_target.data[p_i] = depthEffect(cam, &result, draw_color);
                draw_target.data[p_i] = lightEffect(world, &result, draw_color);
                p_i += 1;

                r.x += c_diff.x;
                r.y += c_diff.y;
                r.z += c_diff.z;
            }
            l.x += l_diff.x;
            l.y += l_diff.y;
            l.z += l_diff.z;
        }
    }
};
pub const Light = struct {
    pos: VF3D,
    color: pge.Pixel,
    brightness: f32,
};
pub const WorldItem = u8;
pub const World = struct {
    map: []?WorldItem,
    size: VU3D,
    lights: std.ArrayListUnmanaged(Light) = .{},

    const Self = @This();
    pub fn init(alloc: Allocator, size: VU3D) !Self {
        var map = try alloc.alloc(?WorldItem, size.x * size.y * size.z);
        for (map) |*v| v.* = null;
        for (0..size.x) |x| for (0..size.y) |y| for (0..size.z) |z| {
            if (x == 0 or
                y == 0 or
                z == 0 or
                x == size.x - 1 or
                y == size.y - 1 or
                z == size.z - 1)
                map[x + (y * size.x) + (z * size.x * size.y)] = 1;
        };
        return Self{
            .map = map,
            .size = size,
        };
    }
    pub fn deinit(self: *Self, alloc: Allocator) void {
        self.lights.deinit(alloc);
        alloc.free(self.map);
        self.* = undefined;
    }

    pub const RaycastResult = struct {
        value: ?WorldItem,
        intercept: VF2D,
        t: VF3D,
    };
    pub fn raycast(
        self: *const World,
        start: VF3D,
        dir: VF3D,
        result: *RaycastResult,
    ) void {
        // starting cell
        const c = VI3D{
            .x = @intFromFloat(i32, math.floor(start.x)),
            .y = @intFromFloat(i32, math.floor(start.y)),
            .z = @intFromFloat(i32, math.floor(start.z)),
        };
        // fraction of starting cell
        const f = VF3D{
            .x = start.x - @floatFromInt(f32, c.x),
            .y = start.y - @floatFromInt(f32, c.y),
            .z = start.z - @floatFromInt(f32, c.z),
        };

        // iterator?
        var id = VF3D{
            .x = 1.0 / math.fabs(dir.x),
            .y = 1.0 / math.fabs(dir.y),
            .z = 1.0 / math.fabs(dir.z),
        };
        // dont remember exactly what these values are for
        // lol why did i write it this way. nobody, not even i, completely know why
        //     the things here are certain ways. needs to be figured out, then maybe
        //     there would be some not-so-trash variable naming and such
        var s: pge.V3(extern struct { i: f32, tn: f32, s: f32, i_i: f32 }) = undefined;
        inline for (.{ "x", "y", "z" }) |axis| {
            @field(s, axis) =
                if (@field(dir, axis) > 0.0)
                .{
                    .i = 1.0,
                    .tn = (1.0 - @field(f, axis)) *
                        @field(id, axis),
                    .s = 1.0,
                    .i_i = 1,
                }
            else
                .{
                    .i = if (@field(dir, axis) < 0.0) -1.0 else 0.0,
                    .tn = @field(f, axis) *
                        @field(id, axis),
                    .s = 0.0,
                    .i_i = if (@field(dir, axis) < 0.0) -1 else 0,
                };
        }

        var current_value: ?WorldItem = null;
        var last_dir: enum { x, y, z } = .x;
        var p = VF3D.Zero;
        var passes: usize = 0;
        const max_passes = 64;
        var world_index: i32 = c.x + (c.y * @intCast(i32, self.size.x)) +
            (c.z * @intCast(i32, self.size.x * self.size.y));
        while (passes < max_passes) : (passes += 1) {
            if (s.y.tn < s.x.tn and s.y.tn < s.z.tn) {
                s.y.tn += id.y;
                p.y += s.y.i;
                world_index += @intFromFloat(
                    i32,
                    s.y.i_i * @floatFromInt(f32, self.size.x),
                );
                last_dir = .y;
            } else if (s.y.tn >= s.x.tn and s.x.tn < s.z.tn) {
                s.x.tn += id.x;
                p.x += s.x.i;
                world_index += @intFromFloat(i32, s.x.i_i);
                last_dir = .x;
            } else {
                s.z.tn += id.z;
                p.z += s.z.i;
                world_index += @intFromFloat(
                    i32,
                    s.z.i_i * @floatFromInt(
                        f32,
                        self.size.x * self.size.y,
                    ),
                );
                last_dir = .z;
            }
            if (world_index < 0 or world_index >= self.map.len) break;
            current_value = self.map[@intCast(u32, world_index)];
            if (current_value != null) break;
        }

        // intercept calculation
        switch (last_dir) {
            .x => p.x -= s.x.i,
            .y => p.y -= s.y.i,
            .z => p.z -= s.z.i,
        }
        const t = VF3D{
            .x = @floatFromInt(f32, c.x) + p.x,
            .y = @floatFromInt(f32, c.y) + p.y,
            .z = @floatFromInt(f32, c.z) + p.z,
        };
        switch (last_dir) {
            .x => {
                const d = p.x + s.x.s - f.x;
                const intercept = VF2D{
                    .x = (dir.y / dir.x) * d - p.y + f.y,
                    .y = (dir.z / dir.x) * d - p.z + f.z,
                };
                result.* = .{
                    .value = current_value,
                    .intercept = intercept,
                    .t = .{
                        .x = t.x + s.x.s,
                        .y = t.y + intercept.x,
                        .z = t.z + intercept.y,
                    },
                };
            },
            .y => {
                const d = p.y + s.y.s - f.y;
                const intercept = VF2D{
                    .x = (dir.x / dir.y) * d - p.x + f.x,
                    .y = (dir.z / dir.y) * d - p.z + f.z,
                };
                result.* = .{
                    .value = current_value,
                    .intercept = intercept,
                    .t = .{
                        .x = t.x + intercept.x,
                        .y = t.y + s.y.s,
                        .z = t.z + intercept.y,
                    },
                };
            },
            .z => {
                const d = p.z + s.z.s - f.z;
                const intercept = VF2D{
                    .x = (dir.x / dir.z) * d - p.x + f.x,
                    .y = (dir.y / dir.z) * d - p.y + f.y,
                };
                result.* = .{
                    .value = current_value,
                    .intercept = intercept,
                    .t = .{
                        .x = t.x + intercept.x,
                        .y = t.y + intercept.y,
                        .z = t.z + s.z.s,
                    },
                };
            },
        }
    }
};
pub const Game = struct {
    camera: Camera,
    world: World,
    brick_image: Sprite,

    pub fn userInit(self: *Game, alloc: Allocator, engine: *pge.EngineState) bool {
        _ = engine;
        const size = VU3D{ .x = 8, .y = 8, .z = 8 };
        var brick_img_image = Image.fromFilePath(
            alloc,
            "./resources/bricks.png",
        ) catch return false;
        defer brick_img_image.deinit();
        self.* = .{
            .camera = .{
                .pos = .{
                    .x = @floatFromInt(f32, size.x) / 2.0,
                    .y = @floatFromInt(f32, size.y) / 2.0,
                    .z = @floatFromInt(f32, size.z) / 2.0,
                },
                .dir = .{ .x = 0.0, .y = 0.0 },
                .fov = .{ .x = math.pi * (120.0 / 180.0), .y = math.pi / 2.0 },
            },
            .world = World.init(alloc, size) catch return false,
            .brick_image = Sprite.fromImage(alloc, brick_img_image) catch
                return false,
        };
        self.world.map[
            4 + (4 * self.world.size.x) +
                (4 * self.world.size.x * self.world.size.y)
        ] = 1;
        self.world.lights.append(alloc, .{
            .pos = .{ .x = 3, .y = 3, .z = 3 },
            .color = pge.Pixel.White,
            .brightness = 0.02,
        }) catch return false;
        self.world.lights.append(alloc, .{
            .pos = .{ .x = 6.5, .y = 6.5, .z = 2.5 },
            .color = pge.Pixel.Blue,
            .brightness = 0.02,
        }) catch return false;
        self.world.lights.append(alloc, .{
            .pos = .{ .x = 6.5, .y = 2.5, .z = 2.5 },
            .color = pge.Pixel.Red,
            .brightness = 0.02,
        }) catch return false;
        return true;
    }

    pub fn userUpdate(
        self: *Game,
        alloc: Allocator,
        engine: *pge.EngineState,
        elapsed_time: f32,
    ) bool {
        _ = alloc;

        self.camera.drawView(
            &self.world,
            self,
            engine.screen_size,
            engine,
        );

        var arena = engine.arena.allocator();
        engine.drawString(.{ .x = 4, .y = 4 }, std.fmt.allocPrint(
            arena,
            "xya: {d} {d} {d}",
            .{ self.camera.pos.x, self.camera.pos.y, self.camera.dir.x },
        ) catch "", pge.Pixel.Blue, 1);

        if (engine.keyPressed(.Escape)) return false;

        const turn_speed = 2.0;
        if (engine.keyHeld(.Left)) self.camera.dir.x -= turn_speed * elapsed_time;
        if (engine.keyHeld(.Right)) self.camera.dir.x += turn_speed * elapsed_time;
        if (engine.keyHeld(.Up)) self.camera.dir.y += turn_speed * elapsed_time;
        if (engine.keyHeld(.Down)) self.camera.dir.y -= turn_speed * elapsed_time;

        const move_speed = 3.0;
        if (engine.keyHeld(.W)) {
            self.camera.pos.x += math.cos(self.camera.dir.x) *
                move_speed * elapsed_time;
            self.camera.pos.y += math.sin(self.camera.dir.x) *
                move_speed * elapsed_time;
        }
        if (engine.keyHeld(.S)) {
            self.camera.pos.x -= math.cos(self.camera.dir.x) *
                move_speed * elapsed_time;
            self.camera.pos.y -= math.sin(self.camera.dir.x) *
                move_speed * elapsed_time;
        }
        if (engine.keyHeld(.A)) {
            self.camera.pos.x += math.cos(self.camera.dir.x - (math.pi / 2.0)) *
                move_speed * elapsed_time;
            self.camera.pos.y += math.sin(self.camera.dir.x - (math.pi / 2.0)) *
                move_speed * elapsed_time;
        }
        if (engine.keyHeld(.D)) {
            self.camera.pos.x += math.cos(self.camera.dir.x + (math.pi / 2.0)) *
                move_speed * elapsed_time;
            self.camera.pos.y += math.sin(self.camera.dir.x + (math.pi / 2.0)) *
                move_speed * elapsed_time;
        }
        if (engine.keyHeld(.Shift)) {
            self.camera.pos.z += move_speed * elapsed_time;
        }
        if (engine.keyHeld(.Space)) {
            self.camera.pos.z -= move_speed * elapsed_time;
        }
        return true;
    }

    pub fn userDeinit(self: *Game, alloc: Allocator, engine: *pge.EngineState) void {
        _ = engine;
        self.brick_image.deinit(alloc);
        self.world.deinit(alloc);
    }
};

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    var alloc = gpa.allocator();

    var engine = try pge.PixelGameEngine(Game).init(
        alloc,
        "tdcaster4",
        .{ .x = 2, .y = 2 },
        .{ .x = 512, .y = 384 },
    );
    defer engine.deinit(alloc);
    try engine.start(alloc);
}
