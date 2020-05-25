//
// Copyright(c) 2017 by Nobuo NAKAGAWA @ Polyphony Digital Inc.
//
// We're Hiring!
// http://www.polyphony.co.jp/recruit/
//

#ifndef _MAIN_XPBD_H
#define _MAIN_XPBD_H


#include <cstdlib>

#if defined(WIN32)
#include <GL/glut.h>
#ifndef _DEBUG
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif // _DEBUG
#elif defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#endif // MACOSX

#include <vector>
#include <string>
#include "glm/glm.hpp"

#include <sstream>

void render_string(std::string& str, int w, int h, int x0, int y0);
void init(int argc, char* argv[]);
void display(void);
void reshape(int width, int height);
void idle(void);
void keyboard(unsigned char key , int x , int y);
void special(int key, int x, int y);


#endif //  _MAIN_XPBD_H