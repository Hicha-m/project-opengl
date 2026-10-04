#pragma once

namespace WindowSystem {
struct Window;
using GLProc = void (*)();
using KeyCallback = void (*)(Window*, int, int, int, int);
using FramebufferCallback = void (*)(Window*, int, int);
enum Key {
    Space = 32, Equal = 61, Minus = 45, Zero = 48,
    A = 65, D = 68, G = 71, H = 72, M = 77, R = 82, S = 83, W = 87, X = 88, Z = 90,
    Escape = 256, Right = 262, Left = 263, Down = 264, Up = 265,
    F1 = 290, F2 = 291, F3 = 292, KeypadZero = 320, KeypadSubtract = 333, KeypadAdd = 334
};
constexpr int Press = 1;
Window* create(int width, int height, const char* title, bool visible, bool fullscreen, bool software);
void destroy(Window*);
void bind(Window*);
bool loadGraphics();
void setUserPointer(Window*, void*);
void* userPointer(Window*);
void setKeyCallback(Window*, KeyCallback);
void setFramebufferCallback(Window*, FramebufferCallback);
void size(Window*, int*, int*);
void framebufferSize(Window*, int*, int*);
void cursor(Window*, double*, double*);
void setCursor(Window*, double, double);
int key(Window*, int);
void pollEvents();
bool shouldClose(Window*);
void close(Window*);
void swap(Window*);
void setTitle(Window*, const char*);
double time();
bool suspended(Window*);
bool consumeResume(Window*);
void setAudioDevice(Window*, unsigned);
void wait();
void drawControls(Window*, bool paused, bool muted);
}
