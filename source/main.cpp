#include <nds.h>
#include <stdio.h>
#include <functional>

#include "MultipassEngine.h"
#include "RedPikmin.h"
#include "YellowPikmin.h"

volatile int frame = 0;

#define TEST_PIKMIN 20

using namespace std;

void init() {
  consoleDemoInit();

  printf("Multipass Engine Demo\n");
  
  videoSetMode(MODE_0_3D);
  glInit();
  glEnable(GL_TEXTURE_2D);
  
  // setup the rear plane
  glClearColor(1,1,0,31);
  glClearDepth(0x7FFF);
  
  glViewport(0,0,255,191);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  
  gluPerspective(70, 256.0 / 192.0, 0.1, 4096);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  gluLookAt(0.0, 5.0, 10.0,  //camera possition 
            0.0, 0.0, 0.0,   //look at
            0.0, 1.0, 0.0);  //up
  
  //Setup default lights; these may be overridden later
  glLight(0, RGB15(31,31,31) , floattov10(-0.5), floattov10(0.5), 0);
  glLight(1, RGB15(6,6,6)    , 0               , floattov10(-1.0), 0);
  glLight(2, RGB15(0,31,0) ,   floattov10(-1.0), 0,          0);
  glLight(3, RGB15(0,0,31) ,   floattov10(1.0) - 1,  0,          0);
  
  glPushMatrix();
}

constexpr v16 operator"" _v16(long double value) {
  return static_cast<v16>(value * 4096);
}

constexpr v16 operator"" _v16(unsigned long long value) {
  return static_cast<v16>(value * 4096);
}

constexpr int32 operator"" _f32(long double value) {
  return static_cast<int32>(value * 4096);
}

constexpr int32 operator"" _f32(unsigned long long value) {
  return static_cast<int32>(value * 4096);
}

constexpr int32 int32FromFloat(float value) {
  return static_cast<int32>(value * 4096);
}

constexpr float floatFromInt32(int32 value) {
  return static_cast<float>(value) / 4096;
}

constexpr v16 v16FromFloat(float value) {
  return static_cast<v16>(value * 4096);
}

void drawTriangleEntity(u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2, u8 r3, u8 g3, u8 b3) {
  glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
  glBegin(GL_TRIANGLE);
  glColor3b(r1, g1, b1);
  glVertex3v16(0, 1_v16, -0.25_v16);
  glColor3b(r2, g2, b2);
  glVertex3v16(0, 0.5_v16, 0.25_v16);
  glColor3b(r3, g3, b3);
  glVertex3v16(0, 0, -0.25_v16);
  glEnd();
}

void drawCaptain(u8 r, u8 g, u8 b) {
  drawTriangleEntity(r, g, b, 171, 109, 51, 245, 245, 175);
}

void drawRedCaptain() {
  drawCaptain(255, 0, 0);
}

void drawBlueCaptain() {
  drawCaptain(0, 0, 255);
}

void drawGridCell(u8 brightness) {
  glPolyFmt(POLY_ALPHA(0) | POLY_CULL_NONE);
  glBegin(GL_QUAD);
  glColor3b(brightness, brightness, brightness);
  glVertex3v16(0, 0, 0);
  glVertex3v16(0, 0, 1_v16);
  glVertex3v16(1_v16, 0, 1_v16);
  glVertex3v16(1_v16, 0, 0);
  glEnd();
}

void drawGrid() {
  glPushMatrix();

  s32 gridSize = 16;

  glTranslatef(-(gridSize >> 1), 0, -(gridSize >> 1));
  for (int x = 0; x < gridSize; ++x) {
    for (int z = 0; z < gridSize; ++z) {
      drawGridCell(32);
      glTranslatef(1, 0, 0);
    }
    glTranslatef(-gridSize, 0, 1);
  }

  glPopMatrix(1);
}

void drawCursor(u8 cursorR, u8 cursorG, u8 cursorB, u8 pointR, u8 pointG, u8 pointB) {
  glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
  glBegin(GL_QUAD);
  glColor3b(cursorR, cursorG, cursorB);
  glVertex3v16(-0.5_v16, 0.1_v16, -0.5_v16);
  glVertex3v16(-0.5_v16, 0.1_v16, 0.5_v16);
  glVertex3v16(0.5_v16, 0.1_v16, 0.5_v16);
  glVertex3v16(0.5_v16, 0.1_v16, -0.5_v16);
  glBegin(GL_TRIANGLE);
  glColor3b(pointR, pointG, pointB);
  glVertex3v16(-0.18_v16, 1.5_v16, 0);
  glVertex3v16(0.18_v16, 1.5_v16, 0);
  glVertex3v16(0, 1_v16, 0);
  glEnd();
}

void withTranslation(float x, float y, float z, function<void()> f) {
  glPushMatrix();
  glTranslatef(x, y, z);
  f();
  glPopMatrix(1);
}

void withScale(float x, float y, float z, function<void()> f) {
  glPushMatrix();
  glScalef(x, y, z);
  f();
  glPopMatrix(1);
}

