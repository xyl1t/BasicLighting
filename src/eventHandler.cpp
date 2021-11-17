#include "eventHandler.hpp"

#include <iostream>
#include <cstring>
#include <functional>

// NOTE: if the framerate is very low and the user calls IsLeftMouseDown() (or any other mouse button function) it may return a wrong answer, because the update method went through the mouse down and mouse up event in one go (because the framerate is very low)
void EventHandler::Update() {
    _mouseXRel = 0;
    _mouseYRel = 0;
    
    for(int i = 0; i < SDL_NUM_SCANCODES; i++) {
        _prevKeys[i] = _keys[i];
    }
	_capturedEvents.clear();
	int index{-1};
    while (SDL_PollEvent(&_event)) {
        _capturedEvents.push_back(_event);

		if (_event.type == SDL_WINDOWEVENT)
		{
			if (_event.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				index = _capturedEvents.size() - 1;
				// std::cout << _event.window.data1 << std::endl;
				// std::cout << "Window Resized! " << _capturedEvents[index].window.data1 << std::endl;
				
			}
		}

        if (_event.type == SDL_MOUSEMOTION) {
            _mouseXRel = _event.motion.xrel;
            _mouseYRel = _event.motion.yrel;
        }

		if (_event.type == SDL_MOUSEMOTION) {
			_mouseXRel = _event.motion.xrel;
			_mouseYRel = _event.motion.yrel;
			_prevMouseX = _mouseX;
			_prevMouseY = _mouseY;
			_mouseX = _event.motion.x;
			_mouseY = _event.motion.y;
		}
		if (_event.type == SDL_MOUSEBUTTONDOWN) {
			_isPrevLeftMouseDown = _isLeftMouseDown;
			_isPrevRightMouseDown = _isRightMouseDown;
			_isPrevMiddleMouseDown = _isMiddleMouseDown;

			_isLeftMouseDown |= _event.button.button == SDL_BUTTON_LEFT;
			_isRightMouseDown |= _event.button.button == SDL_BUTTON_RIGHT;
			_isMiddleMouseDown |= _event.button.button == SDL_BUTTON_MIDDLE;
		}
		if (_event.type == SDL_MOUSEBUTTONUP) {
			_isPrevLeftMouseDown = _isLeftMouseDown;
			_isPrevRightMouseDown = _isRightMouseDown;
			_isPrevMiddleMouseDown = _isMiddleMouseDown;

			_isLeftMouseDown &= _event.button.button != SDL_BUTTON_LEFT;
			_isRightMouseDown &= _event.button.button != SDL_BUTTON_RIGHT;
			_isMiddleMouseDown &= _event.button.button != SDL_BUTTON_MIDDLE;
		}
    }
	
	// if(index != -1) {
	// 	std::cout << "Window Resized! " << _capturedEvents[index].type << std::endl;
	// }

	// _isPrevLeftMouseDown = _isLeftMouseDown;
	// _isPrevRightMouseDown = _isRightMouseDown;
	// _isPrevMiddleMouseDown = _isMiddleMouseDown;
	
	// _prevMouseX = _mouseX;
	// _prevMouseY = _mouseY;
	
    // _isLeftMouseDown = SDL_GetMouseState(&_mouseX, &_mouseY) & SDL_BUTTON(SDL_BUTTON_LEFT);
    // _isRightMouseDown = SDL_GetMouseState(&_mouseX, &_mouseY) & SDL_BUTTON(SDL_BUTTON_RIGHT);
    // _isMiddleMouseDown = SDL_GetMouseState(&_mouseX, &_mouseY) & SDL_BUTTON(SDL_BUTTON_MIDDLE);
	
	// for (int i = 0; i < _capturedEvents.size(); i++) {
    //     if(_capturedEvents[i].type == SDL_WINDOWEVENT) {
	// 		if (_capturedEvents[i].window.event == SDL_WINDOWEVENT_RESIZED) 
	// 			std::cout << "Window Resized! " << _capturedEvents[i].type << std::endl;
	// 	}
    // }
}

std::vector<std::reference_wrapper<const SDL_Event>> EventHandler::GetEvents(uint32_t eventType) {
	std::vector<std::reference_wrapper<const SDL_Event>> events;
	for (const auto& event : _capturedEvents) {
        if(event.type == eventType) {
            events.push_back(std::cref(event));
		}
    }
    
    return events;
}
