#include<stdio.h>
#include<Windows.h>

#include<time.h>

#include "./include/node.h"
#include "./include/borland.h"
#include "./include/graphics.h"
#include "./include/vector.h"

#define OBJ_LINE_BUFFER_SIZE 128
#define OBJ_WORD_BUFFER_SIZE 32

Node initNode(const char *id, Image *image) {
  Node node;
  memset(&node, 0, sizeof(Node));
  memcpy_s(node.id, sizeof(node.id), id, min(sizeof(node.id), strlen(id)));
	node.scale[0] = 1.0;
	node.scale[1] = 1.0;
  node.scale[2] = 1.0;
	node.texture = image;
  node.children = initVector();
  return node;
}

Node initNodeUI(const char *id, Image *image, unsigned char color) {
  Node node;
  memset(&node, 0, sizeof(Node));
  memcpy_s(node.id, sizeof(node.id), id, min(sizeof(node.id), strlen(id)));
	node.scale[0] = 1.0;
	node.scale[1] = 1.0;
  node.scale[2] = 1.0;
	node.texture = image;
  node.shape = initShapePlaneInv(1.0F, 1.0F, color);
  node.children = initVector();
  node.isInterface = TRUE;
  return node;
}

void discardNode(Node node) {
  freeVector(&node.children);
}

void drawNode(Node *node) {
  Node *child;
  IntervalEvent *interval;

  if(node->behaviour != NULL) {
    if(!node->behaviour(node)) return;
  }
  resetIteration(&node->intervalEvents);
  interval = nextData(&node->intervalEvents);
  while(interval) {
    clock_t current = clock();
    clock_t diff = current - interval->begin;
    if(diff < 0) {
      interval->begin = current;
    } else {
      if(interval->interval < (unsigned int)diff) {
        interval->begin = current;
        if(!interval->callback(node)) return;
      }
    }
    interval = nextData(&node->intervalEvents);
  }
	pushTransformation();
  if(node->isInterface) {
    translateTransformation((node->position[0] + node->scale[0] / 2.0F) / 50.0F - 1.0F, (node->position[1] + node->scale[1] / 2.0F) / 50.0F - 1.0F, 0.0F);
  } else {
    translateTransformation(node->position[0], node->position[1], node->position[2]);
  }
  rotateTransformation(node->angle[0], node->angle[1], node->angle[2]);
  resetIteration(&node->children);
  child = previousData(&node->children);
  while(child) {
    drawNode(child);
    child = previousData(&node->children);
  }
  if(node->isInterface) {
    scaleTransformation(node->scale[0] / 50.0F, node->scale[1] / 50.0F, 1.0F);
  } else {
    scaleTransformation(node->scale[0], node->scale[1], node->scale[2]);
  }
  clearAABB();
	fillPolygons(node->shape.vertices, node->shape.indices, node->texture, node->shape.uv, node->shape.uvIndices);
  getAABB(node->aabb);
  popTransformation();
}

int testCollision(Node a, Node b) {
  return (a.aabb[0][0] <= b.aabb[0][1] && a.aabb[0][1] >= b.aabb[0][0]) &&
         (a.aabb[1][0] <= b.aabb[1][1] && a.aabb[1][1] >= b.aabb[1][0]) &&
         (a.aabb[2][0] <= b.aabb[2][1] && a.aabb[2][1] >= b.aabb[2][0]);
}

void addIntervalEvent(Node *node, unsigned int milliseconds, int (*callback)(Node*)) {
  IntervalEvent *interval = malloc(sizeof(IntervalEvent));
  interval->begin = clock();
  interval->interval = milliseconds * CLOCKS_PER_SEC / 1000;
  interval->callback = callback;
  push(&node->intervalEvents, interval);
}

