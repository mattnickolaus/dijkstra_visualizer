#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dijkstras.h"
#include "raylib.h"

#define SW 640 // 800 // 1920 // 640 // 1280
#define SH 340 // 600 // 1080 // 340 // 720

#define TARGET_FPS 60

/* Arena Implementation */
/////////////////////////////////////////

bool is_power_of_two(uintptr_t x) { return (x & (x - 1)) == 0; }

uintptr_t align_forward(uintptr_t ptr, size_t align) {
  uintptr_t p, a, modulo;

  assert(is_power_of_two(align));

  p = ptr;
  a = (uintptr_t)align;
  // Same as (p % a) but faster as 'a' is a power of two
  modulo = p & (a - 1);

  if (modulo != 0) {
    // If 'p' address is not aligned, push the address to the
    // next value which is aligned
    p += a - modulo;
  }
  return p;
}

typedef struct {
  unsigned char *buf;
  size_t buf_len;
  size_t offset;
} TextArena;

void *arena_alloc_align(TextArena *ta, size_t size, size_t align) {
  // Algin offset
  uintptr_t cur_ptr = (uintptr_t)ta->buf + (uintptr_t)ta->offset;
  uintptr_t offset = align_forward(cur_ptr, align);
  offset -= (uintptr_t)ta->buf;

  if (offset + size <= ta->buf_len) {
    void *ptr = &ta->buf[offset];
    ta->offset = offset + size;

    memset(ptr, 0, size);
    return ptr;
  }
  printf("Could not allocate memory for Vertex Arena");
  return NULL;
}

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

// Because C doesn't have default parameters
void *arena_alloc(TextArena *ta, size_t size) {
  return arena_alloc_align(ta, size, DEFAULT_ALIGNMENT);
}

void arena_init(TextArena *ta, void *backing_buffer,
                size_t backing_buffer_length) {
  ta->buf = (unsigned char *)backing_buffer;
  ta->buf_len = backing_buffer_length;
  ta->offset = 0;
}

////////////////////////////////////////////

void SwitchColorOnState(Color *c, int state) {
  switch (state) {
  case (0):
    *c = BLUE;
    break;
  case (1):
    *c = DARKBLUE;
    break;
  case (2):
    *c = SKYBLUE;
    break;
  default:
    *c = BLUE;
    break;
  }
}

typedef struct {
  Rectangle boundaryBox;
  Color color;
  Vector2 postion;
  int state;
  bool action;
  const char *text;
} Button;

typedef struct {
  Button btnCreateVertex;
  Button btnCreateEdge;
  Button btnSolve;
  Button btnReset;
} GuiButtons;

void HandleButtonClick(Vector2 mp, Button *b) {
  if (CheckCollisionPointRec(mp, b->boundaryBox)) {
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
      b->state = 2;
    else
      b->state = 1;

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
      b->action = true;
  } else
    b->state = 0;
}

typedef struct {
  Vector2 *startPoint; // p1 center
  Vector2 *endPoint;   // p2 center
  size_t p1;
  size_t p2;
  Color color;
} GuiEdge;
// Distance/weight will be calculated in realtime

typedef struct {
  GuiEdge *items;
  size_t count;
  size_t capacity;
} GuiEdges;

typedef struct {
  Vector2 center;
  float radius;
  Color color;
  const char *label;
  int state;
  bool action;
} Circle;

typedef struct {
  Circle *items;
  size_t count;
  size_t capacity;
} GuiCircles;

typedef struct {
  GuiEdges *edgesSelected;
  int solutionsSelected[2];
  GuiButtons *buttons;
  TextArena *ta;  // arena to allocate vertex text
  GuiCircles *gc; // main gui vertices
  GuiEdges *ge;   // main gui edges
  GuiCircles *sc; // Circles for displaying solution
  GuiEdges *se;   // Edges for displaying solution
  size_t animSolutionCounter;
  bool solving;
} GameState;

Vector2 getWeightLabelPosition(Vector2 p1, Vector2 p2) {
  float xhalfway = ((p2.x - p1.x) / 2.0f) + p1.x;
  float yhalfway = ((p2.y - p1.y) / 2.0f) + p1.y;

  return (Vector2){xhalfway, yhalfway};
}

