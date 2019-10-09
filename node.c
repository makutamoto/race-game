#include<stdio.h>
#include<Windows.h>

#include<time.h>

#include "./include/node.h"
#include "./include/borland.h"
#include "./include/graphics.h"
#include "./include/vector.h"
#include "./include/matrix.h"

#define OBJ_LINE_BUFFER_SIZE 128
#define OBJ_WORD_BUFFER_SIZE 32

#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

Node initNode(const char *id, Image image) {
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

Node initNodeUI(const char *id, Image image, unsigned char color) {
  Node node;
  memset(&node, 0, sizeof(Node));
  memcpy_s(node.id, sizeof(node.id), id, min(sizeof(node.id), strlen(id)));
  node.scale[0] = 1.0F;
  node.scale[1] = 1.0F;
  node.scale[2] = 1.0F;
	node.texture = image;
  node.shape = initShapePlaneInv(1.0F, 1.0F, color);
  node.children = initVector();
  node.isInterface = TRUE;
  return node;
}

void discardNode(Node node) {
  clearVector(&node.children);
}

void drawNode(Node *node) {
  Node *child;
	pushTransformation();
  if(node->isInterface) {
    translateTransformation((node->position[0] + node->scale[0] / 2.0F) / 50.0F - 1.0F, (node->position[1] + node->scale[1] / 2.0F) / 50.0F - 1.0F, 0.0F);
  } else {
    translateTransformation(node->position[0], node->position[1], node->position[2]);
  }
  rotateTransformation(node->angle[0], node->angle[1], node->angle[2]);
  pushTransformation();
  if(node->isInterface) {
    clearCameraMat4();
    scaleTransformation(node->scale[0] / 50.0F, node->scale[1] / 50.0F, 1.0F);
  } else {
    scaleTransformation(node->scale[0], node->scale[1], node->scale[2]);
  }
  getTransformation(node->lastTransformation);
  clearAABB();
	fillPolygons(node->shape.vertices, node->shape.indices, node->texture, node->shape.uv, node->shape.uvIndices);
  getAABB(node->aabb);
  popTransformation();
  resetIteration(&node->children);
  child = previousData(&node->children);
  while(child) {
    drawNode(child);
    child = previousData(&node->children);
  }
  popTransformation();
}

int testCollision(Node a, Node b) {
  return (a.aabb[0][0] <= b.aabb[0][1] && a.aabb[0][1] >= b.aabb[0][0]) &&
         (a.aabb[1][0] <= b.aabb[1][1] && a.aabb[1][1] >= b.aabb[1][0]) &&
         (a.aabb[2][0] <= b.aabb[2][1] && a.aabb[2][1] >= b.aabb[2][0]);
}

int calcPlaneEquation(const float triangle[3][3], const float target[3][3], float n[3], float *d, float dv[3]) {
  float temp[2][3];
  cross(subVec3(triangle[1], triangle[0], temp[0]), subVec3(triangle[2], triangle[0], temp[1]), n);
  *d = dot3(mulVec3ByScalar(n, -1.0F, temp[0]), triangle[0]);
  dv[0] = dot3(n, target[0]) + *d;
  dv[1] = dot3(n, target[1]) + *d;
  dv[2] = dot3(n, target[2]) + *d;
  if(dv[0] != 0 && dv[1] != 0 && dv[2] != 0) {
    int signDv1 = sign(dv[1]);
    if(sign(dv[0]) == signDv1 && signDv1 == sign(dv[2])) {
      return -1;
    }
  }
  return 0;
}

void calcLineParameters(const float triangle[3][3], const float d[3], const float dv[3], float t[2]) {
  float pv[3];
  int signA, signB, signC;
  int indexA, indexB, indexC;
  signA = sign(dv[0]);
  signB = sign(dv[1]);
  signC = sign(dv[2]);
  if(signA == signB) {
    indexA = 0;
    indexB = 2;
    indexC = 1;
  } else if(signB == signC) {
    indexA = 1;
    indexB = 0;
    indexC = 2;
  } else {
    indexA = 0;
    indexB = 1;
    indexC = 2;
  }
  pv[0] = dot3(d, triangle[0]);
  pv[1] = dot3(d, triangle[1]);
  pv[2] = dot3(d, triangle[2]);
  t[0] = pv[indexA] + (pv[indexB] - pv[indexA]) * dv[indexA] / (dv[indexA] - dv[indexB]);
  t[1] = pv[indexC] + (pv[indexB] - pv[indexC]) * dv[indexC] / (dv[indexC] - dv[indexB]);
}

void projectTriangleOnAxes(const float triangle[3][3], int indexA, int indexB, float out[3][3]) {
  out[0][0] = triangle[0][indexA];
  out[0][1] = triangle[0][indexB];
  out[0][2] = 0.0F;
  out[1][0] = triangle[1][indexA];
  out[1][1] = triangle[1][indexB];
  out[1][2] = 0.0F;
  out[2][0] = triangle[2][indexA];
  out[2][1] = triangle[2][indexB];
  out[2][2] = 0.0F;
}

int testLine2dIntersection(const float lineA[2][3], const float lineB[2][3]) {
  float denominator;
  float uA, uB;
  denominator = (lineB[1][1] - lineB[0][1]) * (lineA[1][0] - lineA[0][0]) - (lineB[1][0] - lineB[0][0]) * (lineA[1][1] - lineA[0][1]);
  if(denominator == 0.0F) return FALSE;
  uA = ((lineB[1][0] - lineB[0][0]) * (lineA[0][1] - lineB[0][1]) - (lineB[1][1] - lineB[0][1]) * (lineA[0][0] - lineB[0][0])) / denominator;
  uB = ((lineA[1][0] - lineA[0][0]) * (lineA[0][1] - lineB[0][1]) - (lineA[1][1] - lineA[0][1]) * (lineA[0][0] - lineB[0][0])) / denominator;
  if((0.0F <= uA && uA <= 1.0F) && (0.0F <= uB && uB <= 1.0F)) return TRUE;
  return FALSE;
}

void edgesOfTriangle(const float triangle[3][3], float out[3][2][3]) {
  COPY_ARY(out[0][0], triangle[0]);
  COPY_ARY(out[0][1], triangle[1]);
  COPY_ARY(out[1][0], triangle[1]);
  COPY_ARY(out[1][1], triangle[2]);
  COPY_ARY(out[2][0], triangle[2]);
  COPY_ARY(out[2][1], triangle[0]);
}

int pointInTriangle(const float triangle[3][3], const float point[3]) {
  float temp[3][3];
  float zA, zB, zC;
  float signB;
  zA = cross(subVec3(triangle[1], triangle[0], temp[0]), subVec3(point, triangle[0], temp[1]), temp[2])[2];
  zB = cross(subVec3(triangle[2], triangle[1], temp[0]), subVec3(point, triangle[1], temp[1]), temp[2])[2];
  zC = cross(subVec3(triangle[0], triangle[2], temp[0]), subVec3(point, triangle[2], temp[1]), temp[2])[2];
  signB = sign(zB);
  return sign(zA) == signB && signB == sign(zC);
}

int testCollisionTriangleTriangle(const float a[3][3], const float b[3][3]) {
  // using Moller's algorithm: A Fast Triangle-Triangle Intersection Test
  float n1[3], n2[3];
  float d1, d2;
  float dv1[3], dv2[3];
  if(calcPlaneEquation(b, a, n2, &d2, dv2) || calcPlaneEquation(a, b, n1, &d1, dv1)) return FALSE;
  if(dv1[0] == 0 && dv1[1] == 0 && dv1[2] == 0) {
    int i;
    float triangleAOnAxes[3][3][3], triangleBOnAxes[3][3][3];
    float areas[3];
    float triangleAEdges[3][2][3], triangleBEdges[3][2][3];
    int triangleIndex;
    projectTriangleOnAxes(a, 0, 1, triangleAOnAxes[0]);
    projectTriangleOnAxes(a, 1, 2, triangleAOnAxes[1]);
    projectTriangleOnAxes(a, 2, 0, triangleAOnAxes[2]);
    projectTriangleOnAxes(a, 0, 1, triangleBOnAxes[0]);
    projectTriangleOnAxes(a, 1, 2, triangleBOnAxes[1]);
    projectTriangleOnAxes(a, 2, 0, triangleBOnAxes[2]);
    areas[0] = areaOfTriangle(triangleAOnAxes[0]);
    areas[1] = areaOfTriangle(triangleAOnAxes[1]);
    areas[2] = areaOfTriangle(triangleAOnAxes[2]);
    if(areas[0] > areas[1]) {
      if(areas[0] > areas[2]) {
        triangleIndex = 0;
      } else {
        triangleIndex = 2;
      }
    } else {
      if(areas[1] > areas[2]) {
        triangleIndex = 1;
      } else {
        triangleIndex = 2;
      }
    }
    edgesOfTriangle(triangleAOnAxes[triangleIndex], triangleAEdges);
    edgesOfTriangle(triangleBOnAxes[triangleIndex], triangleBEdges);
    for(i = 0;i < 3;i++) {
      if(testLine2dIntersection(triangleAEdges[i], triangleBEdges[0]) ||
        testLine2dIntersection(triangleAEdges[i], triangleBEdges[1]) ||
        testLine2dIntersection(triangleAEdges[i], triangleBEdges[2])) {
          return TRUE;
      }
    }
    if(pointInTriangle(triangleAOnAxes[triangleIndex], triangleBOnAxes[triangleIndex][0])
      || pointInTriangle(triangleBOnAxes[triangleIndex], triangleAOnAxes[triangleIndex][0])) return TRUE;
  } else {
    float d[3];
    float t1[2], t2[2];
    float t1MinMax[2], t2MinMax[2];
    cross(n1, n2, d);
    calcLineParameters(a, d, dv1, t1);
    calcLineParameters(b, d, dv2, t2);
    if(t1[0] > t1[1]) {
      t1MinMax[0] = t1[1];
      t1MinMax[1] = t1[0];
    } else {
      t1MinMax[0] = t1[0];
      t1MinMax[1] = t1[1];
    }
    if(t2[0] > t2[1]) {
      t2MinMax[0] = t2[1];
      t2MinMax[1] = t2[0];
    } else {
      t2MinMax[0] = t2[0];
      t2MinMax[1] = t2[1];
    }
    if(!(t1MinMax[1] < t2MinMax[0] || t2MinMax[1] < t1MinMax[0])) {
      return TRUE;
    }
  }
  return FALSE;
}

float (*mulMat4ByTriangle(float mat[4][4], float triangle[3][3], float out[3][3]))[3] {
  int i;
  for(i = 0;i < 3;i++) {
    float point[4];
    float transformed[4];
    COPY_ARY(point, triangle[i]);
    point[3] = 1.0F;
    mulMat4Vec4(mat, point, transformed);
    out[i][0] = transformed[0];
    out[i][1] = transformed[1];
    out[i][2] = transformed[2];
  }
  return out;
}

Vector* getPolygons(Node node, Vector *polygons) {
	size_t i1, i2;
	resetIteration(&node.shape.indices);
	for(i1 = 0;i1 < node.shape.indices.length / 3;i1++) {
		float *triangle = malloc(9 * sizeof(float));
		for(i2 = 0;i2 < 3;i2++) {
			unsigned long index = *(unsigned long*)nextData(&node.shape.indices);
      float *data = dataAt(&node.shape.vertices, index);
      triangle[i2 * 3] = data[0];
      triangle[i2 * 3 + 1] = data[1];
      triangle[i2 * 3 + 2] = data[2];
		}
    push(polygons, triangle);
	}
  return polygons;
}

int testCollisionPolygonPolygon(Node a, Node b) {
  Vector polygonsA = initVector();
  Vector polygonsB = initVector();
  float *polygonA, *polygonB;
  getPolygons(a, &polygonsA);
  getPolygons(b, &polygonsB);
  polygonA = nextData(&polygonsA);
  while(polygonA) {
    float polygonAWorld[3][3];
    mulMat4ByTriangle(a.lastTransformation, (float (*)[3])polygonA, polygonAWorld);
    resetIteration(&polygonsB);
    polygonB = nextData(&polygonsB);
    while(polygonB) {
      float polygonBWorld[3][3];
      mulMat4ByTriangle(b.lastTransformation, (float (*)[3])polygonB, polygonBWorld);
      if(testCollisionTriangleTriangle(polygonAWorld, polygonBWorld)) {
        freeVector(&polygonsA);
        freeVector(&polygonsB);
        return TRUE;
      }
      polygonB = nextData(&polygonsB);
    }
    polygonA = nextData(&polygonsA);
  }
  freeVector(&polygonsA);
  freeVector(&polygonsB);
  return FALSE;
}

void addIntervalEventNode(Node *node, unsigned int milliseconds, void (*callback)(Node*)) {
  IntervalEventNode *interval = malloc(sizeof(IntervalEventNode));
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
    { 1.0F, 1.0F }, { 0.0F, 1.0F }, { 1.0F, 0.0F }, { 0.0F, 0.0F },
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
