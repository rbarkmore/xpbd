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

#define WIDTH 640
#define HEIGHT 480
bool pause = true;
// If you don't want to include the code to capture the screen to an MP4 file,
// undefine the following
//#define CAPTURE_TO_MP4

#ifdef CAPTURE_TO_MP4
#include <stdio.h>
// Miles Macklin tells how to capture simulation video using ffmpeg:
// http://blog.mmacklin.com/2013/06/11/real-time-video-capture-with-ffmpeg/
// start ffmpeg telling it to expect raw rgba 720p-60hz frames
// -i - tells it to read frames from stdin
// the following will open a command prompt window on top of the desired simulation
// window.  Click on the simulation window to bring it to the top.  If you close the
// command prompt window, you will also close the call to ffmpeg, preventing capturing
// a video to the file, output.mp4
const char* cmd = "ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s 640x480 -i - "
                  "-threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip output.mp4";
//int mp4_width = WIDTH;
//int mp4_height = HEIGHT;
FILE* ffmpeg = NULL;
int* buffer = NULL;
bool capture = true;

#endif // CAPTURE_TO_MP4

int main(int argc, char* argv[]) {
  init(argc, argv);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(WIDTH, HEIGHT);
  glutCreateWindow("XPBD: Position-Based Simulation of Compliant Constrained Dynamics");

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special);

  glutMainLoop();

#ifdef CAPTURE_TO_MP4
_pclose(ffmpeg);
#endif // CAPTURE_TO_MP4

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
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_CULL_FACE);

#ifdef CAPTURE_TO_MP4
// open pipe to ffmpeg's stdin in binary write mode
  ffmpeg = _popen(cmd, "wb");
  buffer = new int[WIDTH*HEIGHT];
#endif // CAPTURE_TO_MP4

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
#ifdef CAPTURE_TO_MP4
  if(true == capture){
	  caption = std::stringstream();
	  caption << "Record MP4";
	  render_string(caption.str(), glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 10, 120);
  }
#endif // CAPTURE_TO_MP4

  glutSwapBuffers();

#ifdef CAPTURE_TO_MP4
   if(false == pause) {
     if(true == capture) {
       glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
       fwrite(buffer, sizeof(int)*WIDTH*HEIGHT, 1, ffmpeg);
     }
   }
#endif // CAPTURE_TO_MP4
};

void reshape(int width, int height){
  static GLfloat lightPosition[4] = {0.0f,  2.5f,  5.5f, 1.0f};
  static GLfloat lightDiffuse[3]  = {1.0f,  1.0f,  1.0f      };
  static GLfloat lightAmbient[3]  = {0.25f, 0.25f, 0.25f     };
  static GLfloat lightSpecular[3] = {1.0f,  1.0f,  1.0f      };

#ifdef CAPTURE_TO_MP4
// do not allow reshape to change window size, if we are outputting an MP4
width = WIDTH;
height = HEIGHT;
#endif // CAPTURE_TO_MP4

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
#ifdef CAPTURE_TO_MP4
	case 'C': case 'c':
        if(true == capture){
			capture = false;
		} else {
			capture = true;
		}
		break;
#endif // CAPTURE_TO_MP4
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

