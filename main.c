/*
 * Tyler Filla
 * Due 11 March 2019 <-- LOOK AT THIS TYLER <--- DON'T LOSE SIGHT OF THIS !!!!!!! TODO
 *
 * Ideas for Opening Comment
 * - I decided to approach this program like any multimedia application I've before (with a framebuffer window library like GLFW and a graphics API like OpenGL).
 * - I took this assignment as an opportunity to practice physics-based animation.
 * - The program is written in C99 with the glad, GLFW, and NanoVG libraries and the platform OpenGL implementation thing.
 */

#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>

/** Initial width of the window. */
static const int WINDOW_INIT_WIDTH = 1024;

/** Initial height of the window. */
static const int WINDOW_INIT_HEIGHT = 576;

/** Title of the window. */
static const char* WINDOW_TITLE = "Circles and Arrows IV: A New Hope";

/** The GLFW window instance. */
static GLFWwindow* window;

/** The NanoVG drawing context. */
static NVGcontext* vg;

/**
 * Initialize for rendering.
 */
static void renderInit() {
  // Try to create a NanoVG drawing context
  // This library provides a canvas-style interface on top of OpenGL
  vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  if (!vg) {
    fprintf(stderr, "error: failed to create NanoVG context\n");
    exit(1);
  }

  // Set OpenGL clear color to black
  glClearColor(0, 0, 0, 0);
}

/**
 * Render the next frame.
 */
static void renderFrame() {
  // Since the window is resizable, query the window dimensions prior to rendering
  // This is not the most efficient way to accomplish this (we could listen for a resize event)
  int windowWidth;
  int windowHeight;
  glfwGetWindowSize(window, &windowWidth, &windowHeight);

  // Query the framebuffer dimensions, as well
  // The framebuffer is part of the OpenGL context on this thread
  // It might actually have a different pixel count from the window due to DPI scaling
  int fbWidth;
  int fbHeight;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

  // Clear the color buffer
  // This erases the previous frame's rendered content
  glClear(GL_COLOR_BUFFER_BIT);

  // Begin a new NanoVG frame
  nvgBeginFrame(vg, windowWidth, windowHeight, fbWidth / (float) windowWidth);

  nvgBeginPath(vg);
  nvgRoundedRect(vg, 0, 0, 50, 50, 10);
  nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
  nvgFill(vg);

  // End the NanoVG frame
  nvgEndFrame(vg);
}

int main() {
  // Try to initialize the GLFW library
  if (!glfwInit()) {
    fprintf(stderr, "error: failed to init GLFW\n");
    return 1;
  }

  // Ask GLFW to ask the platform for an OpenGL 3.2 core profile context
  // This is the first version of OpenGL that made the core/compat distinction
  // All modern desktop graphics drivers will support this version of OpenGL
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Try to create the window with the initial dimensions and title hardcoded above
  window = glfwCreateWindow(WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT, WINDOW_TITLE, NULL, NULL);
  if (!window) {
    fprintf(stderr, "error: failed to create window\n");
    return 1;
  }

  // The new window has an OpenGL context associated with it for rendering
  // Make this context, via the window object, current on this thread
  // All future calls to OpenGL functions on this thread will affect this window
  glfwMakeContextCurrent(window);

  // Load OpenGL functions on the current thread
  // This uses the glad library which pulls function pointers dynamically from libGL and friends
  if (!gladLoadGL()) {
    fprintf(stderr, "error: failed to load OpenGL functions\n");
    return 1;
  }

  // Initialize rendering state
  renderInit();

  // Window event loop
  while (1) {
    // Poll for user input events
    glfwPollEvents();

    // Break if window closure requested (e.g. via close button)
    // This causes the event loop to exit, and the window will close on program end
    if (glfwWindowShouldClose(window))
      break;

    // Render the next frame
    renderFrame();

    // Swap the next frame in
    glfwSwapBuffers(window);
  }
}
