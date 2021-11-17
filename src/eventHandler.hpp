#ifndef EVENTHANDLER_HPP
#define EVENTHANDLER_HPP

#if __has_include("SDL2/SDL.h")
# include <SDL2/SDL.h>
#else
# include <SDL.h>
#endif

#include <vector>

namespace EventHandler {
    inline SDL_Event _event{};
    // input fields
    inline uint8_t _prevKeys[SDL_NUM_SCANCODES] { 0 };
    inline uint8_t* _keys{(uint8_t*)SDL_GetKeyboardState(nullptr)};
    inline int _mouseX{};
    inline int _mouseY{};
    inline int _prevMouseX{};
    inline int _prevMouseY{};
    inline int _mouseXRel{};
    inline int _mouseYRel{};
    inline int _isPrevLeftMouseDown{};
    inline int _isPrevRightMouseDown{};
    inline int _isPrevMiddleMouseDown{};
    inline int _isLeftMouseDown{};
    inline int _isRightMouseDown{};
    inline int _isMiddleMouseDown{};
    inline std::vector<SDL_Event> _capturedEvents(32);
	
    void Update();
    
    inline bool IsKeyDown(int key) {
    	return (_keys[key]);
	}
    inline bool IsKeyUp(int key) {
    	return !(_keys[key]);
	}
    inline bool IsKeyPressed(int key) {
    	return (!(bool)_prevKeys[key] && (bool)_keys[key]);
	}
    inline int GetMouseX() {
        return _mouseX;
    }
    inline int GetMouseY() {
        return _mouseY;
    }
    inline int GetMouseXRel() {
        return _mouseXRel;
    }
    inline int GetMouseYRel() {
        return _mouseYRel;
    }
    inline bool IsLeftMouseDown() {
        return _isLeftMouseDown;
    }
    inline bool IsRightMouseDown() {
        return _isRightMouseDown;
    }
	inline bool IsLeftMouseClicked() {
		return (!_isPrevLeftMouseDown && _isLeftMouseDown);
	}
	inline bool IsRightMouseClicked() {
		return (!_isPrevRightMouseDown && _isRightMouseDown);
	}
    //bool IsMiddleMouseDown();
	
	inline const std::vector<SDL_Event>& GetAllEvents() { return _capturedEvents; }
    std::vector<std::reference_wrapper<const SDL_Event>> GetEvents(uint32_t eventType);
};


#endif /* EVENTHANDLER_HPP */
