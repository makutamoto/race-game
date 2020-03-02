#ifndef COMMON_H
#define COMMON_H

#define SCREEN_HEIGHT (1 * 128)
#define SCREEN_WIDTH (1 * 180) // 1.414 * SCREEN_HEIGHT
#define FRAME_PER_SECOND 60

#define CAR_COLLISIONMASK 0x01
#define COURSE_COLLISIONMASK 0x02
#define LAP_COLLISIONMASK (LAPA_COLLISIONMASK | LAPB_COLLISIONMASK | LAPC_COLLISIONMASK)
#define LAPA_COLLISIONMASK 0x04
#define LAPB_COLLISIONMASK 0x08
#define LAPC_COLLISIONMASK 0x10
#define DIRT_COLLISIONMASK 0x20

extern FontSJIS shnm12, shnm16b;
extern Image carImage;
extern Node stageNode, courseNode;
extern Shape carShape, carCollisionShape;
extern WindowManager *rootManager;

#endif
