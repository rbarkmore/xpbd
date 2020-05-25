//
// Copyright(c) 2017 by Nobuo NAKAGAWA @ Polyphony Digital Inc.
//
// We're Hiring!
// http://www.polyphony.co.jp/recruit/
//

#include "main_xpbd.h"
#include "xpbd.hpp"

CApplication g_Application;
CCloth       g_Cloth(2.0f, 2.0f, 20, 20);
CBall        g_Ball(0.1f);

bool pause = true;

int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(640, 480);
  glutCreateWindow("XPBD: Position-Based Simulation of Compliant Constrained Dynamics");

  init(argc, argv);

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special);

  glutMainLoop();
  return 0;
}

void render_string(std::string& str, int w, int h, int x0, int y0) {
  glDisable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, w, h, 0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glRasterPos2f( (GLfloat) x0, (GLfloat) y0);
  int size = (int)str.size();
  for(int i = 0; i < size; ++i){
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, str[i]);
  }
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void init(int argc, char* argv[]){
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glEnable(GL_CULL_FACE);
}

void display(void){
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glDepthFunc(GL_LESS); 
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_NORMALIZE);

  glPushMatrix();
    g_Cloth.Render();
  glPopMatrix();

  glPushMatrix();
    g_Ball.Render();
  glPopMatrix();

  glColor3d(0.2f, 1.0f, 0.2f);
  std::stringstream caption;
  caption << modeString[g_Application.m_Mode];
  render_string(caption.str(), glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 10, 20);
  caption = std::stringstream();
  caption << "Iteration " << g_Application.m_IterationNum;
  render_string(caption.str(), glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 10, 40);
  caption = std::stringstream();
  caption << "dtSim: " << g_Application.GetdtPerSimStep();
  render_string(caption.str(), glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 10, 60);
  caption = std::stringstream();
  caption << "Solve Time " << g_Application.GetSolveTime() << " (ms)";
  render_string(caption.str(), glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 10, 80);
  if(true == pause){
	  caption = std::stringstream();
	  caption << "Paused";
	  render_string(caption.str(), glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 10, 100);
  }

  glutSwapBuffers();
};

void reshape(int width, int height){
  static GLfloat lightPosition[4] = {0.0f,  2.5f,  5.5f, 1.0f};
  static GLfloat lightDiffuse[3]  = {1.0f,  1.0f,  1.0f      };
  static GLfloat lightAmbient[3]  = {0.25f, 0.25f, 0.25f     };
  static GLfloat lightSpecular[3] = {1.0f,  1.0f,  1.0f      };

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glShadeModel(GL_SMOOTH);

  glViewport(0, 0, width, height);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(30.0, (double)width / (double)height, 0.0001f, 1000.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  gluLookAt(0.0f, 00.0f, 5.0f, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); // pos, tgt, up

  glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
  glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiffuse);
  glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmbient);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
};

// uncomment this define, if you want to log sim time and idle time to file
//#define LOG_TO_FILE
#ifdef LOG_TO_FILE
#include <iostream>
#include <fstream>
std::ofstream outf ( "log.txt" );
#endif // LOG_TO_FILE

void idle(void){
	int timeNow = glutGet(GLUT_ELAPSED_TIME);  // returns time since program started, in milliseconds

	int etSim  = timeNow - g_Application.GetTimeOldSim();  // elapsed time spent so far in simulation, use to control simulation to real time
	int etIdle = timeNow - g_Application.GetTimeOldIdle(); // elapsed time spent so far in idle, use to control when to render

	float dtSim = g_Application.GetdtPerSimStep();

    if(etSim >= g_Application.GetmsPerSimStep()){
		g_Application.SetTimeOldSim(timeNow);
		if(false == pause){
			g_Ball.Update(dtSim);
			g_Cloth.Update(g_Application, dtSim, &g_Ball, g_Application.m_IterationNum);
		}
	}

	if(etIdle >= g_Application.GetmsPerFrame() ){
		g_Application.SetTimeOldIdle(timeNow);
        glutPostRedisplay();
	}
}

void keyboard(unsigned char key , int x , int y){
	switch(key){
	case 27:   // esc
		exit(0);
		break;
	case 'P': case 'p':
        if(true == pause){
			pause = false;
		} else {
			pause = true;
		}
		break;
	case '+': {
		int step;
		step = g_Application.GetmsPerSimStep()  + 1;
		if(step > 1000) step = 1000;
		g_Application.SetmsPerSimStep(step);
		break;
		}
	case '-': {
		int step;
		step = g_Application.GetmsPerSimStep()  - 1;
		if(step < 1) step = 1;
		g_Application.SetmsPerSimStep(step);
		break;
		}
	}
}

void special(int key, int x, int y){
  if (key == GLUT_KEY_UP) {
    g_Application.m_IterationNum++;
  }
  if (key == GLUT_KEY_DOWN) {
    if (g_Application.m_IterationNum > 1){
      g_Application.m_IterationNum--;
    }
  }
  if (key == GLUT_KEY_LEFT) {
    if (g_Application.m_Mode > eModePBD) {
      g_Application.m_OldMode = g_Application.m_Mode;
      g_Application.m_Mode--;
    }
  }
  if (key == GLUT_KEY_RIGHT) {
    if (g_Application.m_Mode < eModeMax - 1) {
      g_Application.m_OldMode = g_Application.m_Mode;
      g_Application.m_Mode++;
    }
  }
}

