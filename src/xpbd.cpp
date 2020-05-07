//
// Copyright(c) 2017 by Nobuo NAKAGAWA @ Polyphony Digital Inc.
//
// We're Hiring!
// http://www.polyphony.co.jp/recruit/
//
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

enum eMode {
  eModePBD,
  eModeXPBD_Graphene,
  eModeXPBD_Steel,
  eModeXPBD_Kevlar,
  eModeXPBD_Aluminum,
  eModeXPBD_Concrete,
  eModeXPBD_Wood,
  eModeXPBD_Leather,
  eModeXPBD_Tendon,
  eModeXPBD_Rubber,
  eModeXPBD_Muscle,
  eModeXPBD_Fat,
  eModeMax,
};

static const char* modeString[eModeMax] = {
  "PBD",
  "XPBD(Graphene)",
  "XPBD(Steel)",
  "XPBD(Kevlar)",
  "XPBD(Aluminum)",
  "XPBD(Concrete)",
  "XPBD(Wood)",
  "XPBD(Leather)",
  "XPBD(Tendon)",
  "XPBD(Rubber)",
  "XPBD(Muscle)",
  "XPBD(Fat)",
};

// Miles Macklin's blog (http://blog.mmacklin.com/2016/10/12/xpbd-slides-and-stiffness/)

// Compliance is the inverse of Young's Modulus.
// The units for Young's Modulus are N/M^2, so the units for Compliance are M^2/N
// For example, steel has a Young's Modulus of 200 gigapascals (GPa), or 200 x 10^(9) N/M^2

static const float modeCompliance[eModeMax] = {
  0.0f,                         //  space saved for PBD
  (float) (1.0f / 1.10E+012 ),  //	Graphene
  (float) (1.0f / 2.00E+011 ),  //	Steel
  (float) (1.0f / 1.12E+011 ),  //	Kevlar
  (float) (1.0f / 7.00E+010 ),  //	Aluminum
  (float) (1.0f / 2.50E+010 ),  //	Concrete
  (float) (1.0f / 6.00E+009 ),  //	Wood
  (float) (1.0f / 1.00E+008 ),  //	Leather
  (float) (1.0f / 5.00E+007 ),  //	Tendon
  (float) (1.0f / 1.00E+006 ),  //	Rubber
  (float) (1.0f / 5.00E+003 ),  //	Muscle
  (float) (1.0f / 1.00E+003 )   //	Fat
};

class CParticle{
 private:
  GLfloat   m_InvMass;
  glm::vec3 m_Position;
  glm::vec3 m_OldPosition;
  glm::vec3 m_Acceleration;

public:
  CParticle(GLfloat inv_mass, glm::vec3& position, glm::vec3& acceleration) :
  m_InvMass(inv_mass),
  m_Position(position),
  m_OldPosition(position),
  m_Acceleration(acceleration){}

   CParticle() :
   m_InvMass(1.0f) {}
  ~CParticle() {}

 void       Update(float t){
    if (m_InvMass > 0.0f){
      glm::vec3 tmp = m_Position;
      m_Position += (m_Position - m_OldPosition) + m_Acceleration * t * t;
      m_OldPosition = tmp;
    }
  }
  glm::vec3& GetPosition()  { return m_Position; }
  GLfloat&   GetInvMass()   { return m_InvMass;  }
  void       AddPosition(const glm::vec3& pos, bool is_force = true){
    if ((m_InvMass > 0.0f) || (is_force)) {
      m_Position += pos;
    }
  }
};

class CApplication{
private:
	int m_timeOldSim;
	int m_timeOldIdle;
	int m_msPerFrame; // default to 33 milliseconds per frame (1/FPS)
	int m_msSimStep;  // default to  simulation time step of 10 milliseocnds

	int   m_SolveTime;
public:
  int   m_IterationNum;
  int   m_Mode;
  int   m_OldMode;

  CApplication() :
   m_timeOldSim(0), m_timeOldIdle(0), m_msPerFrame(33), m_msSimStep(10),
   m_SolveTime(0), m_IterationNum(20), m_Mode(eModePBD), m_OldMode(eModeMax){}

   int  GetTimeOldSim(){ return m_timeOldSim; }
   void SetTimeOldSim(int sim){m_timeOldSim = sim; }
   int  GetTimeOldIdle(){ return m_timeOldIdle; }
   void SetTimeOldIdle(int idle){m_timeOldIdle = idle; }

   int   GetmsPerFrame() { return m_msPerFrame; }
   int   GetmsPerSimStep() { return m_msSimStep; }
   float GetdtPerSimStep() { return ( (float) m_msSimStep ) / 1000.0f; }
   
