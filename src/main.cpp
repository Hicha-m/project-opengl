//-----------------------------------------------------------------------------
// This code demonstrates how to implement the Blinn-Phong lighting model for
// multiple light sources using OpenGL 3.3 or a forward compatible
// OpenGL 3.0 rendering context. 
//-----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <string>
#define GLEW_STATIC
#include "GL/glew.h"	// Important - this header must come before glfw3 header
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "ShaderProgram.h"
#include "Texture2D.h"
#include "Camera.h"
#include "Mesh.h"

#include "Sphere.h"
#include "scene/Transform.h"
#include "scene/SceneObject.h"
#include "scene/Scene.h"
#include "graphics/Renderer.h"


// Global Variables
const char* APP_TITLE = "Project - Space";
int gWindowWidth = 1024;
int gWindowHeight = 768;
GLFWwindow* gWindow = NULL;
bool gWireframe = false;
bool gFlashlightOn = true;
bool gCameraDebug = false;
bool fullscreen = true;
glm::vec4 gClearColor(0.06f, 0.06f, 0.07f, 1.0f);

FPSCamera fpsCamera(glm::vec3(0.0f, 3.5f, 10.0f), glm::radians(110.0f), glm::radians(55.0f));
const double ZOOM_SENSITIVITY = -3.0;
float MOVE_SPEED = 5.0; // units per second
const float MOUSE_SENSITIVITY = 0.1f;
const float MAX_DISTANCE = 100000000.0f;


// Function prototypes
void glfw_onKey(GLFWwindow* window, int key, int scancode, int action, int mode);
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height);
void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY);
void update(double elapsedTime);
void showFPS(GLFWwindow* window);
bool initOpenGL();

