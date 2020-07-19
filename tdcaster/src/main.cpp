#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include <cmath>
#include "world.h"

class TdCasterApplication : public olc::PixelGameEngine
{
    public:
    TdCasterApplication()
    {
        sAppName = "TopDownCaster";
    }
    public:
    GameWorld world;
    float cx, cy, ca;
    float fov, vfov;
    int viewMode;
    bool OnUserCreate() override
    {
        viewMode = 0;
        cx = 1;
        cy = 1;
        ca = 0;
        fov = M_PI / 2;
        vfov = M_PI / 3;
        world = GameWorld();
        world.setMap(new int[16*16] {
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
            1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1,
            1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1,
            1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1,
            1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1,
            1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
        }, 16, 16);
        int value;
        float distSqr;
        float intercept;
        int lastDir;
        world.raycast(cx, cy, cos(ca), sin(ca), value, distSqr, intercept, lastDir);
        return true;
    }
    void drawView()
    {
        Clear(olc::Pixel(255, 255, 255));
        float fovd2 = fov / 2;
        float la = ca - fovd2;
        float lvx = cos(la);
        float lvy = sin(la);
        float ra = ca + fovd2;
        float rvx = cos(ra);
        float rvy = sin(ra);
        int width = ScreenWidth(), height = ScreenHeight();
        int heightd2 = height / 2;
        float dvx = (rvx - lvx) / width;
        float dvy = (rvy - lvy) / width; //yes, this is width
        for(int i = 0; i < width; i++)
        {
            float vx = lvx + dvx * i;
            float vy = lvy + dvy * i;
            int value;
            float distSqr;
            float intercept;
            int lastDir;
            if(i == width / 2)
            {
                std::cout << "intercept: ";
            }
            world.raycast(cx, cy, vx, vy, value, distSqr, intercept, lastDir);
            if(i == width / 2)
            {
                std::cout << intercept << "\n";
            }
            float dist = sqrt(distSqr);
            float size = atan(1 / dist) * height / vfov;
            DrawLine(i, heightd2 - size, i, heightd2 + size, olc::Pixel(std::min(255, (int)(512 / dist)), 0, 0));
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
                    FillRect(x, y, blockSize, blockSize, olc::Pixel(255, 0, 0));
                    DrawRect(x, y, blockSize, blockSize, olc::Pixel(201, 0, 0));
                }
            }
        }
        int cameraRad = 4;
        int px = (int)(cx * blockSize);
        int py = (int)(cy * blockSize);
        DrawRect(px - cameraRad, py - cameraRad, cameraRad * 2, cameraRad * 2, olc::Pixel(0, 0, 255));
        //DrawLine(px, py, px + cos(ca) * 8, py + sin(ca) * 8, olc::Pixel(0, 0, 255));
        int value, lastDir;
        float distSqr, intercept;
        world.raycast(cx, cy, cos(ca), sin(ca), value, distSqr, intercept, lastDir);
        float dist = sqrt(distSqr);
        DrawLine(px, py, px + cos(ca) * dist * blockSize, py + sin(ca) * dist * blockSize, olc::Pixel(0, 0, 255));
                std::cout << "intercept: " << intercept << "\n";
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
            ca -= 0.05;
        }
        if(GetKey(olc::Key::RIGHT).bHeld)
        {
            ca += 0.05;
        }
        if(GetKey(olc::Key::W).bHeld)
        {
            cx += cos(ca) * 0.02;
            cy += sin(ca) * 0.02;
        }
        if(GetKey(olc::Key::V).bPressed)
        {
            viewMode++;
            if(viewMode >= 2) viewMode = 0;
        }
        return true;
    }
};
int main()
{
    TdCasterApplication app;
    if(app.Construct(256, 192, 4, 4))
    {
        app.Start();
    }
    return 0;
}