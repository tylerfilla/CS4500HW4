/*
 * Tyler Filla
 * Due 11 March 2019 <-- LOOK AT THIS TYLER <--- DON'T LOSE SIGHT OF THIS !!!!!!! TODO
 *
 * Ideas for Opening Comment
 * - I decided to approach this program like any multimedia application I've before (with a framebuffer window library like GLFW and a graphics API like OpenGL)
 */

#include <stdbool.h>
#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576
#define WINDOW_TITLE "Circles and Arrows IV: A New Hope"

int main() {
  // Initialize GLFW library
  if (!glfwInit()) {
    fprintf(stderr, "error: failed to init glfw\n");
    return 1;
  }

  // Request OpenGL 3.2 core profile
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create the window
  GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);

  if (!window) {
    fprintf(stderr, "error: failed to create window\n");
    return 1;
  }

  // Use this window for displaying OpenGL graphics on this thread
  // We're not going to be bouncing back and forth among threads, so set and forget
  glfwMakeContextCurrent(window);

  // Load OpenGL functions
  if (!gladLoadGL()) {
    fprintf(stderr, "error: failed to load gl\n");
    return 1;
  }

  // Create NanoVG context
  NVGcontext* ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

  // Window loop
  while (true) {
    // Close window if requested
    if (glfwWindowShouldClose(window))
      break;

    // Handle window events
    glfwPollEvents();

    // Clear the screen to black
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Get window/framebuffer metrix FIXME
    int winWidth;
    int winHeight;
    int fbWidth;
    int fbHeight;
    glfwGetWindowSize(window, &winWidth, &winHeight);
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    float ratio = fbWidth / (float) winWidth;
    printf("%f\n", ratio);

    // FIXME: Do actual drawing
    nvgBeginFrame(ctx, fbWidth, fbHeight, ratio);
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, 0, 0, 20, 20, 5);
    nvgFillColor(ctx, nvgRGBA(255, 255, 255, 255));
    nvgFill(ctx);
    nvgEndFrame(ctx);

    // Move current graphics to the window
    glfwSwapBuffers(window);
  }

  nvgDeleteGL3(ctx);
  glfwTerminate();
  return 0;
}
