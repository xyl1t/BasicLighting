#include <iostream>
#include <map>
#include <thread>
#include <fstream>
#include <sstream>
#include <queue>

#if __has_include("SDL2/SDL.h")
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#define CIDR_IMPLEMENTATION
#include "cidr.hpp"
using namespace cdr;

#include "display.hpp"
#include "eventHandler.hpp"
#include "timer.hpp"
#include "generation.hpp"

int linesCount = 0;
int windowWidth;
int windowHeight;
int pixelSize;
float maxLight = 3;//6.9;
Generation gen(16, 16, 4, 4, 4, 4); // NOTE: unfortunately no default constructor, so putting some dummy ctor here
std::vector<std::vector<float>> lightMap;
std::ofstream logFile("log.txt", std::ios::trunc);

class LightMap {
private:
	struct Light {
		uint8_t r{0};
		uint8_t g{0};
		uint8_t b{0};
		uint8_t a{255};
	};

	struct LightNode {
		int x;
		int y;
	};

	std::vector<std::vector<float>> lightMap;
	std::queue<LightNode> lightBfsQueue;

	void setLight(int x, int y, float light) { lightMap[x][y] = light; }

public:
	LightMap(int width, int height) : lightMap() {
		lightMap.resize(width);
		for (auto& e : lightMap) {
			e.resize(height);
		}
	}

	void SetLightSource(int x, int y, float light) {
		lightMap[x][y] = light;
		lightBfsQueue.emplace(LightNode{x, y});
	}
	float GetLightValue(int x, int y) {
		return lightMap[x][y]; 
	}
	float GetNormalizedLight(int x, int y) {
		return lightMap[x][y] / 255.f;
	}

	void Update() {
		while (!lightBfsQueue.empty()) {
			LightNode& node = lightBfsQueue.front();
			lightBfsQueue.pop();
			float lightLevel = (gen.map[node.y][node.x] != '#') * GetLightValue(node.x, node.y);
			float dropoff = 0.5f;

			if (lightLevel < 5) {
				continue;
			}
			// if (gen.map[node.y][node.x] == '#') continue;

			auto propagate = [&](int offsetx, int offsety) {
				if (node.x < 0 || node.y < 0 || node.x >= lightMap.size() || node.y >= lightMap[0].size()) return;
				if (node.x + offsetx < 0 || node.y + offsety < 0 || node.x + offsetx >= lightMap.size() || node.y + offsety >= lightMap[0].size()) return;
				
				float current = GetLightValue(node.x + offsetx, node.y + offsety); 

				if (current < lightLevel*dropoff) {
					setLight(node.x + offsetx, node.y + offsety, lightLevel * dropoff);

					if (gen.map[node.y][node.x] != '#') 
						lightBfsQueue.emplace(LightNode{node.x + offsetx, node.y + offsety});
				}
			};

			propagate( 1,  0);
			propagate(-1,  0);
			propagate( 0,  1);
			propagate( 0, -1);

			dropoff = pow(dropoff, sqrt(2));
			propagate(-1, -1);
			propagate( 1,  1);
			propagate( 1, -1);
			propagate(-1,  1);

			// for (int x = -1; x <= +1; x++) {
			//	for (int y = -1; y <= +1; y++) {
			//		propagate(x, y);
			//	}	
			// }
		}
	}

};

void applyLightCircular(int x, int y, int lightx = 0, int lighty = 0, float lightLevel = 0, int depth = 0) {
	if (x < 0 || y < 0 || x >= lightMap.size() || y >= lightMap[0].size()) return;
	auto getLightAt = [&](float a, float b) { return (lightLevel ? lightLevel : maxLight) - sqrt(a*a + b*b); };
	float newLight = getLightAt(lightx, lighty);
	if (newLight <= lightMap[x][y]) return;
	
	lightMap[x][y] = newLight;
	
	applyLightCircular(x+1, y, lightx+1, lighty, lightLevel);
	applyLightCircular(x, y+1, lightx, lighty+1, lightLevel);
	applyLightCircular(x-1, y, lightx-1, lighty, lightLevel);
	applyLightCircular(x, y-1, lightx, lighty-1, lightLevel);
}