//-----------------------------------------------------------------------------
// Main Application Entry Point
//-----------------------------------------------------------------------------
int main()
{
	if (!initOpenGL())
	{
		// An error occured
		std::cerr << "GLFW initialization failed" << std::endl;
		return -1;
	}

	ShaderProgram sunShader;
	sunShader.loadShaders("shaders/sun.vert", "shaders/sun.frag");
	
	ShaderProgram earthShader;
	earthShader.loadShaders("shaders/earth.vert","shaders/earth.frag");

	ShaderProgram cloudShader;
	cloudShader.loadShaders("shaders/clouds.vert","shaders/clouds.frag");

	ShaderProgram starShader;
	starShader.loadShaders("shaders/stars.vert","shaders/stars.frag");


	Texture2D earthDayTexture, earthNightTexture, earthSpecularTexture,earthNormalTexture ,earthCloudsTexture;
	earthDayTexture.loadTexture("textures/earth/2k_earth_daymap.jpg", true);
	earthNightTexture.loadTexture("textures/earth/2k_earth_nightmap.jpg", true);
	earthCloudsTexture.loadTexture("textures/earth/2k_earth_clouds.jpg",true);
	earthSpecularTexture.loadTexture("textures/earth/2k_earth_specular_map.png", true);
	earthNormalTexture.loadTexture("textures/earth/2k_earth_normal_map.png", true);
	
	Texture2D sunTexture;
	sunTexture.loadTexture("textures/sun/2k_sun.jpg", true);

	Texture2D starTexture;
	starTexture.loadTexture("textures/space/2k_stars.jpg", true);


	Sphere earth(1.0f,32,32);
	Mesh* earthMesh = &earth.getMesh();
		
	Sphere sun(1.0f, 32, 32);
	Mesh* sunMesh = &sun.getMesh();

	Sphere stars(1.0f, 32, 32);
	Mesh* starMesh = &stars.getMesh();

	Scene scene;
	Renderer renderer;
	LightManager lightManager;

	SceneObject earthObject("Earth",earthMesh,&earthShader);
	earthObject.transform.position = glm::vec3(30.0f, 50.0f, 0.0f);
	earthObject.transform.scale = glm::vec3(10.0f);
	earthObject.material.addTexture("dayMap",&earthDayTexture,0);
	earthObject.material.addTexture("nightMap",&earthNightTexture,1);
	earthObject.material.addTexture("specularMap",&earthSpecularTexture,2);
	earthObject.material.addTexture("normalMap",&earthNormalTexture,3);
	earthObject.material.receivesLighting = true;
	scene.addObject(earthObject);


	SceneObject cloudObject("EarthClouds",earthMesh,&cloudShader);
	cloudObject.transform.position = glm::vec3(30.0f, 50.0f, 0.0f);
	cloudObject.transform.scale = glm::vec3(10.1f);
	cloudObject.material.addTexture("cloudMap",&earthCloudsTexture,0);
	cloudObject.material.blending = true;
	cloudObject.material.receivesLighting = true;
	scene.addObject(cloudObject);


	SceneObject sunObject("Sun",sunMesh,&sunShader);

	sunObject.transform.position = glm::vec3(100.0f, 200.0f, 0.0f);
	sunObject.transform.scale = glm::vec3(50.0f);
	sunObject.material.addTexture("sunMap",&sunTexture,0);
	sunObject.material.receivesLighting = false;
	scene.addObject(sunObject);



	SceneObject starObject("Stars",starMesh,&starShader);

	starObject.transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
	starObject.transform.scale = glm::vec3(500.0f);
	starObject.material.addTexture("starMap",&starTexture,0);
	starObject.material.depthLEqual = true;
	starObject.material.depthWrite = false;
	scene.addObject(starObject);

	DirectionalLight sunLight;

	sunLight.direction = glm::normalize(earthObject.transform.position - sunObject.transform.position);
	sunLight.color = glm::vec3(1.0f,0.95f,0.8f);
	sunLight.intensity =1.0f;


	// Set the directional light
	lightManager.setDirectionalLight(sunLight);


	float earthRotation = 0.0f;
	float cloudRotation = 0.0f;


	double lastTime = glfwGetTime();

	// Rendering loop
	while (!glfwWindowShouldClose(gWindow))
	{
		showFPS(gWindow);

		double currentTime = glfwGetTime();
		double deltaTime = currentTime - lastTime;


		earthRotation += deltaTime  * 100;
		cloudRotation += deltaTime * 0.05f * 100;

		// Poll for and process events
		glfwPollEvents();
		update(deltaTime);

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 model(1.0), view(1.0), projection(1.0);

		// Create the View matrix
		view = fpsCamera.getViewMatrix();

		// Create the projection matrix
		projection = glm::perspective(glm::radians(fpsCamera.getFOV()), (float)gWindowWidth / (float)gWindowHeight, 0.1f, MAX_DISTANCE);

		// update the view (camera) position
		glm::vec3 viewPos = fpsCamera.getPosition();



		// --------------------------------------------------------
        // UPDATE OBJECT TRANSFORMS
        // --------------------------------------------------------

        SceneObject* earth = scene.findObject("Earth");
        if (earth)
        {
            earth->transform.rotation = glm::vec3(0.0f,glm::radians(earthRotation),0.0f);
        }
        SceneObject* clouds = scene.findObject("EarthClouds");
        if (clouds)
        {
            clouds->transform.rotation = glm::vec3(0.0f,glm::radians(cloudRotation),0.0f);
        }


        // --------------------------------------------------------
        // STARS FOLLOW CAMERA
        // --------------------------------------------------------

        SceneObject* starsObject = scene.findObject("Stars");
        if (starsObject)
        {
            starsObject->transform.position = viewPos;
        }

		// render
		
		renderer.render(
			scene,
			lightManager,
			view,
			projection,
			viewPos
		);


		// Swap front and back buffers
		glfwSwapBuffers(gWindow);

		lastTime = currentTime;
	}

	glfwTerminate();

	return 0;
}

//-----------------------------------------------------------------------------
// Initialize GLFW and OpenGL
//-----------------------------------------------------------------------------
bool initOpenGL()
{
	glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
	// Intialize GLFW 
	// GLFW is configured.  Must be called before calling any GLFW functions
	if (!glfwInit())
	{
		// An error occured
		std::cerr << "GLFW initialization failed" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	// forward compatible with newer versions of OpenGL as they become available but not backward compatible (it will not run on devices that do not support OpenGL 3.3
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	
	if (fullscreen) {
        GLFWmonitor* pMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* pVmode = glfwGetVideoMode(pMonitor);
        if (pVmode != NULL){
            gWindow = glfwCreateWindow(pVmode->width, pVmode->height, APP_TITLE, pMonitor, NULL);
        }
    } else {
        gWindow = glfwCreateWindow(gWindowWidth, gWindowHeight, APP_TITLE, NULL, NULL);
    }

	if (gWindow == NULL)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	// Make the window's context the current one
	glfwMakeContextCurrent(gWindow);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		return false;
	}

	// Set the required callback functions
	glfwSetKeyCallback(gWindow, glfw_onKey);
	glfwSetFramebufferSizeCallback(gWindow, glfw_onFramebufferSize);
	glfwSetScrollCallback(gWindow, glfw_onMouseScroll);

	// Hides and grabs cursor, unlimited movement
	glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(gWindow, gWindowWidth / 2.0, gWindowHeight / 2.0);

	glClearColor(gClearColor.r, gClearColor.g, gClearColor.b, gClearColor.a);

    // Define the viewport dimensions
    int w, h;
    glfwGetFramebufferSize( gWindow, &w, &h); // For retina display
    glViewport(0, 0, w, h);
    
    //    glViewport(0, 0, gWindowWidth, gWindowHeight);

    glEnable(GL_DEPTH_TEST);

	return true;
}