float DistanceBetweenPoints(Vector2 p1, Vector2 p2) {
  float xDelta = p1.x - p2.x;
  float yDelta = p1.y - p2.y;

  return sqrtf(xDelta * xDelta + yDelta * yDelta);
}

void AppendEdgeSelection(GameState *gs, GuiCircles *gc, size_t i) {
  GuiEdge e;
  if (gs->edgesSelected->count == 0) {
    e.startPoint = &gc->items[i].center;
    e.p1 = i;
  }
  if (gs->edgesSelected->count > 0) {
    e.endPoint = &gc->items[i].center;
    e.p2 = i;
  }
  da_append(gs->edgesSelected, e);
}

void HandleSolutionSelect(GuiCircles *gc, GameState *gs, Vector2 mp) {
  for (int i = 0; i < gc->count; i++) {
    if (CheckCollisionPointCircle(mp, gc->items[i].center,
                                  gc->items[i].radius)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        gc->items[i].state = 2;
      } else
        gc->items[i].state = 1;

      if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (gs->solutionsSelected[0] == INT_MAX) {
          gs->solutionsSelected[0] = i;
        } else if (gs->solutionsSelected[1] == INT_MAX) {
          gs->solutionsSelected[1] = i;
        }
      } else {
        gc->items[i].state = 0;
      }
      SwitchColorOnState(&gc->items[i].color, gc->items[i].state);
    }
  }
}

void HandleVertexSelection(GuiCircles *gc, GameState *gs, Vector2 mp) {
  for (int i = 0; i < gc->count; i++) {
    if (CheckCollisionPointCircle(mp, gc->items[i].center,
                                  gc->items[i].radius)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        gc->items[i].state = 2;
      } else
        gc->items[i].state = 1;

      if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        AppendEdgeSelection(gs, gc, i);
        // Append to a selection buffer is GameState

        if (gs->edgesSelected->count == 2) {
          size_t firstSelection = gs->edgesSelected->items[0].p1;
          size_t secondSelection = gs->edgesSelected->items[1].p2;
          if (firstSelection == secondSelection) {
            printf("Please select a different vertex...\n");
            gs->edgesSelected->items[1] = (GuiEdge){0};
            gs->edgesSelected->count--;
          }
        }
        break;
      }
    } else {
      gc->items[i].state = 0;
    }
    SwitchColorOnState(&gc->items[i].color, gc->items[i].state);
  }
}

void CreateEdge(GuiCircles *gc, Vector2 mp, Button *btn, GameState *gs,
                GuiEdges *ge) {
  HandleVertexSelection(gc, gs, mp);

  if (gs->edgesSelected->count >= 2) {
    GuiEdge e = {
        .startPoint = gs->edgesSelected->items[0].startPoint,
        .endPoint = gs->edgesSelected->items[1].endPoint,
        .p1 = gs->edgesSelected->items[0].p1,
        .p2 = gs->edgesSelected->items[1].p2,
        .color = BLUE,
    };
    da_append(ge, e);

    memset(gs->edgesSelected->items, 0, sizeof(GuiEdge) * 2);
    gs->edgesSelected->count = 0;
    btn->action = false;
  }
}

void CreateVertex(Circle defaultCircle, GuiCircles *gc, TextArena *ta) {
  Circle new_circle = defaultCircle;
  char buf[12];
  sprintf(buf, "%zu", gc->count);

  char *new_label = arena_alloc(ta, sizeof(char) * 12);
  strcpy(new_label, buf);

  new_circle.label = new_label;
  da_append(gc, new_circle);
}

void HandleCircleMouseAction(Vector2 mp, Circle *c) {
  if (CheckCollisionPointCircle(mp, c->center, c->radius)) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      c->state = 2;
    } else
      c->state = 1;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      c->action = true;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      c->action = false;
    }
  } else {
    c->state = 0;
  }
}

void RemoveGuiSolveItems(GuiCircles *sc, GuiEdges *se) {
  da_free(sc);
  *sc = (GuiCircles){0};
  da_free(se);
  *se = (GuiEdges){0};
}

