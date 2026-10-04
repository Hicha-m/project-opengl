#include "platform/Window.h"
#include "platform/OpenGL.h"
#include <iostream>
#ifdef PROJECT_MOBILE
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <vector>
#else
#include <GLFW/glfw3.h>
#endif

namespace WindowSystem {
struct Window {
#ifdef PROJECT_MOBILE
    SDL_Window* native = nullptr;
    SDL_GLContext context = nullptr;
    bool closed = false;
    std::atomic<bool> background{false}, resumed{false};
    std::atomic<unsigned> audioDevice{0};
    double mouseX = 0, mouseY = 0;
    GLuint controlsProgram = 0, controlsVAO = 0, controlsVBO = 0;
#else
    GLFWwindow* native = nullptr;
#endif
    void* user = nullptr;
    KeyCallback keys = nullptr;
    FramebufferCallback resize = nullptr;
};
#ifdef PROJECT_MOBILE
namespace {
Window* active = nullptr;
int translate(SDL_Keycode code) {
    switch (code) {
        case SDLK_ESCAPE: return Escape;
        case SDLK_RIGHT: return Right; case SDLK_LEFT: return Left;
        case SDLK_UP: return Up; case SDLK_DOWN: return Down;
        case SDLK_F1: return F1; case SDLK_F2: return F2; case SDLK_F3: return F3;
        case SDLK_KP_0: return KeypadZero;
        case SDLK_KP_PLUS: return KeypadAdd; case SDLK_KP_MINUS: return KeypadSubtract;
        default: return code >= SDLK_A && code <= SDLK_Z ? int(code - SDLK_A + A) : int(code);
    }
}
}
#endif
Window* create(int width, int height, const char* title, bool visible, bool fullscreen, bool software) {
#ifdef PROJECT_MOBILE
    (void)software;
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL window initialization failed: " << SDL_GetError() << '\n';
        return nullptr;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    auto* window = new Window;
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (!visible) flags |= SDL_WINDOW_HIDDEN;
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    flags |= SDL_WINDOW_FULLSCREEN;
#else
    if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
#endif
    window->native = SDL_CreateWindow(title, width, height, flags);
    if (window->native) window->context = SDL_GL_CreateContext(window->native);
    if (!window->native || !window->context) {
        std::cerr << "OpenGL ES 3.0 context unavailable: " << SDL_GetError() << '\n';
        destroy(window); return nullptr;
    }
    active = window;
    SDL_SetEventFilter([](void* user, SDL_Event* event) -> bool {
        auto* window = static_cast<Window*>(user);
        if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND) {
            window->background = true;
            if (const auto device = window->audioDevice.load()) SDL_PauseAudioDevice(device);
        } else if (event->type == SDL_EVENT_DID_ENTER_FOREGROUND) {
            window->background = false;
            window->resumed = true;
        }
        return true;
    }, window);
    SDL_GL_MakeCurrent(window->native, window->context);
    SDL_GL_SetSwapInterval(1);
    return window;
#else
    glfwSetErrorCallback([](int code, const char* message) {
        std::cerr << "GLFW error " << code << ": " << message << '\n';
    });
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4)
    glfwInitHint(GLFW_PLATFORM, software ? GLFW_PLATFORM_NULL : GLFW_ANY_PLATFORM);
#endif
    bool initialized = glfwInit() == GLFW_TRUE;
#if defined(__linux__) && (GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4))
    if (!initialized && !software && glfwPlatformSupported(GLFW_PLATFORM_X11)) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11); initialized = glfwInit() == GLFW_TRUE;
    }
