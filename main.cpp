#include <memory>
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>

#include <SDL.h>

#include "model.h"
#include "geometry.h"

Model *model = NULL;
const int width  = 1024;
const int height = 768;

struct sdl_deleter
{
  void operator()(SDL_Window *p) const { SDL_DestroyWindow(p); }
  void operator()(SDL_Renderer *p) const { SDL_DestroyRenderer(p); }
  void operator()(SDL_Texture *p) const { SDL_DestroyTexture(p); }
};

void drawPixel(SDL_Renderer* renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255 );
    SDL_RenderDrawPoint(renderer, x, y);
}

void drawPixel(SDL_Renderer* renderer, int x, int y, int r, int g, int b)
{
    SDL_SetRenderDrawColor(renderer, r, g, b, 255 );
    SDL_RenderDrawPoint(renderer, x, y);    
}

void drawLine(SDL_Renderer* renderer, int x0, int y0, int x1, int y1)
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
            drawPixel(renderer, y, x); 
        } 
        else 
        { 
            drawPixel(renderer, x, y); 
        } 

        error2 += derror2; 

        if (error2 > dx) 
        { 
            y += ((y1 > y0) ? 1 : -1); 
            error2 -= dx*2; 
        } 
    } 
} 

int main(int argc, char ** argv)
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
            switch (event.type) {
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
        SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255 );
        SDL_RenderClear(renderer.get());

        // For all faces
        for (int i=0; i<model->nfaces(); i++) 
        {
            std::vector<int> face = model->face(i);

            for (int j=0; j<3; j++) 
            {
                Vec3f v0 = model->vert(face[j]);
                Vec3f v1 = model->vert(face[(j+1)%3]);

                int x0 = (v0.x+1.)*width/2.;
                int y0 = (v0.y+1.)*height/2.;

                int x1 = (v1.x+1.)*width/2.;
                int y1 = (v1.y+1.)*height/2.;

                // Draw face
                drawLine(renderer.get(), x0, y0, x1, y1);
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
    SDL_Quit();
 
    // Goodbye!
    return 0;
}