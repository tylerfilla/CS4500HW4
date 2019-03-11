/*
 * Tyler Filla
 * Due 11 March 2019 <-- LOOK AT THIS TYLER <--- DON'T LOSE SIGHT OF THIS !!!!!!! TODO
 *
 * Ideas for Opening Comment
 * - I decided to approach this program like any multimedia application I've before (with a framebuffer window library like GLFW and a graphics API like OpenGL)
 */

#include <stdbool.h>
#include <stdio.h>

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

  // Create the window
  GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);

  // Use this window for displaying OpenGL graphics on this thread
  // We're not going to be bouncing back and forth among threads, so set and forget
  glfwMakeContextCurrent(window);

  // Window loop
  while (true) {
    // Close window if requested
    if (glfwWindowShouldClose(window))
      break;

    // Handle window events
    glfwPollEvents();

    // TODO: Draw the picture

    // Move current graphics to the window
    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return 0;
}