bool HandleCircleMove(GuiCircles *gc, Vector2 mousePoint) {
  for (int i = 0; i < gc->count; i++) {
    HandleCircleMouseAction(mousePoint, &gc->items[i]);
    SwitchColorOnState(&gc->items[i].color, gc->items[i].state);
    if (gc->items[i].action) {
      gc->items[i].center = mousePoint;
      return true;
    }
  }
  return false;
}

void LoadGUIStateIntoGraph(Graph *g, GuiCircles *gc, GuiEdges *ge) {
  for (size_t i = 0; i < gc->count; i++) {
    Vertex v = {0};
    v.id = i;

    da_for_each(ge, e) {
      if (e->p1 == i && e->p2 != i) {
        Edge new_edge = {
            .origin = i,
            .neighbor = e->p2,
            .weight = DistanceBetweenPoints(*e->startPoint, *e->endPoint),
        };
        da_append(&v, new_edge);
      }
      if (e->p2 == i && e->p1 != i) {
        Edge new_edge = {
            .origin = i,
            .neighbor = e->p1,
            .weight = DistanceBetweenPoints(*e->startPoint, *e->endPoint),
        };
        da_append(&v, new_edge);
      }
    }

    da_append(g, v);
  }
}

bool SolveGraph(GuiCircles *gc, GameState *gs, GuiEdges *ge, Graph *g,
                GuiCircles *sc, GuiEdges *se, Button *btnSolve,
                Vector2 mousePoint, AnimationPath *animPath) {
  HandleSolutionSelect(gc, gs, mousePoint);

  if (gs->solutionsSelected[1] == INT_MAX) {
    return false;
  }

  da_free(g);
  *g = (Graph){0};
  RemoveGuiSolveItems(sc, se);

  LoadGUIStateIntoGraph(g, gc, ge);

  dijkstra(g, gs->solutionsSelected[0], gs->solutionsSelected[1], animPath);
  if (animPath->path == NULL) {
    btnSolve->action = false;
    printf("dijkstra failed");
    return false;
  }

  for (int i = 0; i < animPath->path->count - 1; i++) {
    printf("%d->", animPath->path->items[i]);
  }
  printf("%d\n", animPath->path->items[animPath->path->count - 1]);

  gs->solutionsSelected[0] = INT_MAX;
  gs->solutionsSelected[1] = INT_MAX;
  btnSolve->action = false;
  return true;
}

void SetCurrentSolveFrame(AnimationPath *animPath, GuiCircles *gc,
                          GameState *gs, GuiCircles *sc, GuiEdges *se) {
  RemoveGuiSolveItems(sc, se);

  AnimationStep current = animPath->items[gs->animSolutionCounter];

  gs->animSolutionCounter++;

  Circle circleCopy = gc->items[current.node.id];
  circleCopy.color = RED;
  circleCopy.radius += circleCopy.radius / 4.0f;
  da_append(sc, circleCopy);

  if (gs->animSolutionCounter == 0) {
    return;
  }

  da_for_each(current.prev, prevNode) {
    Circle prevCircle = gc->items[*prevNode];
    prevCircle.color = PURPLE;
    prevCircle.radius += prevCircle.radius / 4.0f;

    da_append(sc, prevCircle);
  }
}

void SetFinalSolution(AnimationPath *animPath, GuiCircles *gc, GameState *gs,
                      GuiCircles *sc, GuiEdges *se, Button *btnSolve) {
  RemoveGuiSolveItems(sc, se);

  for (size_t i = 0; i < animPath->path->count; i++) {
    Circle circleCopy = gc->items[animPath->path->items[i]];
    circleCopy.color = GREEN;
    circleCopy.radius += circleCopy.radius / 4.0f;
    da_append(sc, circleCopy);
  }

  for (size_t i = 0; i < sc->count; i++) {
    if (i + 1 < sc->count) {
      GuiEdge e = {
          .startPoint = &sc->items[i].center,
          .endPoint = &sc->items[i + 1].center,
          .color = GREEN,
      };
      da_append(se, e);
    }
  }
}