#endif
    if (!initialized) return nullptr;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, software ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);
    if (software) glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_OSMESA_CONTEXT_API);
    auto* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    if (monitor) if (const auto* mode = glfwGetVideoMode(monitor)) { width = mode->width; height = mode->height; }
    auto* native = glfwCreateWindow(width, height, title, monitor, nullptr);
    if (!native) { glfwTerminate(); return nullptr; }
    auto* window = new Window; window->native = native;
    glfwSetWindowUserPointer(native, window);
    glfwSetKeyCallback(native, [](GLFWwindow* source, int key, int scancode, int action, int modifiers) {
        auto* wrapper = static_cast<Window*>(glfwGetWindowUserPointer(source));
        if (wrapper->keys) wrapper->keys(wrapper, key, scancode, action, modifiers);
    });
    glfwSetFramebufferSizeCallback(native, [](GLFWwindow* source, int width, int height) {
        auto* wrapper = static_cast<Window*>(glfwGetWindowUserPointer(source));
        if (wrapper->resize) wrapper->resize(wrapper, width, height);
    });
    glfwMakeContextCurrent(native);
    glfwSetInputMode(native, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    return window;
#endif
}
void destroy(Window* window) {
    if (!window) return;
#ifdef PROJECT_MOBILE
    SDL_SetEventFilter(nullptr, nullptr);
    if (window->controlsProgram) glDeleteProgram(window->controlsProgram);
    if (window->controlsVAO) glDeleteVertexArrays(1, &window->controlsVAO);
    if (window->controlsVBO) glDeleteBuffers(1, &window->controlsVBO);
    if (window->context) SDL_GL_DestroyContext(window->context);
    if (window->native) SDL_DestroyWindow(window->native);
    if (active == window) active = nullptr;
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
#else
    glfwDestroyWindow(window->native); glfwTerminate();
#endif
    delete window;
}
void bind(Window* window) {
#ifdef PROJECT_MOBILE
    SDL_GL_MakeCurrent(window->native, window->context);
#else
    glfwMakeContextCurrent(window->native);
#endif
}
bool loadGraphics() {
#ifdef PROJECT_MOBILE
    return gladLoadGLES2(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)) && GLAD_GL_ES_VERSION_3_0;
#else
    return gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) && GLAD_GL_VERSION_3_3;
#endif
}
void setUserPointer(Window* window, void* user) { window->user = user; }
void* userPointer(Window* window) { return window->user; }
void setKeyCallback(Window* window, KeyCallback callback) { window->keys = callback; }
void setFramebufferCallback(Window* window, FramebufferCallback callback) { window->resize = callback; }
void size(Window* window, int* width, int* height) {
#ifdef PROJECT_MOBILE
    SDL_GetWindowSize(window->native, width, height);
#else
    glfwGetWindowSize(window->native, width, height);
#endif
}
void framebufferSize(Window* window, int* width, int* height) {
#ifdef PROJECT_MOBILE
    SDL_GetWindowSizeInPixels(window->native, width, height);
#else
    glfwGetFramebufferSize(window->native, width, height);
#endif
}
void cursor(Window* window, double* x, double* y) {
#ifdef PROJECT_MOBILE
    *x = window->mouseX; *y = window->mouseY;
#else
    glfwGetCursorPos(window->native, x, y);
#endif
}
void setCursor(Window* window, double x, double y) {
#ifdef PROJECT_MOBILE
    window->mouseX = x; window->mouseY = y;
#else
    glfwSetCursorPos(window->native, x, y);
#endif
}
int key(Window* window, int value) {
#ifdef PROJECT_MOBILE
    (void)window;
    SDL_Keycode code = value >= A && value <= Z ? SDL_Keycode(value - A + SDLK_A) : SDL_Keycode(value);
    return SDL_GetKeyboardState(nullptr)[SDL_GetScancodeFromKey(code, nullptr)] ? Press : 0;
#else
    return glfwGetKey(window->native, value);
#endif
}
void pollEvents() {
#ifdef PROJECT_MOBILE
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (!active) continue;
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) active->closed = true;
        else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            if (active->keys && !event.key.repeat) active->keys(active, translate(event.key.key), int(event.key.scancode), event.key.down ? Press : 0, int(event.key.mod));
        } else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            if (active->resize) active->resize(active, event.window.data1, event.window.data2);
        } else if (event.type == SDL_EVENT_RENDER_DEVICE_RESET) {
            std::cerr << "Graphics context lost; restart the application\n"; active->closed = true;
        } else if (event.type == SDL_EVENT_FINGER_DOWN && event.tfinger.y > 0.86f) {
            const int buttons[] = {Space, R, Left, Right, M};
            const int button = std::clamp(int(event.tfinger.x * 5), 0, 4);
            if (active->keys) active->keys(active, buttons[button], 0, Press, 0);
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.which != SDL_TOUCH_MOUSEID) {
            int width, height; size(active, &width, &height);
            if(event.button.y > height*0.86f && width>0) {
                const int buttons[] = {Space,R,Left,Right,M};
                if(active->keys) active->keys(active,buttons[std::clamp(int(event.button.x/width*5),0,4)],0,Press,0);
            }
        } else if (event.type == SDL_EVENT_FINGER_MOTION && event.tfinger.y <= 0.86f) {
            int width, height; size(active, &width, &height);
            active->mouseX += event.tfinger.dx * width; active->mouseY += event.tfinger.dy * height;
        }
    }