Shape initShapePlane(float width, float height, unsigned char color) {
  int i;
  float halfWidth = width / 2.0F;
  float halfHeight = height / 2.0F;
  Shape shape;
  static unsigned long generated_indices[] = { 0, 1, 2, 1, 3, 2 };
  Vertex generated_vertices[4];
  float generated_uv[][2] = {
    { 0.0F, 0.0F }, { 0.0F, 1.0F }, { 1.0F, 0.0F }, { 1.0F, 1.0F },
  };
  generated_vertices[0] = initVertex(-halfWidth, -halfHeight, 0.0F, color);
  generated_vertices[1] = initVertex(halfWidth, -halfHeight, 0.0F, color);
  generated_vertices[2] = initVertex(-halfWidth, halfHeight, 0.0F, color);
  generated_vertices[3] = initVertex(halfWidth, halfHeight, 0.0F, color);
  shape.indices = initVector();
  shape.vertices = initVector();
  shape.uv = initVector();
  shape.uvIndices = initVector();
  for(i = 0;i < 6;i++) {
    unsigned long *index = malloc(sizeof(unsigned long));
    unsigned long *uvIndex = malloc(sizeof(unsigned long));
    *index = generated_indices[i];
    *uvIndex = generated_indices[i];
    push(&shape.indices, index);
    push(&shape.uvIndices, uvIndex);
  }
  for(i = 0;i < 4;i++) {
    Vertex *vertex = malloc(sizeof(Vertex));
    float *coords = malloc(2 * sizeof(float));
    *vertex = generated_vertices[i];
    coords[0] = generated_uv[i][0];
    coords[1] = generated_uv[i][1];
    push(&shape.vertices, vertex);
    push(&shape.uv, coords);
  }
  return shape;
}

Shape initShapePlaneInv(float width, float height, unsigned char color) {
  int i;
  float halfWidth = width / 2.0F;
  float halfHeight = height / 2.0F;
  Shape shape;
  static unsigned long generated_indices[] = { 0, 1, 2, 1, 3, 2 };
  Vertex generated_vertices[4];
  float generated_uv[][2] = {
    { 0.0F, 0.0F }, { 0.0F, 1.0F }, { 1.0F, 0.0F }, { 1.0F, 1.0F },
  };
  shape.indices = initVector();
  shape.vertices = initVector();
  shape.uv = initVector();
  shape.uvIndices = initVector();
  generated_vertices[0] = initVertex(-halfWidth, -halfHeight, 0.0F, color);
  generated_vertices[1] = initVertex(-halfWidth, halfHeight, 0.0F, color);
  generated_vertices[2] = initVertex(halfWidth, -halfHeight, 0.0F, color);
  generated_vertices[3] = initVertex(halfWidth, halfHeight, 0.0F, color);
  for(i = 0;i < 6;i++) {
    unsigned long *index = malloc(sizeof(unsigned long));
    unsigned long *uvIndex = malloc(sizeof(unsigned long));
    *index = generated_indices[i];
    *uvIndex = generated_indices[i];
    push(&shape.indices, index);
    push(&shape.uvIndices, uvIndex);
  }
  for(i = 0;i < 4;i++) {
    Vertex *vertex = malloc(sizeof(Vertex));
    float *coords = malloc(2 * sizeof(float));
    *vertex = generated_vertices[i];
    coords[0] = generated_uv[i][0];
    coords[1] = generated_uv[i][1];
    push(&shape.vertices, vertex);
    push(&shape.uv, coords);
  }
  return shape;
}