  int   GetSolveTime(){ return m_SolveTime; }
  void  SetSolveTime(int time){ m_SolveTime = time; }
};

class CConstraint{
private:
  GLfloat    m_RestLength;
  CParticle* m_Particle1;
  CParticle* m_Particle2;
  GLfloat    m_Stiffness;   // for  PBD(0.0f-1.0f)
  GLfloat    m_Compliance;  // for XPBD
  GLfloat    m_Lambda;      // for XPBD
public:
  CConstraint(CParticle* p0, CParticle* p1) :
  m_RestLength(0.0f),
  m_Particle1(p0),
  m_Particle2(p1),
  m_Stiffness(0.1f),
  m_Compliance(0.0f),
  m_Lambda(0.0f) {
    glm::vec3 p0_to_p1 = m_Particle2->GetPosition() - m_Particle1->GetPosition();
    m_RestLength = glm::length(p0_to_p1);
  }

  void LambdaInit() {
    m_Lambda = 0.0f; // reset every time frame
  }
  void Solve(CApplication& app, float dt){
    GLfloat   inv_mass1         = m_Particle1->GetInvMass();
    GLfloat   inv_mass2         = m_Particle2->GetInvMass();
    GLfloat   sum_mass          = inv_mass1 + inv_mass2;
    if (sum_mass == 0.0f) { return; }
    glm::vec3 p1_minus_p2       = m_Particle1->GetPosition() - m_Particle2->GetPosition();
    GLfloat   distance          = glm::length(p1_minus_p2);
    GLfloat   constraint        = distance - m_RestLength; // Cj(x)
    glm::vec3 correction_vector;
    if (app.m_Mode != eModePBD) { // XPBD
      m_Compliance = modeCompliance[app.m_Mode];
      m_Compliance /= dt * dt;    // a~
      GLfloat dlambda           = (-constraint - m_Compliance * m_Lambda) / (sum_mass + m_Compliance); // eq.18
              correction_vector = dlambda * p1_minus_p2 / (distance + FLT_EPSILON);                    // eq.17
      m_Lambda += dlambda;
    } else {                      // normal PBD
              correction_vector = m_Stiffness * glm::normalize(p1_minus_p2) * -constraint/ sum_mass;   // eq. 1
    }
    m_Particle1->AddPosition(+inv_mass1 * correction_vector);
    m_Particle2->AddPosition(-inv_mass2 * correction_vector);
  }
  float GetStiffness() { return m_Stiffness; }
};

class CBall{
private:
  float     m_Frequency;
  glm::vec3 m_Position;
  float     m_Radius;

public:
  CBall(float radius) :
  m_Frequency(3.14f * 0.4f),
  m_Position(0.0f,0.0f,0.0f),
  m_Radius(radius){}

  void Update(float dt){
    m_Position.z = cos(m_Frequency) * 2.0f;
    m_Frequency += dt / 5.0f;
    if (m_Frequency > 3.14f * 2.0f){ m_Frequency -= 3.14f * 2.0f; }
  }

  void Render(){
    glTranslatef(m_Position.x, m_Position.y, m_Position.z);
    const glm::vec3 color(0.0f, 0.0f, 1.0f);
    glColor3fv((GLfloat*)&color);
    glutSolidSphere(m_Radius, 30, 30);
  }

  glm::vec3& GetPosition(){ return m_Position; }
  float      GetRadius()  { return m_Radius;   }
};

class CCloth{
private:
  int                      m_Width;
  int                      m_Height;
  std::vector<CParticle>   m_Particles;
  std::vector<CConstraint> m_Constraints;
  
  CParticle* GetParticle(int w, int h) {return &m_Particles[ h * m_Width + w ];}
  void       MakeConstraint(CParticle* p1, CParticle* p2) { m_Constraints.push_back(CConstraint(p1, p2));}