void removeLightCircular(int x, int y, int lightx = 0, int lighty = 0, float lightLevel = maxLight, int depth = 0) {
	if (x < 0 || y < 0 || x >= lightMap.size() || y >= lightMap[0].size()) return;
	
	float neighbor = lightMap[x][y];
	
	auto getLightAt = [&](float a, float b) { return lightLevel - sqrt(a*a + b*b); };
	float newLight = getLightAt(lightx, lighty);
	// if (newLight < 0) return;

	if (neighbor > newLight || neighbor == 0 || gen.map[y][x] == '#') {
		if (gen.map[y][x] == '#') {
			lightMap[x][y] = 0;
		}
		if (neighbor > newLight) {
			lightMap[x][y] = 0;
			applyLightCircular(x, y, 0, 0, neighbor);
		}
		
		return;
	}

	lightMap[x][y] = 0;

	removeLightCircular(x+1, y, lightx+1, lighty, lightLevel);
	removeLightCircular(x, y+1, lightx, lighty+1, lightLevel);
	removeLightCircular(x-1, y, lightx-1, lighty, lightLevel);
	removeLightCircular(x, y-1, lightx, lighty-1, lightLevel);
}

void applyLight(int x, int y, float lightLevel = maxLight) {
	if (x < 0 || y < 0 || x >= lightMap.size() || y >= lightMap[0].size()) return;
	int newLight = lightLevel - 1;

	if (newLight < lightMap[x][y]) return;
	if (gen.map[y][x] == '#') {
		lightMap[x][y] = newLight;
		return;
	}

	lightMap[x][y] = lightLevel;
	applyLight(x+1, y, newLight);
	applyLight(x, y+1, newLight);
	applyLight(x-1, y, newLight);
	applyLight(x, y-1, newLight);
}

void removeLight(int x, int y, int lightLevel = maxLight) {
	if (x < 0 || y < 0 || x >= lightMap.size() || y >= lightMap[0].size()) return;
	int neighbor = int(lightMap[x][y]);

	if (neighbor > lightLevel || neighbor == 0 || gen.map[y][x] == '#') {
		if (gen.map[y][x] == '#') {
			lightMap[x][y] = 0;
		}
		if (neighbor > lightLevel+1) {
			lightMap[x][y] = 0;
			applyLight(x, y, neighbor);
		}
		return;
	}
	lightMap[x][y] = 0;
	removeLight(x+1, y, lightLevel - 1);
	removeLight(x, y+1, lightLevel - 1);
	removeLight(x-1, y, lightLevel - 1);
	removeLight(x, y-1, lightLevel - 1);
}

#define applyLight applyLightCircular
#define removeLight removeLightCircular