#else
    glfwPollEvents();
#endif
}
bool shouldClose(Window* window) {
#ifdef PROJECT_MOBILE
    return window->closed;
#else
    return glfwWindowShouldClose(window->native);
#endif
}
void close(Window* window) {
#ifdef PROJECT_MOBILE
    window->closed = true;
#else
    glfwSetWindowShouldClose(window->native, GLFW_TRUE);
#endif
}
void swap(Window* window) {
#ifdef PROJECT_MOBILE
    SDL_GL_SwapWindow(window->native);
#else
    glfwSwapBuffers(window->native);
#endif
}
void setTitle(Window* window, const char* title) {
#ifdef PROJECT_MOBILE
    SDL_SetWindowTitle(window->native, title);
#else
    glfwSetWindowTitle(window->native, title);
#endif
}
double time() {
#ifdef PROJECT_MOBILE
    return double(SDL_GetTicksNS()) / 1000000000.0;
#else
    return glfwGetTime();
#endif
}
bool suspended(Window* window) {
#ifdef PROJECT_MOBILE
    return window->background;
#else
    (void)window; return false;
#endif
}
bool consumeResume(Window* window) {
#ifdef PROJECT_MOBILE
    return window->resumed.exchange(false);
#else
    (void)window; return false;
#endif
}
void setAudioDevice(Window* window, unsigned device) {
#ifdef PROJECT_MOBILE
    if (window) window->audioDevice = device;
#else
    (void)window; (void)device;
#endif
}
void wait() {
#ifdef PROJECT_MOBILE
    SDL_Delay(50);
#endif
}
void drawControls(Window* window, bool paused, bool muted) {
#ifdef PROJECT_MOBILE
    if (!window->controlsProgram) {
        const char* vertexSource = "#version 300 es\nlayout(location=0) in vec2 position; layout(location=1) in vec4 color; out vec4 tint; void main(){tint=color; gl_Position=vec4(position,0,1);}";
        const char* fragmentSource = "#version 300 es\nprecision highp float; in vec4 tint; out vec4 color; void main(){color=tint;}";
        GLuint vertex = glCreateShader(GL_VERTEX_SHADER), fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(vertex, 1, &vertexSource, nullptr); glCompileShader(vertex);
        glShaderSource(fragment, 1, &fragmentSource, nullptr); glCompileShader(fragment);
        window->controlsProgram = glCreateProgram();
        glAttachShader(window->controlsProgram, vertex); glAttachShader(window->controlsProgram, fragment);
        glLinkProgram(window->controlsProgram); glDeleteShader(vertex); glDeleteShader(fragment);
        glGenVertexArrays(1, &window->controlsVAO); glGenBuffers(1, &window->controlsVBO);
    }
    int width, height; framebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) return;
    struct Vertex { float x, y, r, g, b, a; };
    std::vector<Vertex> vertices;
    auto rectangle = [&](float x, float y, float w, float h, std::array<float,4> color) {
        auto point = [&](float px, float py) { return Vertex{2*px/width-1, 2*py/height-1, color[0],color[1],color[2],color[3]}; };
        const Vertex corners[] = {point(x,y), point(x+w,y), point(x+w,y+h), point(x,y+h)};
        for (int index : {0,1,2,0,2,3}) vertices.push_back(corners[index]);
    };
    auto glyph = [](char character)->std::array<unsigned char,7> {
        switch(character) {
            case 'A': return {14,17,17,31,17,17,17}; case 'E': return {31,16,16,30,16,16,31};
            case 'L': return {16,16,16,16,16,16,31}; case 'M': return {17,27,21,21,17,17,17};
            case 'P': return {30,17,17,30,16,16,16}; case 'R': return {30,17,17,30,20,18,17};
            case 'S': return {15,16,16,14,1,1,30}; case 'T': return {31,4,4,4,4,4,4};
            case 'U': return {17,17,17,17,17,17,14}; case 'Y': return {17,17,10,4,4,4,4};
            case '0': return {14,17,19,21,25,17,14}; case '1': return {4,12,4,4,4,4,14};
            case '-': return {0,0,0,31,0,0,0}; case '+': return {0,4,4,31,4,4,0};
            default: return {};
        }
    };
    const char* labels[] = {paused ? "PLAY" : "PAUSE", "RESTART", "-10", "+10", "MUTE"};
    const float cell = width/5.0f, bar = height*0.14f;
    const float scale = std::max(1.0f, std::min(cell*0.78f/42, bar*0.45f/7));
    for (int i=0; i<5; ++i) {
        rectangle(i*cell+2, 0, cell-4, bar, {0.02f,0.03f,0.06f,0.92f});
        const std::string label=labels[i];
        const float x = i*cell + (cell-label.size()*6*scale)/2, y = (bar-7*scale)/2;
        const std::array<float,4> tint = (i==4 && muted) ? std::array<float,4>{1,0.68f,0.22f,1} : std::array<float,4>{0.88f,0.91f,1,1};
        for (std::size_t c=0;c<label.size();++c) {
            const auto rows=glyph(label[c]);
            for(int row=0;row<7;++row) for(int column=0;column<5;++column)
                if(rows[row] & (1 << (4-column))) rectangle(x+(c*6+column)*scale,y+(6-row)*scale,scale,scale,tint);
        }
    }
    GLint program, vao, buffer, sourceRGB, destinationRGB, sourceAlpha, destinationAlpha;
    glGetIntegerv(GL_CURRENT_PROGRAM,&program); glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao); glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);
    glGetIntegerv(GL_BLEND_SRC_RGB,&sourceRGB); glGetIntegerv(GL_BLEND_DST_RGB,&destinationRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,&sourceAlpha); glGetIntegerv(GL_BLEND_DST_ALPHA,&destinationAlpha);
    const bool depth=glIsEnabled(GL_DEPTH_TEST), blend=glIsEnabled(GL_BLEND), cull=glIsEnabled(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(window->controlsProgram); glBindVertexArray(window->controlsVAO); glBindBuffer(GL_ARRAY_BUFFER,window->controlsVBO);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(2*sizeof(float))); glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLES,0,int(vertices.size()));
    glBindBuffer(GL_ARRAY_BUFFER,buffer); glBindVertexArray(vao); glUseProgram(program);
    glBlendFuncSeparate(sourceRGB,destinationRGB,sourceAlpha,destinationAlpha);
    if (depth) glEnable(GL_DEPTH_TEST);
    if (cull) glEnable(GL_CULL_FACE);
    if (!blend) glDisable(GL_BLEND);
#else
    (void)window; (void)paused; (void)muted;
#endif
}
}
