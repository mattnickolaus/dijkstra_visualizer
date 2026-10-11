#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dijkstras.h"

HeapNode da_pop_heap(MinHeap *mh, size_t i) {
  if (mh->count == 0) {
    printf("Error: no items in array to pop");
    return (HeapNode){0};
  }
  if (i >= mh->count) {
    printf("Error: index:%zu is out of bounds of array", i);
    return (HeapNode){0};
  }
  HeapNode popped_item = mh->items[i];

  for (; i < mh->count - 1; i++) {
    mh->items[i] = mh->items[i + 1];
  }
  mh->items[i] = (HeapNode){0};

  mh->count--;
  return popped_item;
}

void bubble_up(MinHeap *mh, int index) {
  if (index == 0) {
    return;
  }
  int parent_index = (index - 1) / 2; // Should truncate the value
  if (mh->items[parent_index].priority > mh->items[index].priority) {
    HeapNode temp = mh->items[parent_index];
    mh->items[parent_index] = mh->items[index];
    mh->items[index] = temp;
    bubble_up(mh, parent_index);
  }
}

void mh_push(MinHeap *mh, HeapNode n) {
  da_append(mh, n);
  bubble_up(mh, mh->count - 1);
}

void bubble_down(MinHeap *mh, size_t index) {
  size_t left_child_index = 2 * index + 1;
  size_t right_child_index = 2 * index + 2;
  size_t min_index = index;

  if (left_child_index < mh->count &&
      mh->items[left_child_index].priority < mh->items[min_index].priority) {
    min_index = left_child_index;
  }

  if (right_child_index < mh->count &&
      mh->items[right_child_index].priority < mh->items[min_index].priority) {
    min_index = right_child_index;
  }

  if (min_index != index) {
    HeapNode temp = mh->items[index];
    mh->items[index] = mh->items[min_index];
    mh->items[min_index] = temp;

    bubble_down(mh, min_index);
  }
}

HeapNode mh_pop(MinHeap *mh) {
  if (mh->count == 0) {
    return (HeapNode){0};
  }
  if (mh->count == 1) {
    return da_pop_heap(mh, 0);
  }

  HeapNode root = mh->items[0];
  mh->items[0] = da_pop_heap(mh, mh->count - 1);
  bubble_down(mh, 0);

  return root;
}

int mh_peek(MinHeap *mh) {
  if (mh->count == 0) {
    printf("No nodes in Heap...returning 0");
    return 0;
  }
  return mh->items[0].id;
}

void dijkstra(Graph *g, int src, int dest, AnimationPath *animPath) {
  if (dest > g->count - 1) {
    printf("Destination is not within the graph");
    return;
  }
  // Bucket for tracking distances with index = id of Vertex
  int distances[g->count];
  // Keeps track of the neighbor with the shortest distance for a given Vertex
  // (used to backtrack)
  int prev[g->count];
  // Keeps track of the vertices we have visited already
  bool visited[g->count];

  for (int i = 0; i < g->count; i++) {
    distances[g->items[i].id] = INT_MAX; // initialize to always be max distance

    prev[g->items[i].id] = src;

    visited[g->items[i].id] = false;
  }
  // Set the src vertex to 0 since we start there
  distances[src] = 0;

  MinHeap mh = {0};
  mh_push(&mh, (HeapNode){.id = src, .priority = 0});

  while (mh.count != 0) {
    HeapNode neighborNode = mh_pop(&mh);
    if (visited[neighborNode.id]) {
      continue;
    }

    visited[neighborNode.id] = true;

    // For tracks the algorithm path for animation
    Path *curr_prev = malloc(sizeof(Path));
    *curr_prev = (Path){0};
    da_append_many(curr_prev, prev, g->count);
    AnimationStep step = (AnimationStep){neighborNode, curr_prev};
    da_append(animPath, step);

    if (neighborNode.id == dest) {
      break;
    }

    Vertex n = g->items[neighborNode.id];
    for (int i = 0; i < n.count; i++) {
      Edge e = n.items[i];
      if (visited[e.neighbor]) {
        continue;
      }

      float new_distance = neighborNode.priority + e.weight;

      if (new_distance < distances[e.neighbor]) {
        distances[e.neighbor] = new_distance;
        prev[e.neighbor] = e.origin;

        mh_push(&mh, (HeapNode){.id = e.neighbor, .priority = new_distance});
      }
    }
  }

  if (distances[dest] == INT_MAX) {
    printf("Destination could not be found\n");
    return;
  }

  Path *p = malloc(sizeof(Path));
  *p = (Path){0};

  int current = dest;
  while (current != src) {
    da_append(p, current);
    current = prev[current];
  }
  // src is missing so add it at the end
  da_append(p, src);
  da_reverse(p);

  da_free(&mh); // free minHeap
  animPath->path = p;
}
