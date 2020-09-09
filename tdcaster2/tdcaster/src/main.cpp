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
    inline void projectVector(float x, float y, float z, float tx, float ty, float tz, float* ox, float* oy, float* oz) {
        float dotWithT = x * tx + y * ty + z * tz;
        float tMagSqr = tx * tx + ty * ty + tz * tz;
        float scalar = dotWithT / tMagSqr;
        *ox = tx * scalar;
        *oy = ty * scalar;
        *oz = tz * scalar;
    }
    inline olc::Pixel dimEffect(olc::Pixel pix, float amount) {
        return olc::Pixel(
            (int)(pix.r / amount),
            (int)(pix.g / amount),
            (int)(pix.b / amount)
        );
    }
    struct Light {
        float x;
        float y;
        float z;
        float lr;
        float lg;
        float lb;
    };
    inline olc::Pixel lightEffect(olc::Pixel pix, float x, float y, float z, std::vector<Light> lights) {//float distSqr, float ar, float ag, float ab) {
        float arS = 0;
        float agS = 0;
        float abS = 0;
        for(int i = 0; i < lights.size(); i++) {
            auto& light = lights.at(i);
            float ndx = x - light.x;
            float ndy = y - light.y;
            float ndz = z - light.z;
            float distSqr = ndx * ndx + ndy * ndy + ndz * ndz;
            arS += light.lr / distSqr;
            agS += light.lg / distSqr;
            abS += light.lb / distSqr;
        }
        return olc::Pixel(
            std::min((int)(pix.r * arS), 255),
            std::min((int)(pix.g * agS), 255),
            std::min((int)(pix.b * abS), 255)
        );
    }
    // inline olc::Pixel normalMapEffect(float toLightX, float toLightY, float toLightZ, float toLightSqr, float toCamX, float toCamY, float toCamZ, float ar, float ag, float ab, float* ox, float* oy, float* oz) {
        
    // }
    struct RaycastCollision
    {
        int value;
        float xInt, yInt;
        float tx, ty, tz;
        float dx, dy, dz;
        int dir;
        RaycastCollision(int value, float xInt, float yInt, float tx, float ty, float tz, float dx, float dy, float dz, int dir)
        {
            this->value = value;
            this->xInt = xInt;
            this->yInt = yInt;
            this->tx = tx;
            this->ty = ty;
            this->tz = tz;
            this->dx = dx;
            this->dy = dy;
            this->dz = dz;
            this->dir = dir;
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
        std::vector<Light> lights;

        olc::Sprite* wallSprite;
        olc::Sprite* floorSprite;
        olc::Sprite* floorNmSprite;
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
            floorNmSprite = new olc::Sprite(resourcePath + "floor_nm.png");

            lights = std::vector<Light>();
            lights.push_back(Light { 2, 2, 0.7, 1, 0, 0 });
            lights.push_back(Light { 6, 6, 0.7, 0, 1, 0 });
            lights.push_back(Light { 4, 4, 0.3, 0.5, 0.5, 0.5 });
            viewMode = 0;
            viewWidth = ScreenWidth();
            viewHeight = ScreenHeight();
            width = 8;
            height = 8;
            turnSpeed = 2;
            moveSpeed = 3;
            int totalCells = width * height;
            ceiling = new int[totalCells] {
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 2, 2, 2, 2, 1, 1,
                1, 1, 2, 1, 1, 2, 1, 1,
                1, 1, 2, 1, 1, 2, 1, 1,
                1, 1, 2, 2, 2, 2, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1
            };
            floor = new int[totalCells] {
                2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2,
                2, 2, 2, 1, 1, 2, 2, 2,
                2, 2, 2, 1, 1, 2, 2, 2,
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
            // calculate initial conditions
            // - floored initial position
            int cx = std::floor(x);
            int cy = std::floor(y);
            int cz = std::floor(z);
            // - fraction only value
            float xf = x - cx;
            float yf = y - cy;
            float zf = z - cz;
            // - step values
            float idx = 1.0 / abs(dx);
            float idy = 1.0 / abs(dy);
            float idz = 1.0 / abs(dz);
            // - sign of direction vector components
            int xi, yi, zi;
            // - sign of direction vector components, but 0 is nonpositive
            int xs, ys, zs;
            // - total amount of step
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
            // scene traversal loop
            // - array index of world array
            int ind = cx + cy * width;
            // - starting value from current position in world array
            int value = walls[ind];
            // - total integer change in cell position
            int xp = 0, yp = 0, zp = 0;
            // - last cell intersection direction (direction being which way the face aligns)
            int lastDir = 0; //0 -> x, 1 -> y, 2 -> z // TODO: use an enum
            for(; passes < viewDistance; passes++)
            {
                // run the smallest tn* axis value (finds the axis of our intersected face)
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
                }
                // step in direction of face
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
                    //we dont care about moving up or down. we know we hit a floor or ceiling. break
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
                // update current value
                value = walls[ind];
                // if value is not 0, then we hit something
                if(value != 0)
                {
                    break;
                }
            }
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
            return RaycastCollision(value, xInt, yInt, tx, ty, tz, dx, dy, dz, lastDir);
        }
        void drawView()
        {
            // rotation vectors
            float pc = cos(pap);
            float ps = sin(pap);
            float dc = cos(pad);
            float ds = sin(pad);
            float viewWidthd2 = tan(pfh / 2);
            float viewHeightd2 = tan(pfv / 2);
            // basic view positions
            float tlVecX = 1, tlVecY = viewWidthd2, tlVecZ = -viewHeightd2;
            float blVecX = 1, blVecY = viewWidthd2, blVecZ = viewHeightd2;
            float trVecX = 1, trVecY = -viewWidthd2, trVecZ = -viewHeightd2;
            // rotate starting view positions to fit our view
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
                    float lr = 0;
                    float lg = 0;
                    float lb = 0;
                    // distance from light
                    // float ndx = res.tx - 4;
                    // float ndy = res.ty - 4;
                    // float ndz = res.tz - 0.5;
                    // float distSqr = ndx * ndx + ndy * ndy + ndz * ndz;
                    // find the color
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
                    // singular light effect
                    drawColor = lightEffect(drawColor, res.tx, res.ty, res.tz, this->lights);
                    // draw pixel
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