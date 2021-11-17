#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#if __has_include("SDL2/SDL.h")
# include <SDL2/SDL.h>
#else
# include <SDL.h>
#endif
#include <string>   
#include "eventHandler.hpp"
#include "cidr.hpp"

// A wraper for SDL_Window and some other stuff
class Display {
public:
    Display(int width, int height, const std::string& title, bool resizeable = false, bool vsync = false, float xScale = 1.0f, float yScale = 1.0f);
    
    void Update();
    void Clear();
    void Clear(cdr::RGB color);
    void Clear(cdr::RGBA color);
    void Abort();
    
    inline void SetPixel(int x, int y, cdr::RGBA color) {
        this->pixels[x + y * this->width] = cdr::RGBtoUINT(color);
    }
    inline void SetPixel(int x, int y, cdr::RGB color) {
        if(x >= 0 && y >= 0 && x < width && y < height)
            this->pixels[x + y * this->width] = cdr::RGBtoUINT(color);
    }
    inline bool IsInit() const {
        return isInit;
    }
    inline int GetCanvasWidth() const  {
        return width;
    }
    inline int GetCanvasHeight() const  {
        return height;
    }
	inline int GetWindowWidth() const  {
        return width*xScale;
    }
    inline int GetWindowHeight() const  {
        return height*yScale;
    }
    inline const std::string& GetTile() const  {
        return title;
    }
    inline uint32_t* GetPixels() {
        return this->pixels;
    }
    inline bool IsClosed() const  {
        return isClosed;
    }
    inline SDL_Window* GetWindow() const  {
        return window;
    }
	inline bool HasResized() const { 
		return resized;
	}

    ~Display();
    
private:
    bool isInit;
    
    // window fields
    SDL_Window* window;
    int width;
    int height;
    const std::string title;
    bool isClosed;
	bool resized = false;
    
    // gfx fields
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint32_t* pixels;
	float xScale;
	float yScale;
};


#endif /* DISPLAY_HPP */
