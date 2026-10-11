#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Any tpye struct that has the following implementation can be used as a da:
//
// typedef sturct {
//	ANYTYPE *items;
//	size_t count;
//	size_t capacity;
// } da;
//

#define DA_INIT_CAP 32

#define da_append(da, item)                                                    \
  do {                                                                         \
    size_t expected_capacity = (da)->count + 1;                                \
    if ((expected_capacity) > (da)->capacity) {                                \
      if ((da)->capacity == 0) {                                               \
        (da)->capacity = DA_INIT_CAP;                                          \
      }                                                                        \
      while ((expected_capacity) > (da)->capacity) {                           \
        (da)->capacity *= 2;                                                   \
      }                                                                        \
      (da)->items =                                                            \
          realloc((da)->items, (da)->capacity * sizeof(*(da)->items));         \
      assert((da)->items != NULL);                                             \
    }                                                                          \
                                                                               \
    (da)->items[(da)->count] = item;                                           \
    (da)->count++;                                                             \
  } while (0)

#define da_free(da)                                                            \
  do {                                                                         \
    free((da)->items);                                                         \
  } while (0)

#define da_reverse(da)                                                         \
  do {                                                                         \
    for (int i = 0; i < (da)->count / 2; i++) {                                \
      typeof((da)->items[i]) temp = (da)->items[i];                            \
      (da)->items[i] = (da)->items[(da)->count - 1 - i];                       \
      (da)->items[(da)->count - 1 - i] = temp;                                 \
    }                                                                          \
  } while (0)

#define da_reserve(da, expected_capacity)                                      \
  do {                                                                         \
    if ((expected_capacity) > (da)->capacity) {                                \
      if ((da)->capacity == 0) {                                               \
        (da)->capacity = DA_INIT_CAP;                                          \
      }                                                                        \
      while ((expected_capacity) > (da)->capacity) {                           \
        (da)->capacity *= 2;                                                   \
      }                                                                        \
      (da)->items =                                                            \
          realloc((da)->items, (da)->capacity * sizeof(*(da)->items));         \
      assert((da)->items != NULL && "Out of memory lol");                      \
    }                                                                          \
  } while (0)

#define da_append_many(da, new_items, new_items_count)                         \
  do {                                                                         \
    da_reserve((da), (da)->count + (new_items_count));                         \
    memcpy((da)->items + (da)->count, (new_items),                             \
           (new_items_count) * sizeof(*(da)->items));                          \
    (da)->count += (new_items_count);                                          \
  } while (0)

#define da_for_each(da, it)                                                    \
  for (typeof((da)->items[0]) *it = (da)->items;                               \
       it < (da)->items + (da)->count; it++)

#define da_pop(da, i)                                                          \
  do {                                                                         \
    if ((da)->count == 0) {                                                    \
      printf("Error: no items in array to pop");                               \
      break;                                                                   \
    }                                                                          \
    if (i >= (da)->count) {                                                    \
      printf("Error: index:%zu is out of bounds of array", i);                 \
      break;                                                                   \
    }                                                                          \
    for (; i < (da)->count - 1; i++) {                                         \
      (da)->items[i] = (da)->items[i + 1];                                     \
    }                                                                          \
    (da)->items[i] = (typeof((da)->items[0])){0};                              \
    (da)->count--;                                                             \
  } while (0)

typedef struct {
  float priority;
  int id;
} HeapNode;

typedef struct {
  HeapNode *items;
  size_t count;
  size_t capacity;
} MinHeap;

typedef struct {
  size_t origin;   // index of Vertex the edge "originated" from
  size_t neighbor; // index of the neighboring Vertex
  float weight;    // <- change back to int if needed  // aka the priority for
                   // minheap
} Edge;

typedef struct {
  int id;
  size_t count;
  size_t capacity;
  Edge *items; // Array of edges
} Vertex;

typedef struct {
  Vertex *items; // Array of Vertices
  size_t count;
  size_t capacity;
} Graph;

typedef struct {
  int *items;
  size_t count;
  size_t capacity;
} Path;

typedef struct {
  HeapNode node;
  Path *prev;
} AnimationStep;

typedef struct {
  Path *path;
  AnimationStep *items;
  size_t count;
  size_t capacity;
} AnimationPath;

void dijkstra(Graph *g, int src, int dest, AnimationPath *animPath);