//-----------------------------------------------------------------------------
// Is called whenever a key is pressed/released via GLFW
//-----------------------------------------------------------------------------
void glfw_onKey(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	
	if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
	{
		gWireframe = !gWireframe;
		if (gWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		// toggle the flashlight
		gFlashlightOn = !gFlashlightOn;
	}

	if (key == GLFW_KEY_G && action == GLFW_PRESS)
	{
		// toggle the flashlight
		MOVE_SPEED *= 2.0f;
	}
	if (key == GLFW_KEY_H && action == GLFW_PRESS)
	{
		// toggle the flashlight
		MOVE_SPEED /= 2.0f;
	}

	if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
		gCameraDebug = !gCameraDebug;
}

//-----------------------------------------------------------------------------
// Is called when the window is resized
//-----------------------------------------------------------------------------
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height)
{
	gWindowWidth = width;
	gWindowHeight = height;

    // Define the viewport dimensions
    int w, h;
    glfwGetFramebufferSize( gWindow, &w, &h); // For retina display
    glViewport(0, 0, w, h);
    
    //    glViewport(0, 0, gWindowWidth, gWindowHeight);

}

//-----------------------------------------------------------------------------
// Called by GLFW when the mouse wheel is rotated
//-----------------------------------------------------------------------------
void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY)
{
	double fov = fpsCamera.getFOV() + deltaY * ZOOM_SENSITIVITY;

	fov = glm::clamp(fov, 1.0, 120.0);

	fpsCamera.setFOV((float)fov);
}

//-----------------------------------------------------------------------------
// Update stuff every frame
//-----------------------------------------------------------------------------
void update(double elapsedTime)
{
	// Camera orientation
	double mouseX, mouseY;

	// Get the current mouse cursor position delta
	glfwGetCursorPos(gWindow, &mouseX, &mouseY);

	// Rotate the camera the difference in mouse distance from the center screen.  Multiply this delta by a speed scaler
	fpsCamera.rotate((float)(gWindowWidth / 2.0 - mouseX) * MOUSE_SENSITIVITY, (float)(gWindowHeight / 2.0 - mouseY) * MOUSE_SENSITIVITY);

	// Clamp mouse cursor to center of screen
	glfwSetCursorPos(gWindow, gWindowWidth / 2.0, gWindowHeight / 2.0);

	// Camera FPS movement

	// Forward/backward
	if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getLook());
	else if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getLook());

	// Strafe left/right
	if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getRight());
	else if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getRight());

	// Up/down
	if (glfwGetKey(gWindow, GLFW_KEY_Z) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * glm::vec3(0.0f, 1.0f, 0.0f));
	else if (glfwGetKey(gWindow, GLFW_KEY_X) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -glm::vec3(0.0f, 1.0f, 0.0f));
}

//-----------------------------------------------------------------------------
// Code computes the average frames per second, and also the average time it takes
// to render one frame.  These stats are appended to the window caption bar.
//-----------------------------------------------------------------------------
void showFPS(GLFWwindow* window)
{
	static double previousSeconds = 0.0;
	static int frameCount = 0;
	double elapsedSeconds;
	double currentSeconds = glfwGetTime(); // returns number of seconds since GLFW started, as double float

	elapsedSeconds = currentSeconds - previousSeconds;

	// Limit text updates to 4 times per second
	if (elapsedSeconds > 0.25)
	{
		previousSeconds = currentSeconds;
		double fps = (double)frameCount / elapsedSeconds;
		double msPerFrame = 1000.0 / fps;

		// The C++ way of setting the window title
		std::ostringstream outs;
		outs.precision(3);	// decimal places
		outs << std::fixed
			<< APP_TITLE << "    "
			<< "FPS: " << fps << "    "
			<< "Frame Time: " << msPerFrame << " (ms)";

		if (gCameraDebug)
		{
			outs << "    Cam Pos: (" << fpsCamera.getPosition().x << ", "
				<< fpsCamera.getPosition().y << ", "
				<< fpsCamera.getPosition().z << ")    "
				<< "Yaw: " << fpsCamera.getYaw() << " deg    "
				<< "Pitch: " << fpsCamera.getPitch() << " deg";
		}
		glfwSetWindowTitle(window, outs.str().c_str());

		// Reset for next average.
		frameCount = 0;
	}

	frameCount++;
}
