#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <experimental/filesystem>
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#ifdef _WIN32
    #define PATH_SLASH "\\"
#else
    #define PATH_SLASH "/"
#endif

namespace TdCaster
{
    struct RaycastCollision
    {
        int value;
        float distSqr;
        float xInt, yInt;
        float tx, ty, tz;
        float dx, dy, dz;
        RaycastCollision(int value, float distSqr, float xInt, float yInt, float tx, float ty, float tz, float dx, float dy, float dz)
        {
            this->value = value;
            this->distSqr = distSqr;
            this->xInt = xInt;
            this->yInt = yInt;
            this->tx = tx;
            this->ty = ty;
            this->tz = tz;
            this->dx = dx;
            this->dy = dy;
            this->dz = dz;
        }
    };
    inline olc::Pixel getPixelFromImage(olc::Sprite* sprite, float xInt, float yInt)
    {
        return sprite->GetPixel(xInt * sprite->width, yInt * sprite->height);
    }
    class TdCasterApplication : public olc::PixelGameEngine
    {
        public:
        std::string path;
        int* ceiling;
        int* walls;
        int* floor;
        int width, height;
        int viewDistance;
        float px, py, pz, pad, pap, pfh, pfv;
        float turnSpeed, moveSpeed;
        TdCasterApplication(std::string path)
        {
            sAppName = "TopDownCaster2";
            this->path = path;
        }
        bool OnUserCreate() override
        {
            width = 8;
            height = 8;
            turnSpeed = 2;
            moveSpeed = 3;
            int totalCells = width * height;
            ceiling = new int[totalCells] {
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1
            };
            floor = new int[totalCells] {
                2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2
            };
            walls = new int[totalCells] {
                3, 3, 3, 3, 3, 3, 3, 3,
                3, 0, 0, 0, 0, 0, 0, 3,
                3, 0, 0, 0, 0, 0, 0, 3,
                3, 0, 0, 0, 0, 0, 0, 3,
                3, 0, 0, 0, 0, 0, 0, 3,
                3, 0, 0, 0, 0, 0, 0, 3,
                3, 0, 0, 0, 0, 0, 0, 3,
                3, 3, 3, 3, 3, 3, 3, 3
            };
            px = 1;
            py = 1;
            pz = 0.5;
            pad = 0;
            pap = 0;
            pfh = M_PI / 2;
            pfv = M_PI / 3;
            viewDistance = 16;
            return true;
        }
        RaycastCollision raycast(float x, float y, float z, float dx, float dy, float dz, int passes = 0)
        {
            int cx = std::floor(x);
            int cy = std::floor(y);
            int cz = std::floor(z);
            int ind = cx + cy * width;
            int value = walls[ind];
            float xf = x - cx;
            float yf = y - cy;
            float zf = z - cz;
            float idx = 1.0 / abs(dx);
            float idy = 1.0 / abs(dy);
            float idz = 1.0 / abs(dz);
            int xi, yi, zi;
            int xs, ys, zs;
            float tnv, tnh, tnl;
            if(dx > 0)
            {
                xi = 1;
                tnh = (1 - xf) * idx;
                xs = 1;
            }
            else
            {
                if(dx < 0)
                {
                    xi = -1;
                }
                else
                {
                    xi = 0;
                }
                tnh = xf * idx;
                xs = 0;
            }
            if(dy > 0)
            {
                yi = 1;
                tnv = (1 - yf) * idy;
                ys = 1;
            }
            else
            {
                if(dy < 0)
                {
                    yi = -1;
                }
                else
                {
                    yi = 0;
                }
                tnv = yf * idy;
                ys = 0;
            }
            if(dz > 0)
            {
                zi = 1;
                tnl = (1 - zf) * idz;
                zs = 1;
            }
            else
            {
                if(dz < 0)
                {
                    zi = -1;
                }
                else
                {
                    zi = 0;
                }
                tnl = zf * idz;
                zs = 0;
            }
            int xp = 0, yp = 0, zp = 0;
            int lastDir = 0; //0 -> x, 1 -> y, 2 -> z
            for(; passes < viewDistance; passes++)
            {
                //run the smallest tn axis value
                //tnl is default
                lastDir = 2;
                if(tnv < tnh)
                {
                    if(tnv < tnl)
                    {
                        //tnv
                        lastDir = 1;
                    }
                }
                else if(tnh < tnl)
                {
                    //tnh
                    lastDir = 0;
                    continue;
                }
                if(lastDir == 0)
                {
                    tnh += idx;
                    xp += xi;
                    ind += xi;
                    if(cx + xp < 0 || cx + xp >= width)
                    {
                        break;
                    }
                }
                else if(lastDir == 1)
                {
                    tnv += idy;
                    yp += yi;
                    ind += yi * width;
                    if(cy + yp < 0 || cy + yp >= height)
                    {
                        break;
                    }
                }
                else
                {
                    //we dont care about moving up or down. we know we hit a floor or ceiling
                    if(zi == 1)
                    {
                        value = ceiling[ind];
                    }
                    else
                    {
                        value = floor[ind];
                    }
                    zp += zi;
                    break;
                }
                value = walls[ind];
                if(value != 0)
                {
                    break;
                }
            }
            //float dxdy = dx / dy, dydx = dy / dx;
            if(lastDir == 0)
            {
                xp -= xi;
            }
            else if(lastDir == 1)
            {
                yp -= yi;
            }
            float tx = cx + xp;
            float ty = cy + yp;
            float tz = cz + zp;
            float xInt, yInt; //we need to find the x and y of where on the cube's face we hit
            if(lastDir == 0) //y axis aligned wall
            {
                float xD = xp + xs - xf;
                xInt = (dy / dx) * xD - yp + yf;
                yInt = (dz / dx) * xD - zp + zf;
                ty += xInt;
                tz += yInt;
            }
            else if(lastDir == 1) //x axis aligned wall
            {
                float yD = yp + ys - yf;
                xInt = (dx / dy) * yD - xp + xf;
                yInt = (dz / dy) * yD - zp + zf;
                tx += xInt;
                tz += yInt;
            }
            else //z axis aligned surface
            {
                float zD = zp + zs - zf;
                xInt = (dx / dz) * zD - xp + xf;
                yInt = (dy / dz) * zD - yp + yf;
                tx += xInt;
                ty += yInt;
            }
            float ndx = tx - x;
            float ndy = ty - y;
            float ndz = tz - z;
            float distSqr = ndx * ndx + ndy * ndy + ndz * ndz;
            return RaycastCollision(value, distSqr, xInt, yInt, tx, ty, tz, dx, dy, dz);
        }
        void drawView()
        {
            
        }
        bool OnUserUpdate(float elapsed) override
        {
            //draw pixels
            drawView();
            //handle input
            DrawString(4, 4, "xya: " + std::to_string(px) + ", " + std::to_string(py) + ", " + std::to_string(pad), olc::BLUE);
            if(GetKey(olc::Key::LEFT).bHeld)
            {
                pad -= turnSpeed * elapsed;
            }
            if(GetKey(olc::Key::RIGHT).bHeld)
            {
                pad += turnSpeed * elapsed;
            }
            if(GetKey(olc::Key::UP).bHeld)
            {
                pap -= turnSpeed * elapsed;
            }
            if(GetKey(olc::Key::DOWN).bHeld)
            {
                pap += turnSpeed * elapsed;
            }
            if(GetKey(olc::Key::W).bHeld)
            {
                px += cos(pad) * moveSpeed * elapsed;
                py += sin(pad) * moveSpeed * elapsed;
            }
            if(GetKey(olc::Key::S).bHeld)
            {
                px += cos(pad) * -moveSpeed * elapsed;
                py += sin(pad) * -moveSpeed * elapsed;
            }
            if(GetKey(olc::Key::A).bHeld)
            {
                px += cos(pad - M_PI / 2) * moveSpeed * elapsed;
                py += sin(pad - M_PI / 2) * moveSpeed * elapsed;
            }
            if(GetKey(olc::Key::D).bHeld)
            {
                px += cos(pad + M_PI / 2) * moveSpeed * elapsed;
                py += sin(pad + M_PI / 2) * moveSpeed * elapsed;
            }
            return true;
        }
    };
}