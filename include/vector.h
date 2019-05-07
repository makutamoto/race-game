#ifndef VECTOR_H
#define VECTOR_H

typedef struct _Vector {
	size_t length;
	struct _VectorItem *firstItem;
	struct _VectorItem *currentItem;
	struct _VectorItem *lastItem;
} Vector;

typedef struct _VectorItem {
	struct _VectorItem *previousItem;
	struct _VectorItem *nextItem;
	void *data;
} VectorItem;

Vector initVector();
void clearVector(Vector *vector);

void resetIteration(Vector *vector);
void* nextData(Vector *vector);
void* previousData(Vector *vector);
VectorItem* ItemAt(Vector vector, size_t index);
void* dataAt(Vector vector, size_t index);

BOOL push(Vector *vector, void *data);
void pop(Vector *vector);
BOOL insertAt(Vector *vector, size_t index, void *data);
void drainAt(Vector *vector, size_t index, void **data);
void removeAt(Vector *vector, size_t index);

#endif