  void DrawTriangle(CParticle* p1, CParticle* p2, CParticle* p3, const glm::vec3 color){
    glColor3fv((GLfloat*)&color);
    glVertex3fv((GLfloat*)&(p1->GetPosition()));
    glVertex3fv((GLfloat*)&(p2->GetPosition()));
    glVertex3fv((GLfloat*)&(p3->GetPosition()));
  }

public:
  CCloth(float width, float height, int num_width, int num_height):
  m_Width(num_width),
  m_Height(num_height) {
    m_Particles.resize(m_Width * m_Height);
    for(int w = 0; w < m_Width; w++){
      for(int h = 0; h < m_Height; h++){
        glm::vec3 pos( width  * ((float)w/(float)m_Width ) - width  * 0.5f,
                      -height * ((float)h/(float)m_Height) + height * 0.5f,
                       0.0f );
        glm::vec3 gravity( 0.0f, -9.8f, 0.0f );
        GLfloat inv_mass = 0.1f;
        if ((h == 0) && (w == 0)          ||
            (h == 0) && (w == m_Width - 1)) {
          inv_mass = 0.0f; //fix only edge point
        }
        m_Particles[ h * m_Width + w ] = CParticle(inv_mass, pos, gravity);
      }
    }
    for(int w = 0; w < m_Width; w++){
      for(int h = 0; h < m_Height; h++){           // structual constraint
        if (w < m_Width  - 1){ MakeConstraint(GetParticle(w, h), GetParticle(w+1, h  )); }
        if (h < m_Height - 1){ MakeConstraint(GetParticle(w, h), GetParticle(w,   h+1)); }
        if (w < m_Width  - 1 && h < m_Height - 1){ // shear constraint
          MakeConstraint(GetParticle(w,   h), GetParticle(w+1, h+1));
          MakeConstraint(GetParticle(w+1, h), GetParticle(w,   h+1));
        }
      }
    }
    for(int w = 0; w < m_Width; w++){
      for(int h = 0; h < m_Height; h++){           // bend constraint
        if (w < m_Width  - 2){ MakeConstraint(GetParticle(w, h), GetParticle(w+2, h  )); }
        if (h < m_Height - 2){ MakeConstraint(GetParticle(w, h), GetParticle(w,   h+2)); }
        if (w < m_Width  - 2 && h < m_Height - 2){
          MakeConstraint(GetParticle(w,   h), GetParticle(w+2, h+2));
          MakeConstraint(GetParticle(w+2, h), GetParticle(w,   h+2));
        }
      }
    }
  }
  ~CCloth(){}

  void Render(){
    glBegin(GL_TRIANGLES);
    int col_idx = 0;
    for(int w = 0; w < m_Width - 1; w++){
      for(int h = 0; h < m_Height - 1; h++){
        glm::vec3 col(1.0f, 0.6f, 0.6f);
        if ( col_idx++ % 2 ){ col = glm::vec3(1.0f, 1.0f, 1.0f);}
        DrawTriangle(GetParticle(w+1,h  ), GetParticle(w,   h), GetParticle(w, h+1), col);
        DrawTriangle(GetParticle(w+1,h+1), GetParticle(w+1, h), GetParticle(w, h+1), col);
      }
    }
    glEnd();
  }

  void Update(CApplication& app, float dt, CBall* ball, int iteration){
    int before = glutGet(GLUT_ELAPSED_TIME);
	std::vector<CParticle>::iterator particle;

    for(particle = m_Particles.begin(); particle != m_Particles.end(); particle++){
      (*particle).Update(dt); // predict position
    }
	std::vector<CConstraint>::iterator constraint;
    for(constraint = m_Constraints.begin(); constraint != m_Constraints.end(); constraint++){
      (*constraint).LambdaInit();
    }
    for(int i = 0; i < iteration; i++){
      for(particle = m_Particles.begin(); particle != m_Particles.end(); particle++){
        glm::vec3 vec    = (*particle).GetPosition() - ball->GetPosition();
        float     length = glm::length(vec);
        float     radius = ball->GetRadius() * 1.8f; // fake radius
        if (length < radius) {
          (*particle).AddPosition(glm::normalize(vec) * (radius - length));
        }
      }
      for(constraint = m_Constraints.begin(); constraint != m_Constraints.end(); constraint++){
        (*constraint).Solve(app, dt);
      }
    }
    app.SetSolveTime(glutGet(GLUT_ELAPSED_TIME) - before);
  }
};

CApplication g_Application;
CCloth       g_Cloth(2.0f, 2.0f, 20, 20);
CBall        g_Ball(0.1f);

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
		#ifdef LOG_TO_FILE
		outf << "etSim: " << etSim << std::endl;
		#endif // LOG_TO_FILE
		g_Application.SetTimeOldSim(timeNow);
		g_Ball.Update(dtSim);
        g_Cloth.Update(g_Application, dtSim, &g_Ball, g_Application.m_IterationNum);
	}

	if(etIdle >= g_Application.GetmsPerFrame() ){
		#ifdef LOG_TO_FILE
		outf << "                   etIdle: " << etIdle << std::endl;
		#endif // LOG_TO_FILE
		g_Application.SetTimeOldIdle(timeNow);
        glutPostRedisplay();
	}
}

void keyboard(unsigned char key , int x , int y){
  switch(key){
  case 27: exit(0); break; // esc
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
