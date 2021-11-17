#include "display.hpp"

#include <iostream>
#include <vector>

Display::Display(int width, int height, const std::string& title, bool resizeable, bool vsync, float xScale, float yScale) : width(width), height(height), title(title), xScale(xScale), yScale(yScale) {
    if(!SDL_WasInit(SDL_INIT_EVERYTHING)) {
        isInit = false;
        return;
    }
    
    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width*xScale, height*yScale, SDL_WINDOW_ALLOW_HIGHDPI | (resizeable * SDL_WINDOW_RESIZABLE));
    
	uint32_t flags = vsync * SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED;
    renderer = SDL_CreateRenderer(window, -1, flags);
    
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    
    this->pixels = new uint32_t[width * height];
    memset(pixels, 0, width*height*sizeof(uint32_t));
    
//    int pitch = 0;
//    SDL_LockTexture(texture, nullptr, (void**)&this->pixels, &pitch);
    
    
    isClosed = false;
    isInit = true;
}

Display::~Display() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    delete[] pixels;
}


void Display::Update() {
	resized = false;
    EventHandler::Update();
    if (EventHandler::GetEvents(SDL_QUIT).size() > 0) {
        isClosed = true;
    }
	
	if (auto e = EventHandler::GetEvents(SDL_WINDOWEVENT); e.size() > 0) {
		for (const auto& ev : e) {
			const SDL_Event& event = ev.get();
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				resized = true;
				
				width = event.window.data1;
				height = event.window.data2;
				
				uint32_t* newPixels = new uint32_t[width * height];
				memcpy(newPixels, pixels, width * sizeof(uint32_t));
				delete[] pixels;
				pixels = newPixels;
				
				SDL_DestroyTexture(texture);
				texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_STREAMING, width, height);
    			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
				break;
			}
		}
	}
	

    SDL_UpdateTexture(texture, NULL, pixels, width * sizeof(uint32_t));
	
    SDL_RenderClear(renderer);
	
	SDL_Rect srcRect{0,0, GetCanvasWidth(), GetCanvasHeight()};
	SDL_Rect dstRect{0,0, GetWindowWidth(), GetWindowHeight()};
    SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
    SDL_RenderPresent(renderer);
}

void Display::Clear() {
    memset(this->pixels, 0, width * height * sizeof(uint32_t));
}
void Display::Clear(cdr::RGB color) {
    for (int i = 0; i < width*height; i++)
        pixels[i] = cdr::RGBtoUINT(color);
}
void Display::Clear(cdr::RGBA color) {
    for (int i = 0; i < width*height; i++)
        pixels[i] = cdr::RGBtoUINT(color);
}

void Display::Abort() {
    isClosed = true;
}
