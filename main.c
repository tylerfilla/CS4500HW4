/*
 * Tyler Filla
 * Due 11 March 2019 <-- LOOK AT THIS TYLER <--- DON'T LOSE SIGHT OF THIS !!!!!!! TODO
 *
 * Ideas for Opening Comment
 * - I decided to approach this program like any multimedia application I've before (with a framebuffer window library like GLFW and a graphics API like OpenGL).
 * - I took this assignment as an opportunity to practice physics-based animation.
 * - The program is written in C99 with the glad, GLFW, and NanoVG libraries and the platform OpenGL implementation thing.
 * - I have no qualms with global variables (mutable, even) in a simple single-threaded program.
 * - I did not try to draw arrowheads on self-referencing arcs.
 */

#include <math.h>
#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>

/** The number of loaded circles. */
static int numCircles;

/** The loaded circles. */
static struct circle** circles;

/** The number of loaded arrows. */
static int numArrows;

/** The loaded arrows. */
static struct arrow** arrows;

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

/** The latest framebuffer width. */
static int fbWidth;

/** The latest framebuffer height. */
static int fbHeight;

/** The latest computed frames per second. */
static double perfFps;

/** The size of the sliding window for performance measurement. */
#define PERF_WINDOW_SIZE 16

/** The perf window. This holds past frame times. */
static double perfWindow[PERF_WINDOW_SIZE];

/** The index of the current frame in the perf window. */
static int perfWindowCurrent;

/**
 * A single circle.
 */
struct circle {
  /** The circle size. Doubles as radius and mass. */
  float size;

  /** The current x-coordinate of the circle. */
  float currentX;

  /** The current y-coordinate of the circle. */
  float currentY;

  /** The last x-coordinate of the circle. */
  float lastX;

  /** The last y-coordinate of the circle. */
  float lastY;

  /** The x-axis acceleration of the circle. */
  float accelX;

  /** The y-axis acceleration of the circle. */
  float accelY;
};

/**
 * A single arrow.
 */
struct arrow {
  /** The source circle. */
  struct circle* src;

  /** The destination circle. */
  struct circle* dest;
};

/**
 * Update physics of the circles.
 */
static void updateCirclePhysics() {
  // Iterate over circles
  for (int i = 0; i < numCircles; ++i) {
    struct circle* circle = circles[i];

    // The net force on this circle
    float forceX = 0;
    float forceY = 0;

    // Subtract force due to friction (this is pretty hacky)
    // It uses *roughly* the current velocity and an arbitrary factor
    // Needless to say, it could be better!
    forceX -= 50 * circle->size * (circle->currentX - circle->lastX);
    forceY -= 50 * circle->size * (circle->currentY - circle->lastY);

    // Factor in circle-side repulsion
    forceX += 10000 * circle->size / fmax(1, circle->currentX);
    forceX += 10000 * circle->size / fmin(-1, circle->currentX - fbWidth);
    forceY += 10000 * circle->size / fmax(1, circle->currentY);
    forceY += 10000 * circle->size / fmin(-1, circle->currentY - fbHeight);

    // Factor in circle-circle repulsion
    for (int j = 0; j < numCircles; ++j) {
      struct circle* otherCircle = circles[j];

      // Compute x- and y-axis distances between these two circles
      float dx = circle->currentX - otherCircle->currentX;
      float dy = circle->currentY - otherCircle->currentY;

      // Compute absolute distance
      double dist = sqrt(dx * dx + dy * dy);

      // Ignore "close enough" circles
      // This is a hack that avoids some divisions by zero
      if (dist < 0.01)
        continue;

      // Compute magnitude of repulsive force
      float mag = 100 * (circle->size * otherCircle->size) / (dist * dist);

      // Compute vector components of force
      forceX += mag * dx;
      forceY += mag * dy;
    }

    // Go over all arrows from this circle
    for (int j = 0; j < numArrows; ++j) {
      struct arrow* arrow = arrows[j];

      if (arrow->src != circle)
        continue;

      // Get displacement of circles
      float dx = arrow->src->currentX - arrow->dest->currentX;
      float dy = arrow->src->currentY - arrow->dest->currentY;

      // Compute Euclidian distance between circles
      double dist = sqrt(dx * dx + dy * dy);

      // "Close enough" rejection like above
      if (dist < 0.01)
        continue;

      // Compute target distance
      double targetDist = arrow->src->size + arrow->dest->size + 150;

      // Add a spring force between these two
      forceX += 100 * (dist - targetDist) * -dx / dist;
      forceY += 100 * (dist - targetDist) * -dy / dist;
    }

    // Update acceleration of the circle based on its net force
    circle->accelX = forceX / circle->size;
    circle->accelY = forceY / circle->size;
  }

  // Integrate new circle positions
  for (int i = 0; i < numCircles; ++i) {
    struct circle* circle = circles[i];

    // Save a copy of the circle's current position
    float tempX = circle->currentX;
    float tempY = circle->currentY;

    // Verlet integration-style position update
    // We don't need to track circle velocities (we compute velocities on the fly, essentially)
    float delta = 1 / 60.0f;
    circle->currentX += (circle->currentX - circle->lastX) + circle->accelX * delta * delta;
    circle->currentY += (circle->currentY - circle->lastY) + circle->accelY * delta * delta;

    // Keep old positions for future integrations
    circle->lastX = tempX;
    circle->lastY = tempY;
  }
}