Shape initShapeBox(float width, float height, float depth, unsigned char color) {
  int i;
  float halfWidth = width / 2.0F;
  float halfHeight = height / 2.0F;
  float halfDepth = depth / 2.0F;
  Shape shape;
  static unsigned long generated_indices[] = {
    0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6,
    8, 9, 10, 9, 11, 10, 12, 13, 14, 13, 15, 14,
    16, 17, 18, 17, 19, 18, 20, 21, 22, 21, 23, 22,
  };
  Vertex generated_vertices[24];
  float generated_uv[][2] = {
    { 0.0F, 0.0F }, { 0.0F, 1.0F }, { 1.0F, 0.0F }, { 1.0F, 1.0F },
    { 0.0F, 0.0F }, { 1.0F, 0.0F }, { 0.0F, 1.0F }, { 1.0F, 1.0F },
    { 0.0F, 0.0F }, { 0.0F, 1.0F }, { 1.0F, 0.0F }, { 1.0F, 1.0F },
    { 0.0F, 0.0F }, { 1.0F, 0.0F }, { 0.0F, 1.0F }, { 1.0F, 1.0F },
    { 0.0F, 0.0F }, { 0.0F, 1.0F }, { 1.0F, 0.0F }, { 1.0F, 1.0F },
    { 0.0F, 0.0F }, { 1.0F, 0.0F }, { 0.0F, 1.0F }, { 1.0F, 1.0F },
  };
  shape.indices = initVector();
  shape.vertices = initVector();
  shape.uv = initVector();
  shape.uvIndices = initVector();
  generated_vertices[0] = initVertex(-halfWidth, -halfHeight, halfDepth, color);
  generated_vertices[1] = initVertex(-halfWidth, halfHeight, halfDepth, color);
  generated_vertices[2] = initVertex(halfWidth, -halfHeight, halfDepth, color);
  generated_vertices[3] = initVertex(halfWidth, halfHeight, halfDepth, color);
  generated_vertices[4] = initVertex(-halfWidth, -halfHeight, -halfDepth, color);
  generated_vertices[5] = initVertex(halfWidth, -halfHeight, -halfDepth, color);
  generated_vertices[6] = initVertex(-halfWidth, halfHeight, -halfDepth, color);
  generated_vertices[7] = initVertex(halfWidth, halfHeight, -halfDepth, color);
  generated_vertices[8] = initVertex(halfWidth, -halfHeight, -halfDepth, color);
  generated_vertices[9] = initVertex(halfWidth, -halfHeight, halfDepth, color);
  generated_vertices[10] = initVertex(halfWidth, halfHeight, -halfDepth, color);
  generated_vertices[11] = initVertex(halfWidth, halfHeight, halfDepth, color);
  generated_vertices[12] = initVertex(-halfWidth, -halfHeight, -halfDepth, color);
  generated_vertices[13] = initVertex(-halfWidth, halfHeight, -halfDepth, color);
  generated_vertices[14] = initVertex(-halfWidth, -halfHeight, halfDepth, color);
  generated_vertices[15] = initVertex(-halfWidth, halfHeight, halfDepth, color);
  generated_vertices[16] = initVertex(-halfWidth, -halfHeight, -halfDepth, color);
  generated_vertices[17] = initVertex(-halfWidth, -halfHeight, halfDepth, color);
  generated_vertices[18] = initVertex(halfWidth, -halfHeight, -halfDepth, color);
  generated_vertices[19] = initVertex(halfWidth, -halfHeight, halfDepth, color);
  generated_vertices[20] = initVertex(-halfWidth, halfHeight, -halfDepth, color);
  generated_vertices[21] = initVertex(halfWidth, halfHeight, -halfDepth, color);
  generated_vertices[22] = initVertex(-halfWidth, halfHeight, halfDepth, color);
  generated_vertices[23] = initVertex(halfWidth, halfHeight, halfDepth, color);
  for(i = 0;i < 36;i++) {
    unsigned long *index = malloc(sizeof(unsigned long));
    unsigned long *uvIndex = malloc(sizeof(unsigned long));
    *index = generated_indices[i];
    *uvIndex = generated_indices[i];
    push(&shape.indices, index);
    push(&shape.uvIndices, uvIndex);
  }
  for(i = 0;i < 24;i++) {
    Vertex *vertex = malloc(sizeof(Vertex));
    float *coords = malloc(2 * sizeof(float));
    *vertex = generated_vertices[i];
    coords[0] = generated_uv[i][0];
    coords[1] = generated_uv[i][1];
    push(&shape.vertices, vertex);
    push(&shape.uv, coords);
  }
  return shape;
}

size_t getUntil(char *string, char separator, size_t index, char *out, size_t out_size) {
  size_t i = 0;
  if(string[index] == '\0') {
    out[0] = '\0';
    return 0;
  }
  for(;string[index] != '\0';index++) {
    if(string[index] != separator) break;
  }
  for(;string[index] != '\0';index++) {
    if(string[index] == separator || string[index] == '\n') {
      out[i] = '\0';
      break;
    }
    if(i < out_size - 1) {
      out[i] = string[index];
      i += 1;
    }
  }
  return index + 1;
}

