#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <experimental/filesystem>
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include "world.h"

#ifdef _WIN32
    #define PATH_SLASH "\\"
#else
    #define PATH_SLASH "/"
#endif

class TdCasterApplication : public olc::PixelGameEngine
{
    public:
    std::string path;
    TdCasterApplication(std::string path)
    {
        sAppName = "TopDownCaster";
        this->path = path;
    }
    public:
    tdcaster::GameWorld world;
    float cx, cy, ca;
    float fov, vfov;
    int viewMode;
    float turnSpeed, moveSpeed;
    olc::Sprite* wallSprite;
    olc::Decal* wallDecal;
    bool OnUserCreate() override
    {
        wallSprite = new olc::Sprite(path + PATH_SLASH + "resources" + PATH_SLASH + "bricks.png");
        wallDecal = new olc::Decal(wallSprite);
        viewMode = 0;
        moveSpeed = 2;
        turnSpeed = 3;
        cx = 1;
        cy = 1;
        ca = 0;
        fov = M_PI / 2;
        vfov = M_PI / 3;
        world = tdcaster::GameWorld();
        world.setMap(new int[16*16] {
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 1, 0, 0, 0, 0, 0, 6, 0, 0, 2, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1,
            1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
        }, 16, 16);
        return true;
    }
    void drawColumn(olc::Decal& image, int x, float intercept, float distSqr, olc::Pixel lightFunc(olc::Pixel, float))
    {
        float dist = sqrt(distSqr);
        float size = atan(0.5 / dist) * ScreenHeight() / vfov;
        int col = (int)(intercept * image.sprite->width);
        int heightd2 = ScreenHeight() / 2;
        DrawPartialDecal(olc::vf2d(x, heightd2 - size), olc::vf2d(1, size * 2), wallDecal, olc::vf2d(col, 0), olc::vf2d(1, image.sprite->height), lightFunc(olc::WHITE, distSqr));
    }
    void drawRaycast(tdcaster::RaycastCollision col, int x, float dist = 0)
    {
        float actualDist = col.getDist() + dist;
        auto lightFunc = [](olc::Pixel pix, float distSqr) {
            // return olc::Pixel(
            //     std::min((int)(pix.r * (16.0 / distSqr)), (int)pix.r),
            //     std::min((int)(pix.g * (16.0 / distSqr)), (int)pix.g),
            //     std::min((int)(pix.b * (16.0 / distSqr)), (int)pix.b)
            // );
            return pix;
        };
        if((col.value & 1) != 0)
        {
            drawColumn(*wallDecal, x, col.intercept, actualDist * actualDist, lightFunc);
        }
        if((col.value & 2) != 0)
        {
            float rvx = col.dx, rvy = col.dy;
            if(col.lastDir == 0)
            {
                rvx *= -1;
            }
            else if(col.lastDir == 1)
            {
                rvy *= -1;
            }
            drawRaycast(world.raycast(col.tx, col.ty, rvx, rvy, col.i), x, actualDist);
        }
        if((col.value & 4) != 0)
        {
            drawColumn(*wallDecal, x, col.intercept, actualDist * actualDist, [](olc::Pixel pix, float distSqr) {
                return olc::Pixel(
                    // std::min((int)(pix.r * (16.0 / distSqr)), (int)pix.r),
                    // std::min((int)(pix.g * (16.0 / distSqr)), (int)pix.g),
                    // std::min((int)(pix.b * (16.0 / distSqr)), (int)pix.b),
                    pix.r, pix.g, pix.b,
                    127
                );
            });
        }
    }
    void drawView()
    {
        Clear(olc::Pixel(0, 0, 0));
        float fovd2 = fov / 2;
        float beginDir = ca - fovd2;
        int width = ScreenWidth(), height = ScreenHeight();
        int heightd2 = height / 2;
        
        for(int i = 0; i < width; i++)
        {
            float dir = beginDir + (fov * i / width);
            float vx = cos(dir), vy = sin(dir);
            tdcaster::RaycastCollision col = world.raycast(cx, cy, vx, vy);
            drawRaycast(col, i);
        }
    }
    void drawTopDown()
    {
        int blockSize = 16;
        Clear(olc::Pixel(255, 255, 255));
        int* map = world.getMap();
        for(int i = 0; i < world.getWidth(); i++)
        {
            for(int j = 0; j < world.getHeight(); j++)
            {
                int value = world.getValue(i, j);
                int x = i * blockSize;
                int y = j * blockSize;
                if(value == 1)
                {
                    //FillRect(x, y, blockSize, blockSize, olc::Pixel(255, 0, 0));
                    //DrawRect(x, y, blockSize, blockSize, olc::Pixel(201, 0, 0));
                    DrawSprite(x, y, wallSprite);
                }
            }
        }
        int cameraRad = 4;
        int px = (int)(cx * blockSize);
        int py = (int)(cy * blockSize);
        DrawRect(px - cameraRad, py - cameraRad, cameraRad * 2, cameraRad * 2, olc::Pixel(0, 0, 255));
        tdcaster::RaycastCollision col = world.raycast(cx, cy, cos(ca), sin(ca));
        float dist = sqrt(col.distSqr);
        DrawLine(px, py, px + cos(ca) * dist * blockSize, py + sin(ca) * dist * blockSize, olc::Pixel(0, 0, 255));
    }
    bool OnUserUpdate(float fElapsedTime) override
    {
        if(viewMode == 0)
        {
            drawView();
        }
        else if(viewMode == 1)
        {
            drawTopDown();
        }
        DrawString(4, 4, "xya: " + std::to_string(cx) + ", " + std::to_string(cy) + ", " + std::to_string(ca), olc::BLUE);
        if(GetKey(olc::Key::LEFT).bHeld)
        {
            ca -= turnSpeed * fElapsedTime;
        }
        if(GetKey(olc::Key::RIGHT).bHeld)
        {
            ca += turnSpeed * fElapsedTime;
        }
        if(GetKey(olc::Key::W).bHeld)
        {
            cx += cos(ca) * moveSpeed * fElapsedTime;
            cy += sin(ca) * moveSpeed * fElapsedTime;
        }
        if(GetKey(olc::Key::S).bHeld)
        {
            cx += cos(ca) * -moveSpeed * fElapsedTime;
            cy += sin(ca) * -moveSpeed * fElapsedTime;
        }
        if(GetKey(olc::Key::A).bHeld)
        {
            cx += cos(ca - M_PI / 2) * moveSpeed * fElapsedTime;
            cy += sin(ca - M_PI / 2) * moveSpeed * fElapsedTime;
        }
        if(GetKey(olc::Key::D).bHeld)
        {
            cx += cos(ca + M_PI / 2) * moveSpeed * fElapsedTime;
            cy += sin(ca + M_PI / 2) * moveSpeed * fElapsedTime;
        }
        if(GetKey(olc::Key::V).bPressed)
        {
            viewMode++;
            if(viewMode >= 2) viewMode = 0;
        }
        return true;
    }
};
int main(int argc, char* argv[])
{
    std::string argv_str(argv[0]);
    std::string base = argv_str.substr(0, argv_str.find_last_of(PATH_SLASH));
    TdCasterApplication app(base);
    if(app.Construct(1024, 768, 1, 1))
    {
        app.Start();
    }
    return 0;
}