void drawCircle(u32 segments) {
  float const radiansPerArc = 360.0 / segments;

  glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
  glBegin(GL_TRIANGLE);
  glColor3b(255, 0, 0);
  for (u32 i = 0; i < segments; ++i) {
    glPushMatrix();
    glRotateY(i * radiansPerArc);
    glVertex3v16(1_v16, 0, 0);
    glVertex3v16(1_v16, 0, 0);
    glRotateY(radiansPerArc);
    glVertex3v16(1_v16, 0, 0);
    glPopMatrix(1);
  }
  glEnd();
}

void drawVector(v16 x, v16 y, v16 z) {
  glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
  glBegin(GL_TRIANGLE);
  glColor3b(0, 255, 0);
  glVertex3v16(0, 0, 0);
  glVertex3v16(0, 0, 0);
  glVertex3v16(x, y, z);
  glEnd();
}

template<typename T>
struct Offset {
  T x, y, z;
};

struct Captain {
  Offset<float> cursor;
  float maxDistanceFromCursor = 4;
  float moveRate = 0.15f;
  Offset<float> position;
  s16 angle = 0;
};

Offset<float> delta(Offset<float> const& begin, Offset<float> const& end) {
  Offset<float> deltaVector;
  deltaVector.x = end.x - begin.x;
  deltaVector.y = end.y - begin.y;
  deltaVector.z = end.z - begin.z;
  return deltaVector;
}

int32 magnitude(Offset<float> const& vector) {
  Offset<int32> fixedPointVector{
    int32FromFloat(vector.x),
    int32FromFloat(vector.y),
    int32FromFloat(vector.z)
  };
  return sqrtf32(mulf32(fixedPointVector.x, fixedPointVector.x) +
      mulf32(fixedPointVector.y, fixedPointVector.y) +
      mulf32(fixedPointVector.z, fixedPointVector.z));
}

Offset<float> direction(Offset<float> const& begin, Offset<float> const& end) {
  Offset<float> deltaVector = delta(begin, end);
  int32 fixedPointDelta[] = {
    int32FromFloat(deltaVector.x),
    int32FromFloat(deltaVector.y),
    int32FromFloat(deltaVector.z)
  };
  normalizef32(fixedPointDelta);
  Offset<float> direction;
  direction.x = floatFromInt32(fixedPointDelta[0]);
  direction.y = floatFromInt32(fixedPointDelta[1]);
  direction.z = floatFromInt32(fixedPointDelta[2]);
  return direction;
}

void updateCaptain(Captain& captain) {
  if (keysCurrent() & KEY_LEFT) {
    captain.cursor.x -= captain.moveRate;
  }
  if (keysCurrent() & KEY_RIGHT) {
    captain.cursor.x += captain.moveRate;
  }
  if (keysCurrent() & KEY_UP) {
    captain.cursor.z -= captain.moveRate;
  }
  if (keysCurrent() & KEY_DOWN) {
    captain.cursor.z += captain.moveRate;
  }
  Offset<float> towardCursor = direction(captain.position, captain.cursor);
  int32 distanceFromCursor = magnitude(delta(captain.position, captain.cursor));
  if (int32FromFloat(captain.maxDistanceFromCursor) < distanceFromCursor) {
    captain.position.x += captain.moveRate * towardCursor.x;
    captain.position.y += captain.moveRate * towardCursor.y;
    captain.position.z += captain.moveRate * towardCursor.z;
  }
  // Do a dot product with straight forward (the direction the character faces
  // by defualt) and the direction of the cursor. Because two of the components
  // (x and y) are zero in the first vector, the only component that needs to be
  // multiplied for the dot product is the z.
  // Though, thinking about it, since the along-z axis vector is of unit length,
  // all this math is equivalent to just the z component of the "toward" vector.
  captain.angle = acosLerp(v16FromFloat(towardCursor.z)) *
      (towardCursor.x < 0 ? -1 : 1);
}

Captain redCaptain;

void gameloop() {
  //Example debug code; remove later?
  frame++;
  
  touchPosition touchXY;
  touchRead(&touchXY);
  scanKeys();

  updateCaptain(redCaptain);

  // print at using ansi escape sequence \x1b[line;columnH 
  //printf("\x1b[10;0HFrame = %d",frame);
  //printf("\x1b[16;0HTouch x = %04X, %04X\n", touchXY.rawx, touchXY.px);
  //printf("Touch y = %04X, %04X\n", touchXY.rawy, touchXY.py);

  drawGrid();
  withTranslation(redCaptain.position.x, redCaptain.position.y,
      redCaptain.position.z, []() {
        glPushMatrix();
        glRotateYi(redCaptain.angle);
        drawRedCaptain();
        glPopMatrix(1);
        Offset<float> towardCursor = direction(redCaptain.position, redCaptain.cursor);
        drawVector(v16FromFloat(towardCursor.x), v16FromFloat(towardCursor.y), v16FromFloat(towardCursor.z));
      });
  withTranslation(redCaptain.cursor.x, redCaptain.cursor.y, redCaptain.cursor.z,
      []() {
        drawCursor(255, 128, 0, 192, 192, 192);
        withScale(4, 4, 4, []() {
          drawCircle(16);
        });
      });
  glFlush(0);
  swiWaitForVBlank();
}

int main(void) {
  init();
  
  while(1) {
    gameloop();
  }

  return 0;
}
