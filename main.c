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
 * - It has limited support for drawing multiple duplicate arrows. (only self-referencing)
 * - Accepts between 2 and 20 circles and between 2 and 100 arrows.
 * - Consequently, this turned into a vector math refresher :)
 */

#include <math.h>
#include <stdio.h>
#include <time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>

/** The number of loaded circles. */
static int numCircles;

/** The loaded circles. */
static struct circle** circles;

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

/** The name of the loaded file. */
static char fileName[256];

/** The current circle marker. */
static struct marker* marker;

/** Gameplay state. The total number of marks on the board. */
static int gameTotalNumMarks;

/** Gameplay state. The current circle. */
static int gameCurrentCircle;

/** Gameplay state. The number of unique circles marked. */
static int gameNumUniqueCirclesMarked;

/** Gameplay statistic. The maximum number of checks on a single circle. */
static int gameMaxChecksSingle;

/** Gameplay statistic. The average number of checks on a single circle. */
static float gameAvgChecksSingle;

/**
 * One or more arrows.
 */
struct arrow {
  /** The arrow count. */
  int count;

  /** The arrow destination. */
  struct circle* dest;
};

/**
 * A single circle.
 */
struct circle {
  /** The circle number. */
  int number;

  /** The outgoing arrows. */
  struct arrow** arrows;

  /** The number of unique outgoing arrows. */
  int numUniqueArrows;

  /** The total number of outgoing arrows. */
  int numArrows;

  /** The number of checkmarks on the circle. */
  int numMarks;

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
 * A position marker. This is the ring shape that outlines the current circle as
 * the game is played.
 */
struct marker {
  /** The current x-coordinate of the marker. */
  float currentX;

  /** The current y-coordinate of the marker. */
  float currentY;

  /** The last x-coordinate of the marker. */
  float lastX;

  /** The last y-coordinate of the marker. */
  float lastY;

  /** The x-axis acceleration of the marker. */
  float accelX;