void InitButtons(GuiButtons *b) {
  b->btnCreateVertex = (Button){
      .boundaryBox = (Rectangle){(float)SW - ((float)SW / 10),
                                 (float)SH - ((float)SH / 20), (float)SW / 11,
                                 (float)SH / 25},
      .color = BLUE,
      .postion = {(float)SW - ((float)SW / 10), (float)SH - ((float)SH / 20)},
      .state = 0,
      .action = false,
      .text = "+ Vertex",
  };
  b->btnCreateEdge = (Button){
      .boundaryBox = {(float)SW - ((float)SW / 10),
                      (float)SH - ((float)SH / 20) - (float)SH / 25 - 5.0f,
                      (float)SW / 11, (float)SH / 25},
      .color = BLUE,
      .postion = {(float)SW - ((float)SW / 10),
                  (float)SH - ((float)SH / 20) - (float)SH / 25 - 5.0f},
      .state = 0,
      .action = false,
      .text = "+ Edge",
  };
  b->btnSolve = (Button){
      .boundaryBox = {5.0f, (float)SH - ((float)SH / 20), (float)SW / 11,
                      (float)SH / 25},
      .color = BLUE,
      .postion = {5.0f, (float)SH - ((float)SH / 20)},
      .state = 0,
      .action = false,
      .text = "Solve",
  };
};

void DrawButtons(GuiButtons *b, const int fontSize) {
  DrawRectangleRec(b->btnCreateVertex.boundaryBox, b->btnCreateVertex.color);
  DrawText(
      b->btnCreateVertex.text,
      b->btnCreateVertex.postion.x + b->btnCreateVertex.boundaryBox.width / 20,
      b->btnCreateVertex.postion.y + b->btnCreateVertex.boundaryBox.height / 4,
      fontSize, WHITE);
  DrawRectangleRec(b->btnCreateEdge.boundaryBox, b->btnCreateEdge.color);
  DrawText(b->btnCreateEdge.text,
           b->btnCreateEdge.postion.x + b->btnCreateEdge.boundaryBox.width / 20,
           b->btnCreateEdge.postion.y + b->btnCreateEdge.boundaryBox.height / 4,
           fontSize, WHITE);

  DrawRectangleRec(b->btnSolve.boundaryBox, b->btnSolve.color);
  DrawText(b->btnSolve.text,
           b->btnSolve.postion.x + b->btnSolve.boundaryBox.width / 20,
           b->btnSolve.postion.y + b->btnSolve.boundaryBox.height / 4, fontSize,
           WHITE);
}