int initShapeFromObj(Shape *shape, char *filename) {
  FILE *file;
  char buffer[OBJ_LINE_BUFFER_SIZE];
  char temp[OBJ_WORD_BUFFER_SIZE];
  size_t line = 1;
  shape->indices = initVector();
  shape->vertices = initVector();
  shape->uv = initVector();
  shape->uvIndices = initVector();
  if(fopen_s(&file, filename, "r")) {
    fputs("File not found.", stderr);
    fclose(file);
    return -1;
  }
  while(fgets(buffer, OBJ_LINE_BUFFER_SIZE, file)) {
    int i;
    size_t index = 0;
    size_t old_index;
    index = getUntil(buffer, ' ', index, temp, OBJ_WORD_BUFFER_SIZE);
    if(strcmp(temp, "v") == 0) {
      Vertex *vertex = calloc(sizeof(Vertex), 1);
      if(vertex == NULL) {
        fputs("readObj: Memory allocation failed.", stderr);
        fclose(file);
        return -2;
      }
      for(i = 0;i < 4;i++) {
        old_index = index;
        index = getUntil(buffer, ' ', index, temp, OBJ_WORD_BUFFER_SIZE);
        if(temp[0] == '\0') {
          if(i == 3) {
            vertex->components[3] = 1.0F;
          } else {
            fprintf(stderr, "readObj: Vertex component %d does not found. (%zu, %zu)", i, line, old_index);
            fclose(file);
            return -3;
          }
        } else {
          vertex->components[i] = (float)atof(temp);
        }
      }
      push(&shape->vertices, vertex);
    } else if(strcmp(temp, "vt") == 0) {
      float *coords = malloc(2 * sizeof(float));
      if(coords == NULL) {
        fputs("readObj: Memory allocation failed.", stderr);
        fclose(file);
        return -4;
      }
      for(i = 0;i < 2;i++) {
        old_index = index;
        index = getUntil(buffer, ' ', index, temp, OBJ_WORD_BUFFER_SIZE);
        if(temp[0] == '\0') {
          if(i != 0) {
            coords[1] = 0.0F;
          } else {
            fprintf(stderr, "readObj: Terxture cordinates' component %d does not found. (%zu, %zu)", i, line, old_index);
            fclose(file);
            return -5;
          }
        } else {
          if(i == 1) {
            coords[i] = 1.0F - (float)atof(temp);
          } else {
            coords[i] = (float)atof(temp);
          }
        }
      }
      push(&shape->uv, coords);
    } else if(strcmp(temp, "f") == 0) {
      unsigned long faceIndices[3];
      unsigned long faceUVIndices[3];
      for(i = 0;i < 3;i++) {
        old_index = index;
        index = getUntil(buffer, ' ', index, temp, OBJ_WORD_BUFFER_SIZE);
        if(temp[0] == '\0') {
          fprintf(stderr, "readObj: Vertex component %d does not found. (%zu, %zu)", i, line, old_index);
          fclose(file);
          return -6;
        } else {
          size_t index2 = 0;
          char temp2[OBJ_WORD_BUFFER_SIZE];
          index2 = getUntil(temp, '/', index2, temp2, OBJ_WORD_BUFFER_SIZE);
          faceIndices[i] = atoi(temp2);
          if(faceIndices[i] < 0) {
            faceIndices[i] += shape->vertices.length;
          } else {
            faceIndices[i] -= 1;
          }
          getUntil(temp, '/', index2, temp2, OBJ_WORD_BUFFER_SIZE);
          if(temp2[0] == '\0') {
            faceUVIndices[i] = 0;
          } else {
            faceUVIndices[i] = atoi(temp2);
            if(faceUVIndices[i] < 0) {
              faceUVIndices[i] += shape->uv.length;
            } else {
              faceUVIndices[i] -= 1;
            }
          }
        }
      }
      for(i = 2;i >= 0;i--) {
        unsigned long *faceIndex = malloc(sizeof(unsigned long));
        unsigned long *faceUVIndex = malloc(sizeof(unsigned long));
        if(faceIndex == NULL || faceUVIndex == NULL) {
          fputs("readObj: Memory allocation failed.", stderr);
          fclose(file);
          return -7;
        }
        *faceIndex = faceIndices[i];
        *faceUVIndex = faceUVIndices[i];
        push(&shape->indices, faceIndex);
        push(&shape->uvIndices, faceUVIndex);
      }
    } else if(temp[0] != '\0' && temp[0] != '#' && strcmp(temp, "vn") != 0 &&
              strcmp(temp, "vp") != 0 && strcmp(temp, "l") != 0 && strcmp(temp, "mtllib") != 0 &&
              strcmp(temp, "o") != 0 && strcmp(temp, "usemtl") != 0 && strcmp(temp, "s") != 0) {
      fprintf(stderr, "readObj: Unexpected word '%s' (%zu)", temp, line);
      fclose(file);
      return -8;
    }
    line += 1;
  }
  fclose(file);
  return 0;
}

void discardShape(Shape shape) {
  freeVector(&shape.indices);
  freeVector(&shape.vertices);
  freeVector(&shape.uv);
  freeVector(&shape.uvIndices);
}