int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_EVERYTHING);

	float zoom{1};
	if(argc >= 3) {
		windowWidth = std::stoi(argv[1]);
		windowHeight = std::stoi(argv[2]);
		if(argc >= 4) {
			pixelSize = std::stoi(argv[3]);
		}
	} else {
		pixelSize = 16;
		windowWidth = 800/pixelSize*pixelSize;
		windowHeight = 600/pixelSize*pixelSize;
	}
	gen = Generation(windowWidth/pixelSize,windowHeight/pixelSize,4,4,4,4);
	LightMap lm(windowWidth/pixelSize, windowHeight/pixelSize);
	
	Display display(windowWidth, windowHeight, "A* Pathfinding", true, false, zoom, zoom);
	Renderer renderer{display.GetPixels(), display.GetCanvasWidth(), display.GetCanvasHeight()};

	auto duration = std::chrono::system_clock::now().time_since_epoch();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	srand(millis);
	gen.Start(6,6,8,6,8); 
	lightMap.resize(gen.WIDTH);
	for (int i = 0; i < gen.WIDTH; i++) {
		lightMap[i].resize(gen.HEIGHT);
	} 
	for(int i = 0; i < gen.WIDTH; i++) {
		for(int j = 0; j < gen.HEIGHT; j++) {
			char& current = gen.map[j][i];
			if (current == '-') {
				current = ' ';
			} else if (current != '#') {
				current = '.';
			}
		}
	}
	for(int x = 0; x < gen.WIDTH; x++) {
		for(int y = 0; y < gen.HEIGHT; y++) {
			char& current = gen.map[y][x];
			if (current == 'P') {
				applyLight(x, y, maxLight);
			}
		}
	}

	bool smooth = true;
	int mouseX = 0;
	int mouseY = 0;
	int current = SDL_GetTicks();
	int old = 0;
	int elapsed = 0;

	while (!display.IsClosed()) {
		elapsed = current - old;
		old = current;
		current = SDL_GetTicks();
		std::cout << "ms: " << elapsed << std::endl;

		if (EventHandler::IsKeyDown(SDL_SCANCODE_C) && EventHandler::IsKeyDown(SDL_SCANCODE_LCTRL)) {
			display.Abort();
		}
		if (EventHandler::IsLeftMouseDown()) {
			int newMouseX = EventHandler::GetMouseX() / pixelSize;
			int newMouseY = EventHandler::GetMouseY() / pixelSize;
			if (mouseX != newMouseX || mouseY != newMouseY) {
				mouseX = newMouseX;
				mouseY = newMouseY;
				lm.SetLightSource(newMouseX, newMouseY, 255);
			}
		}
		if (EventHandler::IsKeyPressed(SDL_SCANCODE_S)) {
			smooth = !smooth;
		}

		renderer.Clear();

		for(int x = 0; x < gen.WIDTH; x++) {
			for(int y = 0; y < gen.HEIGHT; y++) {
				char current = gen.map[y][x];
				if (current == '#') {
					renderer.FillRectangle(RGB::Blue, x * pixelSize, y * pixelSize, pixelSize, pixelSize);
				} else if (current == '.') {
					renderer.FillRectangle(RGB::Green, x * pixelSize, y * pixelSize, pixelSize, pixelSize);
				} else {
					renderer.FillRectangle(RGB::Red, x * pixelSize, y * pixelSize, pixelSize, pixelSize);
				}
			}
		}

		// update light
		lm.Update();

		Bitmap absoluteShadowMask(windowWidth, windowHeight);
		Renderer absoluteShadowMask_rend(absoluteShadowMask.GetData(), absoluteShadowMask.GetWidth(), absoluteShadowMask.GetHeight());

		// Mask
		Bitmap mask_shadow(windowWidth / pixelSize, windowHeight / pixelSize);
		Bitmap mask_empty(windowWidth / pixelSize, windowHeight / pixelSize);
		Bitmap mask_wall(windowWidth / pixelSize, windowHeight / pixelSize);
		for (int x = 0; x < mask_shadow.GetWidth(); x++) {
			for (int y = 0; y < mask_shadow.GetHeight(); y++) {
				float lightVal = lm.GetNormalizedLight(x, y);
				uint8_t colorLightVal = 255*lightVal;//(0.1 < lightVal ? lightVal : 0.1);
				mask_shadow.SetPixel({colorLightVal, colorLightVal, colorLightVal}, x, y);

				if (gen.map[y][x] == '#') {
					mask_wall.SetPixel({colorLightVal, colorLightVal, colorLightVal}, x, y);
				} else {
					mask_empty.SetPixel({colorLightVal, colorLightVal, colorLightVal}, x, y);
				}

				if (lightVal > 0) {
					absoluteShadowMask_rend.FillRectangle(0xffffffff, x * pixelSize, y * pixelSize, pixelSize, pixelSize);
				}
			}
		}

		// Shadow map
		Bitmap shadowMap(windowWidth, windowHeight);
		Renderer shadowMap_rend(shadowMap.GetData(), shadowMap.GetWidth(), shadowMap.GetHeight());
		if (smooth) shadowMap_rend.ScaleType = Renderer::ScaleType::Linear;
		shadowMap_rend.DrawBitmap(mask_shadow, 0, 0, shadowMap.GetWidth(), shadowMap.GetHeight(), 0, 0, mask_shadow.GetWidth(), mask_shadow.GetHeight());

		Bitmap shadowMap_wall(windowWidth, windowHeight);
		Renderer shadowMap_wall_rend(shadowMap_wall.GetData(), shadowMap_wall.GetWidth(), shadowMap_wall.GetHeight());
		if (smooth) shadowMap_wall_rend.ScaleType = Renderer::ScaleType::Linear;
		shadowMap_wall_rend.DrawBitmap(mask_wall, 0, 0, shadowMap_wall.GetWidth(), shadowMap_wall.GetHeight(), 0, 0, mask_shadow.GetWidth(), mask_shadow.GetHeight());
		shadowMap_wall_rend.ApplyMask(absoluteShadowMask);

		Bitmap shadowMap_empty(windowWidth, windowHeight);
		Renderer shadowMap_empty_rend(shadowMap_empty.GetData(), shadowMap_empty.GetWidth(), shadowMap_empty.GetHeight());
		if (smooth) shadowMap_empty_rend.ScaleType = Renderer::ScaleType::Linear;
		shadowMap_empty_rend.DrawBitmap(mask_empty, 0, 0, shadowMap_empty.GetWidth(), shadowMap_empty.GetHeight(), 0, 0, mask_shadow.GetWidth(), mask_shadow.GetHeight());

		// for (int i = 0; i < shadowMap.GetWidth(); i++) {
		// 	for (int j = 0; j < shadowMap.GetHeight(); j++) {
		// 		// std::cout << "i " << i << i << i << '\n';
		// 		// std::cout << "j " << j << '\n';
		// 		// std::cout << "shadowMap_empt.GetRawPixel " << shadowMap_empty.GetRawPixel(i, j) << "\n";
		// 		// std::cout << "shadowMap_empt.GetWidth " << shadowMap_empty.GetWidth() << "\n";
		// 		// std::cout << "shadowMap_empt.GetHeight " << shadowMap_empty.GetHeight() << "\n\n";
		// 		uint8_t col_empty = shadowMap_empty.GetPixel(i, j).r;
		// 		uint8_t col_wall = shadowMap_wall.GetPixel(i, j).r;
		// 		uint8_t col = col_empty > col_wall ? col_empty : col_wall;

		// 		shadowMap.SetPixel({col, col, col}, i, j);

		// 		// shadowMap.SetRawPixel(
		// 		// 		shadowMap_wall.GetRawPixel(i, j) | 
		// 		// 		shadowMap_empty.GetRawPixel(i, j),
		// 		// 		i, j);
		// 	}
		// }

		// renderer.DrawBitmap(shadowMap, 0, 0, renderer.GetWidth(), renderer.GetHeight(), 0, 0, shadowMap.GetWidth(), shadowMap.GetHeight());

		shadowMap_rend.ApplyMask(absoluteShadowMask);

		for(int x = 0; x < gen.WIDTH; x++) {
			for(int y = 0; y < gen.HEIGHT; y++) {
				char current = gen.map[y][x];
				RGB color;
				if (current == '#') {
					renderer.DrawPixel(RGB::Blue, x * pixelSize + pixelSize/2, y * pixelSize + pixelSize/2);
					color = RGB::Blue;
				} else {
					color = RGB::White;
				}
				renderer.FillRectangle(color, x * pixelSize, y * pixelSize, pixelSize, pixelSize);
			}
		}

		renderer.ApplyMask(shadowMap);

		for(int x = 0; x < gen.WIDTH; x++) {
			for(int y = 0; y < gen.HEIGHT; y++) {
				char current = gen.map[y][x];
				if (current == '#') {
					renderer.DrawPixel(RGB::Blue/2, x * pixelSize + pixelSize/2, y * pixelSize + pixelSize/2);
				} 
			}
		}

		display.Update();
	}

	SDL_Quit();
	return 0;
}

// build and run - keyboard shortcut
// :nnoremap <nowait><leader>b :wa<CR>:make -C build<CR>:!build/basicLighting<CR>

