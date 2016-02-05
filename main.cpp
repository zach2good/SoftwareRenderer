#include <memory>
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <limits>

#include <SDL.h>

#include "model.h"
#include "geometry.h"


const int width  = 800;
const int height = 800;
const int depth  = 255;

Model *model = NULL;
int *zbuffer = NULL;
Vec3f light_dir(0,0,-1);
Vec3f camera(0,0,3);

struct sdl_deleter
{
    void operator()(SDL_Window* p) const { SDL_DestroyWindow(p); }
    void operator()(SDL_Renderer* p) const { SDL_DestroyRenderer(p); }
    void operator()(SDL_Texture* p) const { SDL_DestroyTexture(p); }
};

void pixel(SDL_Renderer* renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawPoint(renderer, x, y);
}

void pixel(SDL_Renderer* renderer, int x, int y, int r, int g, int b)
{
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderDrawPoint(renderer, x, y);
}

void line(SDL_Renderer* renderer, int x0, int y0, int x1, int y1)
{
    bool steep = false;

    if (std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);

        steep = true;
    }

    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;

    for (int x = x0; x <= x1; x++)
    {
        if (steep)
        {
            pixel(renderer, y, x);
        }
        else
        {
            pixel(renderer, x, y);
        }

        error2 += derror2;

        if (error2 > dx)
        {
            y += ((y1 > y0) ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

Vec3f m2v(Matrix m) 
{
    return Vec3f(m[0][0]/m[3][0], m[1][0]/m[3][0], m[2][0]/m[3][0]);
}

Matrix v2m(Vec3f v) 
{
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

Matrix viewport(int x, int y, int w, int h) 
{
    Matrix m = Matrix::identity(4);
    m[0][3] = x+w/2.f;
    m[1][3] = y+h/2.f;
    m[2][3] = depth/2.f;

    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = depth/2.f;
    return m;
}

void triangle(SDL_Renderer* renderer, Vec3i t0, Vec3i t1, Vec3i t2, Vec2i uv0, Vec2i uv1, Vec2i uv2, float intensity, int *zbuffer) 
{
    if (t0.y==t1.y && t0.y==t2.y) return; // i dont care about degenerate triangles

    if (t0.y>t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
    if (t0.y>t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
    if (t1.y>t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }

    int total_height = t2.y-t0.y;

    for (int i=0; i<total_height; i++) 
    {
        bool second_half = i>t1.y-t0.y || t1.y==t0.y;
        int segment_height = second_half ? t2.y-t1.y : t1.y-t0.y;
        float alpha = (float)i/total_height;
        float beta  = (float)(i-(second_half ? t1.y-t0.y : 0))/segment_height; // be careful: with above conditions no division by zero here

        Vec3i A   =               t0  + Vec3f(t2-t0  )*alpha;
        Vec3i B   = second_half ? t1  + Vec3f(t2-t1  )*beta : t0  + Vec3f(t1-t0  )*beta;
        Vec2i uvA =               uv0 +      (uv2-uv0)*alpha;
        Vec2i uvB = second_half ? uv1 +      (uv2-uv1)*beta : uv0 +      (uv1-uv0)*beta;

        if (A.x>B.x) { std::swap(A, B); std::swap(uvA, uvB); }

        for (int j=A.x; j<=B.x; j++) 
        {
            float phi = B.x==A.x ? 1. : (float)(j-A.x)/(float)(B.x-A.x);
            Vec3i   P = Vec3f(A) + Vec3f(B-A)*phi;
            Vec2i uvP =     uvA +   (uvB-uvA)*phi;
            int idx = P.x+P.y*width;

            if (zbuffer[idx]<P.z) 
            {
                zbuffer[idx] = P.z;
                TGAColor color = model->diffuse(uvP);
               pixel(renderer, P.x, P.y, color.r*intensity, color.g*intensity, color.b*intensity);
            }
        }
    }
}


int main(int argc, char** argv)
{
    model = new Model("obj/african_head.obj");

    bool quit = false;
    SDL_Event event;

    SDL_Init(SDL_INIT_VIDEO);

    auto window = std::unique_ptr<SDL_Window, sdl_deleter>(
             SDL_CreateWindow("Software Renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0),
             sdl_deleter());

    auto renderer = std::unique_ptr<SDL_Renderer, sdl_deleter>(
             SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED),
             sdl_deleter());

    auto screen = std::unique_ptr<SDL_Texture, sdl_deleter>(
             SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height),
             sdl_deleter());

    std::chrono::high_resolution_clock timer;

    auto start = timer.now();
    auto stop = timer.now();
    float deltaTime = 0.0f;

    while (!quit)
    {
        // Handle input
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                quit = true;
                break;

                default:
                    break;
            }
        }

        // Start timer
        start = timer.now();

        // Render to screen texture
        SDL_SetRenderTarget(renderer.get(), screen.get());

        // Clear
        SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
        SDL_RenderClear(renderer.get());

        zbuffer = new int[width * height];
        for (int i = 0; i < width * height; i++)
        {
            zbuffer[i] = std::numeric_limits < int >::min();
        }

         // draw the model
        Matrix Projection = Matrix::identity(4);
        Matrix ViewPort   = viewport(width/8, height/8, width*3/4, height*3/4);
        Projection[3][2] = -1.f/camera.z;

        for (int i=0; i<model->nfaces(); i++) 
        {
            std::vector<int> face = model->face(i);
            Vec3i screen_coords[3];
            Vec3f world_coords[3];

            for (int j=0; j<3; j++) 
            {
                Vec3f v = model->vert(face[j]);
                screen_coords[j] =  m2v(ViewPort*Projection*v2m(v));
                world_coords[j]  = v;
            }

            Vec3f n = (world_coords[2]-world_coords[0])^(world_coords[1]-world_coords[0]);
            n.normalize();
            float intensity = n*light_dir;

            if (intensity>0) 
            {
                Vec2i uv[3];
                for (int k=0; k<3; k++) 
                {
                    uv[k] = model->uv(i, k);
                }

                triangle(renderer.get(), screen_coords[0], screen_coords[1], screen_coords[2], uv[0], uv[1], uv[2], intensity, zbuffer);
            }
        }

        // Release screen texture
        SDL_SetRenderTarget(renderer.get(), NULL);

        // Flip screen texture
        SDL_RenderCopyEx(renderer.get(), screen.get(), NULL, NULL, 0, NULL, SDL_FLIP_VERTICAL);

        // Render texture to screen
        SDL_RenderPresent(renderer.get());

        // Print render time in ms
        stop = timer.now();
        deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << deltaTime << "ms" << std::endl;
    }

    // Cleanup
    delete model;
    delete[] zbuffer;
    SDL_Quit();

    // Goodbye!
    return 0;
}