/**
 * Draw the FPS count.
 */
static void drawFps() {
  // Create FPS count text
  char text[16];
  snprintf(text, sizeof text - 1, "FPS: %.0f", perfFps);

  nvgSave(vg);

  // Set text properties
  nvgFontFace(vg, "sans");
  nvgFontSize(vg, 22);
  nvgFillColor(vg, nvgRGBA(120, 120, 120, 255));
  nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

  // Draw text
  nvgText(vg, 8, 8, text, NULL);

  nvgRestore(vg);
}

/**
 * Draw the loaded data info text.
 */
static void drawInfoText() {
  // Line 1: File name
  char line1[128];
  snprintf(line1, sizeof line1 - 1, "File: %s", "/home/person/file.txt");

  // Line 2: Number of circles
  char line2[16];
  snprintf(line2, sizeof line2 - 1, "Circles: %d", numCircles);

  // Line 3: Number of arrows
  char line3[16];
  snprintf(line3, sizeof line3 - 1, "Arrows: %d", numArrows);

  nvgSave(vg);

  // Set text properties
  nvgFontFace(vg, "sans");
  nvgFontSize(vg, 22);
  nvgFillColor(vg, nvgRGBA(120, 120, 120, 255));
  nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);

  // Draw lines of text
  nvgText(vg, fbWidth - 8, 8, line1, NULL);
  nvgText(vg, fbWidth - 8, 8 + 22 + 2, line2, NULL);
  nvgText(vg, fbWidth - 8, 8 + 22 + 2 + 22 + 2, line3, NULL);

  nvgRestore(vg);
}

/**
 * Initialize for rendering.
 */