int main(void) {

  const int defaultFontSize = SW / 80; // ex: SW=1920 -> 24pt

  void *backing_buffer = malloc(sizeof(char) * 512);
  TextArena ta = {0};
  arena_init(&ta, backing_buffer, sizeof(char) * 512);

  GuiCircles gc = {0};
  GuiEdges ge = {0};

  GuiCircles sc = {0}; // solution circles
  GuiEdges se = {0};   // solution edges

  GuiEdges edgeSelections = {0};
  GuiButtons gb = {0};
  GameState gs = {
      .edgesSelected = &edgeSelections,
      .solutionsSelected = {INT_MAX, INT_MAX},
      .buttons = &gb,
  };
  // GameState gs = {0};
  // InitGameState(&gs);
  // arena_init(gs.ta, backing_buffer, sizeof(char) * 512);

  Graph g = {0};

  InitButtons(gs.buttons);

  Circle defaultCircle = {
      .center = (Vector2){(float)SW / 2, (float)SH / 2},
      .radius = (float)SW / 30.0f,
      .color = BLUE,
      .action = false,
      .state = 0,
  };

  InitWindow(SW, SH, "Dijkstra's Demo");

  Vector2 mousePoint = {0.0f, 0.0f};
  SetTargetFPS(TARGET_FPS);

  AnimationPath animPath = {0};

  // Initializing set Tick Rate and accumulator to track the delta time
  // between ticks
  const double tick_rate = 1.0; // 1 animation "frame" per second
  double accumulator = 0.0f;

  while (!WindowShouldClose()) {
    // Update

    mousePoint = GetMousePosition();
    gs.buttons->btnCreateVertex.action = false;

    if (gc.count >= 2) {
      HandleButtonClick(mousePoint, &gs.buttons->btnCreateEdge);
    }

    if (ge.count > 0) {
      HandleButtonClick(mousePoint, &gs.buttons->btnSolve);
    }

    // disble vertex create and move if Edge Select is active
    if (!gs.buttons->btnCreateEdge.action && !gs.buttons->btnSolve.action) {
      bool moved = HandleCircleMove(&gc, mousePoint);
      if (moved) {
        RemoveGuiSolveItems(&sc, &se);
      }
      HandleButtonClick(mousePoint, &gs.buttons->btnCreateVertex);
    }

    if (gs.buttons->btnCreateVertex.action) {
      RemoveGuiSolveItems(&sc, &se);
      CreateVertex(defaultCircle, &gc, &ta);
    }

    if (gs.buttons->btnCreateEdge.action) {
      CreateEdge(&gc, mousePoint, &gs.buttons->btnCreateEdge, &gs, &ge);
    }

    if (gs.buttons->btnSolve.action) {
      gs.animSolutionCounter = 0;
      animPath = (AnimationPath){0};

      gs.solving = SolveGraph(&gc, &gs, &ge, &g, &sc, &se,
                              &gs.buttons->btnSolve, mousePoint, &animPath);
    }

    if (gs.solving) {
      // set some tickrate to update the Animation steps at a slow enough
      // speed
      accumulator += GetFrameTime();

      while (accumulator >= tick_rate) {
        // Update the solution circles for output

        if (gs.animSolutionCounter == animPath.count) {
          SetFinalSolution(&animPath, &gc, &gs, &sc, &se,
                           &gs.buttons->btnSolve);
          gs.solving = false;
        } else {
          SetCurrentSolveFrame(&animPath, &gc, &gs, &sc, &se);
        }
        accumulator -= tick_rate;
      }
    }

    SwitchColorOnState(&gs.buttons->btnCreateVertex.color,
                       gs.buttons->btnCreateVertex.state);
    SwitchColorOnState(&gs.buttons->btnCreateEdge.color,
                       gs.buttons->btnCreateEdge.state);
    SwitchColorOnState(&gs.buttons->btnSolve.color, gs.buttons->btnSolve.state);

    // Draw
    BeginDrawing();
    ClearBackground(WHITE);

    // Solution circles
    da_for_each(&sc, c) { DrawCircleV(c->center, c->radius, c->color); }
    // Solution edges
    da_for_each(&se, e) {
      DrawLineEx(*e->startPoint, *e->endPoint, 12.0f, e->color);
    }

    // Vertexes
    da_for_each(&ge, e) {
      DrawLineEx(*e->startPoint, *e->endPoint, 4.0f, e->color);
      float weight = DistanceBetweenPoints(*e->startPoint, *e->endPoint);
      Vector2 labelPosition =
          getWeightLabelPosition(*e->startPoint, *e->endPoint);
      labelPosition.x += 2.0f;
      labelPosition.y += 2.0f;

      DrawText(TextFormat("%.0f", weight), labelPosition.x, labelPosition.y,
               defaultFontSize, BLUE);
    }

    // Edges
    da_for_each(&gc, c) {
      DrawCircleV(c->center, c->radius, c->color);
      DrawText(c->label, c->center.x - c->radius / 10.0f,
               c->center.y - c->radius / 10.0f, defaultFontSize, WHITE);
    }

    // Draw all GUI Buttons
    DrawButtons(gs.buttons, defaultFontSize);

    // Solution Selection Instruction Text
    if (gs.buttons->btnSolve.action) {
      DrawText("Select the start and end vertices to find the Shortest path...",
               SW / 3, SH / 20, defaultFontSize, BLACK);
    }

    // Edge Selection Instruction Text and temporary line
    if (gs.buttons->btnCreateEdge.action) {
      DrawText("Select 2 Vertices to draw edge between...", (SW / 2) - (SW / 8),
               SH / 20, defaultFontSize, BLACK);
      if (gs.edgesSelected->count == 1) {
        DrawLineEx(*gs.edgesSelected->items[0].startPoint, mousePoint, 4.0f,
                   BLUE);
      }
    }

    if (gc.count == 0) {
      DrawText("Click on + Vertex to start creating a graph",
               (SW / 2) - (SW / 6), SH / 20, defaultFontSize, BLACK);
    }

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
