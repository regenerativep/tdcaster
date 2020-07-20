#pragma once
#include <math.h>

class GameWorld
{
    private:
    int* map;
    int width;
    int height;
    public:
    int viewDistance;
    GameWorld()
    {
        map = nullptr;
        viewDistance = 16;
    }
    void setMap(int* map, int width, int height)
    {
        this->map = map;
        this->width = width;
        this->height = height;
    }
    inline int* getMap()
    {
        return map;
    }
    inline int getWidth()
    {
        return width;
    }
    inline int getHeight()
    {
        return height;
    }
    inline int getValue(int x, int y)
    {
        return map[x + y * width];
    }
    void raycast(float x, float y, float dx, float dy, int& value, float& distSqr, float& intercept, int& lastDir)
    {
        int cx = floor(x);
        int cy = floor(y);
        value = getValue(cx, cy);
        distSqr = 0;

        float xf = x - cx;
        float yf = y - cy;

        float idx = 1.0 / abs(dx);
        float idy = 1.0 / abs(dy);

        int xi, yi;
        int xs, ys;
        float tnv, tnh;
        if(dx > 0)
        {
            xi = 1;
            tnh = (1 - xf) * idx;
            xs = 1;
        }
        else if(dx < 0)
        {
            xi = -1;
            tnh = xf * idx;
            xs = 0;
        }
        else
        {
            xi = 0;
            tnh = xf * idx;
            xs = 0;
        }
        if(dy > 0)
        {
            yi = 1;
            tnv = (1 - yf) * idy;
            ys = 1;
        }
        else if(dy < 0)
        {
            yi = -1;
            tnv = yf * idy;
            ys = 0;
        }
        else
        {
            yi = 0;
            tnv = yf * idy;
            ys = 0;
        }
        int xp = 0, yp = 0;
        lastDir = 0;
        for(int i = 0; i < viewDistance; i++)
        {
            if(tnv < tnh)
            {
                tnv += idy;
                yp += yi;
                lastDir = 1;
                if(cy + yp < 0 || cy + yp >= height)
                {
                    break;
                }
            }
            else
            {
                tnh += idx;
                xp += xi;
                lastDir = 0;
                if(cx + xp < 0 || cx + xp >= width)
                {
                    break;
                }
            }
            value = getValue(cx + xp, cy + yp);
            if(value != 0)
            {
                break;
            }
        }
        float dxdy = dx / dy, dydx = dy / dx;
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
        if(lastDir == 0) //y intercept
        {
            intercept = dydx * (xp + xs - xf) - yp + yf;
            ty += intercept;
            tx += xs;
        }
        else if(lastDir == 1) //x intercept
        {
            intercept = dxdy * (yp + ys - yf) - xp + xf;
            tx += intercept;
            ty += ys;
        }
        float ndx = tx - x;
        float ndy = ty - y;
        distSqr = ndx * ndx + ndy * ndy;
        
        if(value == 2)
        {
            if(lastDir == 0)
            {
                dx *= -1;
                xi = -xi;
                xs = !xs;
                idx = 1.0 / abs(dx);
            }
            else if(lastDir == 1)
            {
                dy *= -1;
                yi = -yi;
                ys = !ys;
                idy = 1.0 / abs(dy);
            }
            float nDistSqr;
            raycast(tx, ty, dx, dy, value, nDistSqr, intercept, lastDir);
            distSqr = sqrt(nDistSqr) + sqrt(distSqr);
            distSqr *= distSqr;
        }
    }
};