  /** The y-axis acceleration of the marker. */
  float accelY;
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
    for (int j = 0; j < circle->numUniqueArrows; ++j) {
      // Get displacement of circles
      float dx = circle->currentX - circle->arrows[j]->dest->currentX;
      float dy = circle->currentY - circle->arrows[j]->dest->currentY;

      // Compute Euclidian distance between circles
      double dist = sqrt(dx * dx + dy * dy);

      // "Close enough" rejection like above
      if (dist < 0.01)
        continue;

      // Compute target distance
      // This is all very roughly computed
      double targetDist = circle->size + circle->arrows[j]->dest->size + 150;

      // Add a spring force between these two
      forceX += 10 * (dist - fmax(0, targetDist)) * -dx / dist;
      forceY += 10 * (dist - fmax(0, targetDist)) * -dy / dist;
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

  // Net force of the marker
  float forceX = 0;
  float forceY = 0;

  // Friction/damping on the marker
  forceX -= 10000 * (marker->currentX - marker->lastX);
  forceY -= 10000 * (marker->currentY - marker->lastY);

  // Make the marker tend toward the current circle
  {
    struct circle* circle = circles[gameCurrentCircle - 1];

    // Get displacement of marker from current circle
    float dx = marker->currentX - circle->currentX;
    float dy = marker->currentY - circle->currentY;

    // Compute Euclidian distance between marker and current circle
    double dist = sqrt(dx * dx + dy * dy);

    // "Close enough" rejection like above
    if (dist > 0.01) {
      // Add a spring force between these two
      forceX += 1000 * dist * -dx / dist;
      forceY += 1000 * dist * -dy / dist;
    }
  }

  // Update acceleration of the marker based on its net force
  marker->accelX = forceX / 10;
  marker->accelY = forceY / 10;

  // Integrate new marker position
  {
    // Save a copy of the marker's current position
    float tempX = marker->currentX;
    float tempY = marker->currentY;

    // Same deal as above
    float delta = 1 / 60.0f;
    marker->currentX += (marker->currentX - marker->lastX) + marker->accelX * delta * delta;
    marker->currentY += (marker->currentY - marker->lastY) + marker->accelY * delta * delta;

    // Keep old positions for future integrations
    marker->lastX = tempX;
    marker->lastY = tempY;
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
  snprintf(line1, sizeof line1 - 1, "File: %s", fileName);

  // Line 2: Number of circles
  char line2[16];
  snprintf(line2, sizeof line2 - 1, "Circles: %d", numCircles);

  // Count total number of arrows on screen
  int numArrows = 0;
  for (int i = 0; i < numCircles; ++i) {
    numArrows += circles[i]->numArrows;
  }

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

static void drawConcludeText() {
  nvgSave(vg);

  nvgFontFace(vg, "sans");
  nvgFontSize(vg, 22);
  nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
  nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
  nvgText(vg, fbWidth / 2, 3 * fbHeight / 4, "GAME OVER", NULL);
  nvgText(vg, fbWidth / 2, 3 * fbHeight / 4 + 22 + 2, "Press ENTER to exit", NULL);

  nvgRestore(vg);
}

/**
 * Draw the running statistics text.
 */
static void drawStatsText() {
  // Line 1: Current circle number
  char line1[128];
  snprintf(line1, sizeof line1 - 1, "Current circle: %d", gameCurrentCircle);

  // Line 2: Unique circles marked
  char line2[128];
  snprintf(line2, sizeof line2 - 1, "Unique circles marked: %d", gameNumUniqueCirclesMarked);

  // Line 3: Total number of checks
  char line3[128];
  snprintf(line3, sizeof line3 - 1, "Total checks on screen: %d", gameTotalNumMarks);

  // Line 4: Total number of checks on one circle
  char line4[128];
  snprintf(line4, sizeof line4 - 1, "Maximum checks on one circle: %d", gameMaxChecksSingle);

  // Line 5: Average number of checks on one circle
  char line5[128];
  snprintf(line5, sizeof line4 - 1, "Average checks on one circle: %.1f", gameAvgChecksSingle);

  nvgSave(vg);

  // Set text properties
  nvgFontFace(vg, "sans");
  nvgFontSize(vg, 22);
  nvgFillColor(vg, nvgRGBA(120, 120, 120, 255));
  nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

  // Draw lines of text
  nvgText(vg, fbWidth - 8, fbHeight - (8 + 22 + 2 + 22 + 2 + 22 + 2 + 22 + 2), line1, NULL);
  nvgText(vg, fbWidth - 8, fbHeight - (8 + 22 + 2 + 22 + 2 + 22 + 2), line2, NULL);
  nvgText(vg, fbWidth - 8, fbHeight - (8 + 22 + 2 + 22 + 2), line3, NULL);
  nvgText(vg, fbWidth - 8, fbHeight - (8 + 22 + 2), line4, NULL);
  nvgText(vg, fbWidth - 8, fbHeight - (8), line5, NULL);

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

  // Draw circles
  for (int i = 0; i < numCircles; ++i) {
    struct circle* circle = circles[i];
    // Draw arrows leading away from this circle
    for (int i = 0; i < circle->numUniqueArrows; ++i) {
      struct arrow* arrow = circle->arrows[i];

      // Iterate through all arrows this arrow struct represents
      for (int j = 0; j < arrow->count; ++j) {
        // If arrow is self-referencing
        if (circle == arrow->dest) {
          float cx = circle->currentX + circle->size + 10 * j;
          float cy = circle->currentY;
          float rad = circle->size + 10 * j;

          // Arrow shaft
          nvgBeginPath(vg);
          nvgArc(vg, cx, cy, rad, 0, 2 * M_PI, NVG_CW);
          nvgStrokeColor(vg, nvgRGBA(255, 0, 0, 255));
          nvgStroke(vg);
        } else {
          // Center to center
          float cx1 = circle->currentX;
          float cy1 = circle->currentY;
          float cx2 = arrow->dest->currentX;
          float cy2 = arrow->dest->currentY;

          // Distance from center to center
          double dist = sqrt((cx2 - cx1) * (cx2 - cx1) + (cy2 - cy1) * (cy2 - cy1));

          // A vector from the center of circle 1 (source) to that of circle 2 (dest)
          float cvx = cx1 - cx2;
          float cvy = cy1 - cy2;

          // A unit vector perpendicular to that above
          float cvxpu = -cvy / dist;
          float cvypu = cvx / dist;

          // Shift the center to accommodate multiple arrows
//        cx1 += cvxpu * 15 * (j - circle->numArrows / 2);
//        cy1 += cvypu * 15 * (j - circle->numArrows / 2);
//        cx2 += cvxpu * 15 * (j - circle->numArrows / 2);
//        cy2 += cvypu * 15 * (j - circle->numArrows / 2);

          // Nearest edge to nearest edge
          float x1 = ((cx2 - cx1) / dist) * circle->size + cx1;
          float y1 = ((cy2 - cy1) / dist) * circle->size + cy1;
          float x2 = ((cx1 - cx2) / dist) * arrow->dest->size + cx2;
          float y2 = ((cy1 - cy2) / dist) * arrow->dest->size + cy2;

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
    }

    // Circle fill
    float colorShift = circle->numMarks == 0 ? 0 : fmin(255, 50 + circle->numMarks * 40);
    nvgBeginPath(vg);
    nvgCircle(vg, circle->currentX, circle->currentY, circle->size);
    nvgFillColor(vg, nvgRGBA(colorShift, 0, colorShift, 255));
    nvgFill(vg);

    // Circle number
    char cnum[4];
    snprintf(cnum, sizeof cnum - 1, "%d", i + 1);
    nvgSave(vg);
    nvgFontFace(vg, "sans");
    nvgFontSize(vg, 22);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(vg, circle->currentX, circle->currentY, cnum, NULL);
    nvgRestore(vg);

    // Mark number
    char marks[4];
    snprintf(marks, sizeof marks - 1, "%d", circle->numMarks);
    nvgSave(vg);
    nvgFontFace(vg, "sans");
    nvgFontSize(vg, 18);
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgText(vg, circle->currentX - 1.5 * circle->size, circle->currentY - 1.5 * circle->size, marks, NULL);
    nvgRestore(vg);
  }

  // Marker ring
  nvgBeginPath(vg);
  nvgCircle(vg, marker->currentX, marker->currentY, 50);
  nvgStrokeColor(vg, nvgRGBA(0, 200, 0, 255));
  nvgStrokeWidth(vg, 4);
  nvgStroke(vg);

  // Draw text features
  drawFps();
  drawInfoText();
  drawStatsText();

  // Draw game over text when the game ends
  if (gameNumUniqueCirclesMarked >= numCircles) {
    drawConcludeText();
  }

  // End the NanoVG frame
  nvgEndFrame(vg);
}

/**
 * Load from the given input file.
 *
 * @param file The input file
 * @return Zero on success, otherwise nonzero
 */
static int loadFromFile(FILE* file) {
  if (!file)
    return 1;

  // Read circle count
  fscanf(file, "%d", &numCircles);

  // Validate circle count
  if (numCircles < 2 || numCircles > 20) {
    fprintf(stderr, "error: circle count out of range: %d\n", numCircles);
    numCircles = 0;
    return 1;
  }

  // Allocate circles
  circles = calloc(numCircles, sizeof(struct circle*));
  for (int i = 0; i < numCircles; ++i) {
    circles[i] = calloc(1, sizeof(struct circle));
    circles[i]->number = i + 1;
    circles[i]->currentX = circles[i]->lastX = rand() % WINDOW_INIT_WIDTH;
    circles[i]->currentY = circles[i]->lastY = rand() % WINDOW_INIT_HEIGHT;
    circles[i]->size = 20;
  }

  // Read arrow count
  int numArrows;
  fscanf(file, "%d", &numArrows);

  // Validate arrow count
  if (numArrows < 2 || numArrows > 100) {
    fprintf(stderr, "error: arrow count out of range: %d\n", numArrows);
    return 1;
  }

  // Read in arrows one-by-one
  for (int i = 0; i < numArrows; ++i) {
    // Read one arrow line
    int arrowSrc;
    int arrowDest;
    fscanf(file, "%d %d", &arrowSrc, &arrowDest);

    // Validate source circle
    if (arrowSrc < 1 || arrowSrc > 20) {
      fprintf(stderr, "error: arrow %d source out of range: %d\n", i, arrowSrc);
      return 1;
    }

    // Validate destination circle
    if (arrowDest < 1 || arrowDest > 20) {
      fprintf(stderr, "error: arrow %d destination out of range: %d\n", i, arrowDest);
      return 1;
    }

    // The source circle
    struct circle* src = circles[arrowSrc - 1];

    // Go over arrows already on the source circle
    // We will coalesce duplicate arrows here
    int coalesced = 0;
    for (int j = 0; j < src->numUniqueArrows; ++j) {
      if (src->arrows[j]->dest == circles[arrowDest - 1]) {
        // Coalesce it by incrementing the counts only
        src->arrows[j]->count++;
        src->numArrows++;
        coalesced = 1;
        break;
      }
    }

    // If arrow was not coalesced
    // We need to add a new arrow here
    if (!coalesced) {
      int newIndex = src->numUniqueArrows;

      // Tack on another arrow pointer for a new arrow
      src->arrows = realloc(src->arrows, ++src->numUniqueArrows * sizeof(struct arrow*));
      src->arrows[newIndex] = calloc(1, sizeof(struct arrow));
      src->arrows[newIndex]->count = 1;
      src->arrows[newIndex]->dest = circles[arrowDest - 1];
      src->numArrows++;
    }
  }

  // Start on circle 1
  gameCurrentCircle = 1;

  // Create the marker over circle 1
  marker = calloc(1, sizeof(struct marker));
  marker->currentX = marker->lastX = circles[0]->currentX;
  marker->currentY = marker->lastY = circles[0]->currentY;
  marker->accelX = 0;
  marker->accelY = 0;

  return 0;
}

/**
 * Take a turn in the game.
 */
static void takeTurn() {
  // The current circle
  struct circle* circle = circles[gameCurrentCircle - 1];

  // Increment marks on circle
  circle->numMarks++;
  gameTotalNumMarks++;

  // Validate number of marks
  if (gameTotalNumMarks > 1000000) {
    fprintf(stderr, "error: maximum number of marks reached on circle %d\n", gameCurrentCircle);
    exit(1);
  }

  // If current circle's counter just hit one
  // This would indicate reaching the circle for the first time
  if (circle->numMarks == 1) {
    gameNumUniqueCirclesMarked++;
    printf("game: reached circle %d for the first time\n", gameCurrentCircle);
    printf("game: there are %d unique circles marked so far out of %d\n", gameNumUniqueCirclesMarked, numCircles);
  }

  // Keep maximum mark count up to date
  if (circle->numMarks > gameMaxChecksSingle) {
    gameMaxChecksSingle = circle->numMarks;
  }

  // Assert positive arrow count
  if (circle->numArrows == 0) {
    fprintf(stderr, "error: circle %d has no output arrows\n", gameCurrentCircle);
    fprintf(stderr, "error: the board is not strongly-connected\n");
    exit(1);
  }

  // The "hat" from which we pick an arrow to follow
  // Since I decided on a really stupid data structure for arrows, I'm stuck with this mess!
  struct circle** hat = NULL;
  int hatNum = 0;

  // Collect all the arrows
  for (int i = 0; i < circle->numUniqueArrows; ++i) {
    for (int j = 0; j < circle->arrows[i]->count; ++j) {
      hat = realloc(hat, ++hatNum * sizeof(struct circle*));
      hat[hatNum - 1] = circle->arrows[i]->dest;
    }
  }

  // Assert hat is allocated (hopefully we didn't run out of memory)
  if (!hat) {
    fprintf(stderr, "error: memory allocation failed when picking an arrow to follow\n");
    exit(1);
  }

  // Pick a random arrow from the hat to follow and follow it
  gameCurrentCircle = hat[rand() % hatNum]->number;

  printf("game: go to circle %d\n", gameCurrentCircle);

  // Clean up the hat
  free(hat);

  // Update average single circle mark count
  float numMarks = 0;
  for (int i = 0; i < numCircles; ++i) {
    numMarks += circles[i]->numMarks;
  }
  gameAvgChecksSingle = numMarks / numCircles;
}

/**
 * Prompt the user for an input file and load it.
 */
static void promptAndLoadFile() {
  printf("Circles and Arrows IV\n");
  printf("By Tyler Filla\n");

  do {
    // Prompt for a file name
    printf("Input: ");
    fgets(fileName, 256, stdin);
    for (int i = 0; i < sizeof fileName; ++i) {
      if (fileName[i] == '\n') {
        fileName[i] = '\0';
      }
    }

    // Try to open the file for read
    FILE* file = fopen(fileName, "r");
    if (!file) {
      perror("error: could not open file: ");
      continue;
    }

    // Try to load state from file
    if (!loadFromFile(file)) {
      printf("Successfully loaded file!\n");
      fclose(file);
      break;
    } else {
      fprintf(stderr, "error: could not load from file\n");
      fclose(file);
      continue;
    }
  } while (1);
}

/**
 * The GLFW key callback for the graphics window.
 */
static void windowKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  // If enter key was pressed
  if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
    // If game is over
    if (gameNumUniqueCirclesMarked >= numCircles) {
      // Then kill the program
      exit(0);
    }
  }
}

int main() {
  // Seed the RNG
  srand(time(NULL));

  // Prompt and load from user-specified file
  promptAndLoadFile();

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

  // Add key callback
  glfwSetKeyCallback(window, windowKeyCallback);

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

  // Time of the last turn taken
  double lastTurnTime = glfwGetTime();

  // Window event loop
  while (1) {
    // If time since last turn exceeded ample time
    if (glfwGetTime() - lastTurnTime > 1) {
      if (gameNumUniqueCirclesMarked >= numCircles) {
        printf("The game is complete.\n");
      } else {
        // Take a turn
        takeTurn();
      }

      // Remember last turn time
      lastTurnTime = glfwGetTime();
    }

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
