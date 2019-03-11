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

  // Use this window for displaying OpenGL graphics on this thread
  // We're not going to be bouncing back and forth among threads, so set and forget
  glfwMakeContextCurrent(window);

  // Load OpenGL functions
  if (!gladLoadGL()) {
    fprintf(stderr, "error: failed to load gl\n");
    return 1;
  }

  // Window loop
  while (true) {
    // Close window if requested
    if (glfwWindowShouldClose(window))
      break;

    // Handle window events
    glfwPollEvents();

    // Clear the screen
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Move current graphics to the window
    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return 0;
}
