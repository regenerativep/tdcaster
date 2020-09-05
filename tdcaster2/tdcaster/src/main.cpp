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
    inline void rotateVectorX(float* y, float* z, float c, float s)
    {
        double ny = (*y * c) - (*z * s);
        double nz = (*y * s) + (*z * c);
        *y = ny;
        *z = nz;
    }
    inline void rotateVectorY(float* x, float* z, float c, float s)
    {
        float nx = (*x * c) + (*z * s);
        float nz = (*z * c) - (*x * s);
        *x = nx;
        *z = nz;
    }
    inline void rotateVectorZ(float* x, float* y, float c, float s)
    {
        float nx = (*x * c) + (*y * s);
        float ny = (*x * s) - (*y * c);
        *x = nx;
        *y = ny;
    }
    inline olc::Pixel lightEffect(olc::Pixel pix, float distSqr, float lightAmount = 8) {
        // return olc::Pixel(
        //     std::min((int)(pix.r * (lightAmount / distSqr)), (int)pix.r),
        //     std::min((int)(pix.g * (lightAmount / distSqr)), (int)pix.g),
        //     std::min((int)(pix.b * (lightAmount / distSqr)), (int)pix.b)
        // );
        return olc::Pixel(
            std::min((int)(pix.r * (lightAmount / distSqr)), 255),
            std::min((int)(pix.g * (lightAmount / distSqr)), 255),
            std::min((int)(pix.b * (lightAmount / distSqr)), 255)
        );
    }
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
        int viewWidth, viewHeight;
        int viewDistance;
        float px, py, pz, pad, pap, pfh, pfv;
        float turnSpeed, moveSpeed;
        int viewMode;

        olc::Sprite* wallSprite;
        olc::Sprite* floorSprite;
        olc::Sprite* ceilSprite;
        TdCasterApplication(std::string path)
        {
            sAppName = "TopDownCaster2";
            this->path = path;
        }
        bool OnUserCreate() override
        {
            std::string resourcePath = path + PATH_SLASH + "resources" + PATH_SLASH;
            wallSprite = new olc::Sprite(resourcePath + "bricks.png");
            ceilSprite = new olc::Sprite(resourcePath + "ceiling.png");
            floorSprite = new olc::Sprite(resourcePath + "floor.png");

            viewMode = 0;
            viewWidth = ScreenWidth();//512;
            viewHeight = ScreenHeight();//384;
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
            pfh = M_PI * (120.f / 180.f);
            pfv = M_PI / 2;
            viewDistance = 16;
            return true;
        }
        inline RaycastCollision raycast(float x, float y, float z, float dx, float dy, float dz, int passes = 0) {
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
                    // continue;
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
            else if(lastDir == 2) {
                zp -= zi;
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
                tx += xs;
                ty += xInt;
                tz += yInt;
            }
            else if(lastDir == 1) //x axis aligned wall
            {
                float yD = yp + ys - yf;
                xInt = (dx / dy) * yD - xp + xf;
                yInt = (dz / dy) * yD - zp + zf;
                tx += xInt;
                ty += ys;
                tz += yInt;
            }
            else //z axis aligned surface
            {
                float zD = zp + zs - zf;
                xInt = (dx / dz) * zD - xp + xf;
                yInt = (dy / dz) * zD - yp + yf;
                tx += xInt;
                ty += yInt;
                tz += zs;
            }
            float ndx = tx - x;
            float ndy = ty - y;
            float ndz = tz - z;
            float distSqr = ndx * ndx + ndy * ndy + ndz * ndz;
            return RaycastCollision(value, distSqr, xInt, yInt, tx, ty, tz, dx, dy, dz);
        }
        inline void drawView()
        {
            float pc = cos(pap);
            float ps = sin(pap);
            float dc = cos(pad);
            float ds = sin(pad);
            float viewWidthd2 = tan(pfh / 2);
            float viewHeightd2 = tan(pfv / 2);
            float tlVecX = 1, tlVecY = viewWidthd2, tlVecZ = -viewHeightd2;
            float blVecX = 1, blVecY = viewWidthd2, blVecZ = viewHeightd2;
            float trVecX = 1, trVecY = -viewWidthd2, trVecZ = -viewHeightd2;
            rotateVectorY(&tlVecX, &tlVecZ, pc, ps);
            rotateVectorZ(&tlVecX, &tlVecY, dc, ds);
            rotateVectorY(&trVecX, &trVecZ, pc, ps);
            rotateVectorZ(&trVecX, &trVecY, dc, ds);
            rotateVectorY(&blVecX, &blVecZ, pc, ps);
            rotateVectorZ(&blVecX, &blVecY, dc, ds);
            float lDiffX = (blVecX - tlVecX) / viewHeight, lDiffY = (blVecY - tlVecY) / viewHeight, lDiffZ = (blVecZ - tlVecZ) / viewHeight;
            float cDiffX = (trVecX - tlVecX) / viewWidth, cDiffY = (trVecY - tlVecY) / viewWidth, cDiffZ = (trVecZ - tlVecZ) / viewWidth;
            float lVecX = tlVecX, lVecY = tlVecY, lVecZ = tlVecZ;
            for(int i = 0; i < viewHeight; i++) {
                float rVecX = lVecX, rVecY = lVecY, rVecZ = lVecZ;
                for(int j = 0; j < viewWidth; j++) {
                    RaycastCollision res = raycast(px, py, pz, rVecX, rVecY, rVecZ);
                    olc::Pixel drawColor = olc::BLANK;
                    switch(res.value) {
                        case 1:
                            drawColor = getPixelFromImage(floorSprite, res.xInt, res.yInt);
                            break;
                        case 2:
                            drawColor = getPixelFromImage(ceilSprite, res.xInt, res.yInt);
                            break;
                        case 3:
                            drawColor = getPixelFromImage(wallSprite, res.xInt, res.yInt);
                            break;
                    }
                    drawColor = lightEffect(drawColor, res.distSqr, 1);
                    Draw(j, i, drawColor);
                    rVecX += cDiffX; rVecY += cDiffY; rVecZ += cDiffZ;
                }
                lVecX += lDiffX; lVecY += lDiffY; lVecZ += lDiffZ;
            }
        }
        void drawTopDown() {
            int blockSize = 32;
            for(int i = 0; i < width; i++) {
                for(int j = 0; j < height; j++) {
                    int value = walls[i + j * width];
                    if(value != 0) {
                        DrawRect(i * blockSize, j * blockSize, blockSize, blockSize, olc::WHITE);
                    }
                }
            }
            int camRad = 4;
            int vx = (int)(px * blockSize);
            int vy = (int)(py * blockSize);
            DrawRect(vx - camRad, vy - camRad, camRad * 2, camRad * 2, olc::BLUE);
            float vam = cos(pap);
            TdCaster::RaycastCollision col = raycast(px, py, pz, cos(pad) * vam, sin(pad) * vam, sin(pap));
            olc::Pixel hitCol = olc::WHITE;
            switch(col.value) {
                case 1:
                    hitCol = olc::YELLOW;
                    break;
                case 2:
                    hitCol = olc::GREY;
                    break;
                case 3:
                    hitCol = olc::CYAN;
                    break;
            }
            DrawLine(vx, vy, col.tx * blockSize, col.ty * blockSize, hitCol);
            DrawLine(vx, vy, vx + cos(pad) * 16, vy + sin(pad) * 16, olc::RED);
            // DrawLine(vx, vy, vx + cos(pad) * sqrt(col.distSqr) * blockSize, vy + sin(pad) * sqrt(col.distSqr) * blockSize, olc::WHITE);
            DrawRect(width * blockSize, 0, blockSize, blockSize, olc::BLUE);
            DrawCircle((width + col.xInt) * blockSize, col.yInt * blockSize, 2, olc::RED);
        }
        bool OnUserUpdate(float elapsed) override
        {
            Clear(olc::Pixel(0, 0, 0));
            //draw pixels
            if(viewMode == 0) {
                drawView();
            }
            else if(viewMode == 1) {
                drawTopDown();
            }
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
                pap += turnSpeed * elapsed;
            }
            if(GetKey(olc::Key::DOWN).bHeld)
            {
                pap -= turnSpeed * elapsed;
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
            if(GetKey(olc::Key::V).bPressed) {
                viewMode = (viewMode + 1) % 2;
            }
            return true;
        }
    };
}

int main(int argc, char* argv[]) {
    std::string argv_str(argv[0]);
    std::string base = argv_str.substr(0, argv_str.find_last_of(PATH_SLASH));
    TdCaster::TdCasterApplication app(base);
    if(app.Construct(1024, 768, 1, 1)) {
        app.Start();
    }
    return 0;
}