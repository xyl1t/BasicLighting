#include <ios>
#include <iostream>
#include <map>
#include <thread>
#include <fstream>
#include <sstream>
#include <queue>
#include <cstdlib>

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
public:
	struct Light {
		uint8_t r{0};
		uint8_t g{0};
		uint8_t b{0};
		float dropOff{0.75f};
	};

	struct LightNode {
		int x;
		int y;
		float light;
		float dropOff;
	};

private:

	std::vector<std::vector<Light>> lightMap;
	std::queue<LightNode> lightBfsQueue[3]; // 3 channels -> r g b
	std::queue<LightNode> lightRemovalBfsQueue[3];

	void setLightChannel(int x, int y, uint8_t val, int channel) { 
		if (channel == 0) {
			lightMap[x][y].r = val;
		} else if (channel == 1) {
			lightMap[x][y].g = val;
		} else {
			lightMap[x][y].b = val;
		}
	}
	void setLight(int x, int y, uint8_t r, uint8_t g, uint8_t b, float dropOff) { 
		lightMap[x][y] = {r, g, b, dropOff};
	}
	void setLight(int x, int y, const Light& l) { 
		lightMap[x][y] = l;
	}


public:
	int width;
	int height;
	LightMap(int width, int height) : lightMap(), width(width), height(height) {
		lightMap.resize(width);
		for (auto& e : lightMap) {
			e.resize(height);
		}
	}

	void SetLightSource(int x, int y, uint8_t r, uint8_t g, uint8_t b, float dropOff) {
		lightMap[x][y] = {r, g, b, dropOff};
		lightBfsQueue[0].emplace(LightNode{x, y, (float)r, dropOff});
		lightBfsQueue[1].emplace(LightNode{x, y, (float)g, dropOff});
		lightBfsQueue[2].emplace(LightNode{x, y, (float)b, dropOff});
	}
	void RemoveLightSource(int x, int y) {
		// lightRemovalBfsQueue.emplace(LightNode{x, y, GetLightValue(x, y)});
		lightRemovalBfsQueue[0].emplace(LightNode{x, y, (float)lightMap[x][y].r});
		lightRemovalBfsQueue[1].emplace(LightNode{x, y, (float)lightMap[x][y].g});
		lightRemovalBfsQueue[2].emplace(LightNode{x, y, (float)lightMap[x][y].b});
		lightMap[x][y] = {};
	}
	const Light& GetLightValue(int x, int y) {
		return lightMap[x][y]; 
	}
	float GetLightChannel(int x, int y, int channel) {
		if (channel == 0) {
			return lightMap[x][y].r;
		} else if (channel == 1) {
			return lightMap[x][y].g;
		} else {
			return lightMap[x][y].b;
		}
	}
	// float GetNormalizedLight(int x, int y) {
	// 	return lightMap[x][y] / 255.f;
	// }

	void Update() {
		for (int channel = 0; channel < 3; channel++) {
			while (!lightRemovalBfsQueue[channel].empty()) {
				LightNode& node = lightRemovalBfsQueue[channel].front();
				float currentLightLevel = node.light;
				lightRemovalBfsQueue[channel].pop();


				auto propagate = [&](int offsetx, int offsety) {
					if (node.x < 0 || node.y < 0 || node.x >= lightMap.size() || node.y >= lightMap[0].size()) return;
					if (node.x + offsetx < 0 || node.y + offsety < 0 || node.x + offsetx >= lightMap.size() || node.y + offsety >= lightMap[0].size()) return;

					float neighbor = GetLightChannel(node.x + offsetx, node.y + offsety, channel); 

					if (neighbor != 0 && neighbor < currentLightLevel) {
						setLightChannel(node.x + offsetx, node.y + offsety, 0, channel);
						lightRemovalBfsQueue[channel].emplace(LightNode{node.x + offsetx, node.y + offsety, neighbor});
					} else if (neighbor > currentLightLevel) {
						lightBfsQueue[channel].emplace(LightNode{node.x + offsetx, node.y + offsety});
					}
				};

				propagate( 1,  0);
				propagate(-1,  0);
				propagate( 0,  1);
				propagate( 0, -1);
				propagate(-1, -1);
				propagate( 1,  1);
				propagate( 1, -1);
				propagate(-1,  1);

			}
		}

		for (int channel = 0; channel < 3; channel++) {
			while (!lightBfsQueue[channel].empty()) {
				LightNode& node = lightBfsQueue[channel].front();
				lightBfsQueue[channel].pop();
				float currentLightLevel = (gen.map[node.y][node.x] != '#') * GetLightChannel(node.x, node.y, channel);
				float dropoff = 0.6f; // TODO: this should be part of a light node

				if (currentLightLevel < 10) {
					continue;
				}

				auto propagate = [&](int offsetx, int offsety) {
					if (node.x < 0 || node.y < 0 || node.x >= lightMap.size() || node.y >= lightMap[0].size()) return;
					if (node.x + offsetx < 0 || node.y + offsety < 0 || node.x + offsetx >= lightMap.size() || node.y + offsety >= lightMap[0].size()) return;

					float neighbor = GetLightChannel(node.x + offsetx, node.y + offsety, channel); 

					if (neighbor < currentLightLevel*dropoff) {
						setLightChannel(node.x + offsetx, node.y + offsety, currentLightLevel * dropoff, channel);

						lightBfsQueue[channel].emplace(LightNode{node.x + offsetx, node.y + offsety});
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
	}
};

void applyLightCircular(int x, int y, int lightx = 0, int lighty = 0, float lightLevel = 0, int depth = 0) {
  if (x < 0 || y < 0 || x >= lightMap.size() || y >= lightMap[0].size()) return;
  auto getLightAt = [&](float a, float b) { return (lightLevel ? lightLevel : maxLight) - sqrt(a * a + b * b); };
  float newLight = getLightAt(lightx, lighty);
  if (newLight <= lightMap[x][y]) return;

  lightMap[x][y] = newLight;

  applyLightCircular(x + 1, y, lightx + 1, lighty, lightLevel);
  applyLightCircular(x, y + 1, lightx, lighty + 1, lightLevel);
  applyLightCircular(x - 1, y, lightx - 1, lighty, lightLevel);
  applyLightCircular(x, y - 1, lightx, lighty - 1, lightLevel);
  applyLightCircular(x, y - 1, lightx, lighty - 1, lightLevel);
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
	removeLightCircular(x, y - 1, lightx, lighty - 1, lightLevel);
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
	applyLight(x, y - 1, newLight);
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
	removeLight(x, y - 1, lightLevel - 1);
}

#define applyLight applyLightCircular
#define removeLight removeLightCircular
cdr::RGB getRandColor() {
	uint8_t randNumber = rand() % 128 + 127;
	return RGB(randNumber, randNumber, randNumber);
  return RGB(randNumber, randNumber, randNumber);
}
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

	Display display(windowWidth, windowHeight, "Basic Lighting", true, false, zoom, zoom);
	Renderer renderer{display.GetPixels(), display.GetCanvasWidth(), display.GetCanvasHeight()};

	auto duration = std::chrono::system_clock::now().time_since_epoch();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	srand(millis);
	gen.Start(8,6,8,6,8);
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
		static int color = 8;
		if (EventHandler::IsKeyDown(SDL_SCANCODE_R)) {
			color = 1;
		}
		if (EventHandler::IsKeyDown(SDL_SCANCODE_G)) {
			color = 2;
		}
		if (EventHandler::IsKeyDown(SDL_SCANCODE_B)) {
			color = 4;
		}
		if (EventHandler::IsKeyDown(SDL_SCANCODE_W)) {
			color = 8;
		}
		if (EventHandler::IsLeftMouseDown()) {
			int mx = EventHandler::GetMouseX() / pixelSize;
			int my = EventHandler::GetMouseY() / pixelSize;
			// if (mouseX != newMouseX || mouseY != newMouseY) {
			// 	mouseX = newMouseX;
			// 	mouseY = newMouseY;
			// 	lm.SetLightSource(newMouseX, newMouseY, 255);
			// }
			lm.SetLightSource(mx, my, (color & 1 || color >> 3) * 255, ((color >> 1) & 1 || color >> 3) * 255, ((color >> 2) & 1 || color >> 3) * 255, 0.8);
		}
		if (EventHandler::IsRightMouseDown()) {
			int mx = EventHandler::GetMouseX() / pixelSize;
			int my = EventHandler::GetMouseY() / pixelSize;
			// if (mouseX != newMouseX || mouseY != newMouseY) {
			// 	mouseX = newMouseX;
			// 	mouseY = newMouseY;
			// 	lm.RemoveLightSource(newMouseX, newMouseY);
			// }
			lm.RemoveLightSource(mx, my);
		}
		if (EventHandler::IsKeyPressed(SDL_SCANCODE_S)) {
			smooth = !smooth;
		}

		static float pulse = 0;
		pulse += elapsed/1000.f;
		int clr = (std::sin(pulse)+1)/2.f*255;
		lm.RemoveLightSource(20+4, 20-4);
		lm.SetLightSource(   20+4, 20-4, clr, 0, 0, 0.75f);
		lm.RemoveLightSource(21+4, 21-3);
		lm.SetLightSource(   21+4, 21-3, 0, clr, 0, 0.75f);
		lm.RemoveLightSource(19+4, 21-3);
		lm.SetLightSource(   19+4, 21-3, 0, 0, clr, 0.75f);

		renderer.Clear();

		// for(int x = 0; x < gen.WIDTH; x++) {
		// 	for(int y = 0; y < gen.HEIGHT; y++) {
		// 		char current = gen.map[y][x];
		// 		if (current == '#') {
		// 			renderer.FillRectangle(RGB::Blue, x * pixelSize, y * pixelSize, pixelSize, pixelSize);
		// 		} else if (current == '.') {
		// 			renderer.FillRectangle(RGB::Green, x * pixelSize, y * pixelSize, pixelSize, pixelSize);
		// 		} else {
		// 			renderer.FillRectangle(RGB::Red, x * pixelSize, y * pixelSize, pixelSize, pixelSize);
		// 		}
		// 	}
		// }

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
				// float lightVal = GetNormalizedLight(x, y);
				const LightMap::Light& light = lm.GetLightValue(x, y);
				mask_shadow.SetPixel({light.r, light.g, light.b}, x, y);

				if (gen.map[y][x] == '#') {
					mask_wall.SetPixel({light.r, light.g, light.b}, x, y);
				} else {
					mask_empty.SetPixel({light.r, light.g, light.b}, x, y);
				}

				uint32_t maskR = (light.r > 0)*0xFF000000;
				uint32_t maskG = (light.g > 0)*0x00FF0000;
				uint32_t maskB = (light.b > 0)*0x0000FF00;
				uint32_t mask = maskR + maskB + maskG + 0xff;
				// std::cout << std::hex << mask << '\n';
				absoluteShadowMask_rend.FillRectangle(mask, x * pixelSize, y * pixelSize, pixelSize, pixelSize);
			}
		}

		// Shadow map
		Bitmap shadowMap(windowWidth, windowHeight);
		Renderer shadowMap_rend(shadowMap.GetData(), shadowMap.GetWidth(), shadowMap.GetHeight());
		if (smooth) shadowMap_rend.ScaleType = Renderer::ScaleType::Linear;
		shadowMap_rend.DrawBitmap(mask_shadow, 0, 0, shadowMap.GetWidth(), shadowMap.GetHeight(), 0, 0, mask_shadow.GetWidth(), mask_shadow.GetHeight());

		shadowMap_rend.ApplyMask(absoluteShadowMask);

		// renderer.DrawBitmap(shadowMap, 0, 0, shadowMap.GetWidth(), shadowMap.GetHeight(), 0, 0, shadowMap.GetWidth(), shadowMap.GetHeight());

		for(int x = 0; x < gen.WIDTH; x++) {
			for(int y = 0; y < gen.HEIGHT; y++) {
				char current = gen.map[y][x];
				RGB color;

				for (int px = 0; px < pixelSize; px++) {
					for (int py = 0; py < pixelSize; py++) {
						if (current == '#') {
							color = getRandColor();
						} else {
							color = RGB::White;
						}
						// std::cout << "px: " << px << std::endl;
						// std::cout << "py: " << py << std::endl;
						renderer.DrawPixel(color, x*pixelSize + px, y*pixelSize + py);
					}
				}
				
				// renderer.FillRectangle(color, x * pixelSize, y * pixelSize, pixelSize, pixelSize);
			}
		}

		renderer.ApplyMask(shadowMap);


		for(int x = 0; x < gen.WIDTH; x++) {
			for(int y = 0; y < gen.HEIGHT; y++) {
				char current = gen.map[y][x];
				if (current == '#') {
					renderer.DrawPixel(RGB::Blue, x * pixelSize + pixelSize/2, y * pixelSize + pixelSize/2);
				}
			}
		}


		display.Update();
	}

	SDL_Quit();
	return 0;
}

// :nnoremap <nowait><leader>b :FloatermNew --autoclose=0 --position=topright make -C build && ./build/basicLighting<CR><ESC>
// :nnoremap <nowait><leader>b :wa<CR>:make -C build<CR>:!build/basicLighting<CR>
// :nnoremap <nowait><leader>b :wa<CR>:vs<CR><C-w>l:term<CR>imake -C build/;./build/basicLighting<CR><C-\><C-n>:q<CR>