static void renderInit() {
  // Set OpenGL clear color to white
  glClearColor(1, 1, 1, 1);

  // Try to create a NanoVG drawing context
  // This library provides a canvas-style interface on top of OpenGL
  vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  if (!vg) {
    fprintf(stderr, "error: failed to create NanoVG context\n");
    exit(1);
  }

  // Set my desired sans-serif font
  nvgCreateFont(vg, "sans", "Roboto-Regular.ttf");
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
  // The fbWidth and fbHeight variables are global so we can render with them
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

  // Update the OpenGL viewport dimensions
  // Again, this would be more perfomant in a callback function
  glViewport(0, 0, fbWidth, fbHeight);

  // Clear all the buffers we use
  // This erases the previous frame's rendered content and artifacts
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // Begin a new NanoVG frame
  nvgBeginFrame(vg, windowWidth, windowHeight, fbWidth / (float) windowWidth);

  // Draw self-referencing arrows
  for (int i = 0; i < numArrows; ++i) {
    struct arrow* arrow = arrows[i];

    // Assert arrow has good source and destination circles
    if (!arrow->src || !arrow->dest)
      continue;

    // If arrow is self-referencing
    if (arrow->src == arrow->dest) {
      float cx = arrow->src->currentX + arrow->src->size;
      float cy = arrow->src->currentY;
      float rad = arrow->src->size;

      // Arrow shaft
      nvgBeginPath(vg);
      nvgArc(vg, cx, cy, rad, 0, 2 * M_PI, NVG_CW);
      nvgStrokeColor(vg, nvgRGBA(255, 0, 0, 255));
      nvgStroke(vg);
    } else {
      // Center to center
      float cx1 = arrow->src->currentX;
      float cy1 = arrow->src->currentY;
      float cx2 = arrow->dest->currentX;
      float cy2 = arrow->dest->currentY;

      // Distance from center to center
      double dist = sqrt((cx2 - cx1) * (cx2 - cx1) + (cy2 - cy1) * (cy2 - cy1));

      // Nearest edge to nearest edge
      float x1 = ((cx2 - cx1) / dist) * arrow->src->size + arrow->src->currentX;
      float y1 = ((cy2 - cy1) / dist) * arrow->src->size + arrow->src->currentY;
      float x2 = ((cx1 - cx2) / dist) * arrow->dest->size + arrow->dest->currentX;
      float y2 = ((cy1 - cy2) / dist) * arrow->dest->size + arrow->dest->currentY;

      // Intermediate arrowhead vector
      // This is from the perspective of (x2, y2) and points toward (x1, y1) with arrowhead length
      float hvx = ((cx1 - cx2) / dist) * 10;
      float hvy = ((cy1 - cy2) / dist) * 10;

      // Perpendicular vector to (hvx, hvy), but with half this length
      // We'll use this to displace the arrowhead from the line a bit by a certain angle
      float hvxp = -hvy / 2;
      float hvyp = hvx / 2;

      // Final arrowhead points coordinates
      // Two points: one (usually) above and one (usually) below the arrow shaft
      // If the shaft is vertical, take your pick (I don't know which will be which)
      float h1x2 = hvx + hvxp + x2;
      float h1y2 = hvy + hvyp + y2;
      float h2x2 = hvx - hvxp + x2;
      float h2y2 = hvy - hvyp + y2;

      // Arrow shaft
      nvgBeginPath(vg);
      nvgMoveTo(vg, x1, y1);
      nvgLineTo(vg, x2, y2);
      nvgStrokeColor(vg, nvgRGBA(255, 0, 0, 255));
      nvgStroke(vg);

      // Arrowhead part 1
      nvgBeginPath(vg);
      nvgMoveTo(vg, x2, y2);
      nvgLineTo(vg, h1x2, h1y2);
      nvgStrokeColor(vg, nvgRGBA(255, 0, 0, 255));
      nvgStroke(vg);

      // Arrowhead part 2
      nvgBeginPath(vg);
      nvgMoveTo(vg, x2, y2);
      nvgLineTo(vg, h2x2, h2y2);
      nvgStrokeColor(vg, nvgRGBA(255, 0, 0, 255));
      nvgStroke(vg);
    }
  }

  // Draw circles
  for (int i = 0; i < numCircles; ++i) {
    struct circle* circle = circles[i];

    // Circle fill
    nvgBeginPath(vg);
    nvgCircle(vg, circle->currentX, circle->currentY, circle->size);
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
    nvgFill(vg);
  }

  // Draw the FPS counter
  drawFps();

  // Draw loaded data info text
  drawInfoText();

  // End the NanoVG frame
  nvgEndFrame(vg);
}

int main() {
  numCircles = 3;
  circles = calloc(3, sizeof(struct circle*));
  circles[0] = calloc(1, sizeof(struct circle));
  circles[0]->size = 40;
  circles[0]->currentX = circles[0]->lastX = 500;
  circles[0]->currentY = circles[0]->lastY = 300;
  circles[1] = calloc(1, sizeof(struct circle));
  circles[1]->size = 40;
  circles[1]->currentX = circles[1]->lastX = 200;
  circles[1]->currentY = circles[1]->lastY = 50;
  circles[2] = calloc(1, sizeof(struct circle));
  circles[2]->size = 40;
  circles[2]->currentX = circles[2]->lastX = 240;
  circles[2]->currentY = circles[2]->lastY = 300;

  numArrows = 2;
  arrows = calloc(3, sizeof(struct arrow*));
  arrows[0] = calloc(1, sizeof(struct arrow));
  arrows[0]->src = circles[0];
  arrows[0]->dest = circles[1];
  arrows[1] = calloc(1, sizeof(struct arrow));
  arrows[1]->src = circles[0];
  arrows[1]->dest = circles[0];

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
    if (glfwWindowShouldClose(window)) {
      break;
    }

    // Update all the circles
    updateCirclePhysics();

    // Record time before rendering
    double timeBeforeFrame = glfwGetTime();

    // Render the next frame
    renderFrame();

    // Swap the next frame in
    glfwSwapBuffers(window);

    // Record frame time into perf window
    perfWindow[perfWindowCurrent] = glfwGetTime() - timeBeforeFrame;
    perfWindowCurrent = ++perfWindowCurrent % PERF_WINDOW_SIZE;

    // Compute frames per second
    perfFps = 0;
    for (int i = 0; i < PERF_WINDOW_SIZE; ++i) {
      perfFps += perfWindow[i];
    }
    perfFps = PERF_WINDOW_SIZE / perfFps;
  }
}
