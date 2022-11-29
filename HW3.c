/*
 *  HW 3
 *  Author: Connor Davel
 *  Date: 9/22/2022
 *  Sources: Lots of code is used from examples 1-8
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#ifdef USEGLEW
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h>
// Tell Xcode IDE to not gripe about OpenGL deprecation
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#else
#include <GL/glut.h>
#endif
//  Default resolution
//  For Retina displays compile with -DRES=2
#ifndef RES
#define RES 1
#endif

int th=0;     // view angles used in class
int ph=0;
double g_x = 0;
double g_y = 0;
double g_z = 0;

double fp_x = 0;
double fp_y = 0;
double fp_z = 0;

#define N 6000

//  Cosine and Sine in degrees
#define Cos(x) (cos((x)*3.14159265/180))
#define Sin(x) (sin((x)*3.14159265/180))

#define LEN 8192  //  Maximum length of text string
void Print(const char* format , ...)
{
   char    buf[LEN];
   char*   ch=buf;
   va_list args;
   //  Turn the parameters into a character string
   va_start(args,format);
   vsnprintf(buf,LEN,format,args);
   va_end(args);
   //  Display the characters one at a time at the current raster position
   while (*ch)
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*ch++);
}

/*
 *  Check for OpenGL errors
 */
void ErrCheck(const char* where)
{
   int err = glGetError();
   if (err) fprintf(stderr,"ERROR: %s [%s]\n",gluErrorString(err),where);
}

/*
 *  Print message to stderr and exit
 */
void Fatal(const char* format , ...)
{
   va_list args;
   va_start(args,format);
   vfprintf(stderr,format,args);
   va_end(args);
   exit(1);
}

typedef struct
{
   double p[3];
   double v[3];
   double mass;  
}Boid; 

Boid boids[N]; 

static void Vertex(double th,double ph, double r, double g, double b)
{
   glColor3f(r,g,b);
   glVertex3d(Sin(th)*Cos(ph) , Sin(ph) , Cos(th)*Cos(ph)); //polar -> cartesian coordinates
}

void sphere(double x, double y, double z, double x_scale, double y_scale, double z_scale, int rot) {
   //  Save transformation
   double r = 0;
   double g = 1; 
   double b = 0;
   glPushMatrix();
   //  Offset and scale
   glTranslated(x,y,z);
   glScaled(x_scale, y_scale, z_scale);

   //  South pole cap
   glBegin(GL_TRIANGLE_FAN);
   Vertex(0,-90, r,g,b);
   for (int th=0;th<=360;th+=rot)
   {
      Vertex(th,rot-90, r,g,b);
   }
   glEnd();

   //  Latitude bands
   for (int ph=rot-90;ph<=90-2*rot;ph+=rot)
   {
      glBegin(GL_QUAD_STRIP);
      for (int th=0;th<=360;th+=rot)
      {
         Vertex(th,ph, r,g,b);
         Vertex(th,ph+rot, r,g,b);
      }
      glEnd();
   }

   //  North pole cap
   glBegin(GL_TRIANGLE_FAN);
   Vertex(0,90, r,g,b);
   for (int th=0;th<=360;th+=rot)
   {
      Vertex(th,90-rot, r,g,b);
   }
   glEnd();

   //  Undo transformations
   glPopMatrix();
}

void cyl(double x, double y, double z, double dx, double dy, double dz, double r1, double r2) {
   double sx = dx - x;
   double sy = dy - y;
   double sz = dz - z; 
   double length = sqrt(sx*sx + sy*sy + sz*sz);
   //  Unit vector in direction of flght
   double D0 = sqrt(dx*dx+dy*dy+dz*dz);
   double X0 = dx/D0;
   double Y0 = dy/D0;
   double Z0 = dz/D0;
   //  Unit vector in "up" direction
   // 0 = ux*dx + uy*dy + uz*dz
   // ux = 0
   // uy = 1
   // 0 = dy + uz*dz
   // uz = -dy/dz 
   double ux;
   double uy;
   double uz; 
   if (dz == 0) {
      uz = 1;
      ux = 0;
      uy = 0; 
   }
   else {
      ux = 0;
      uy = 1; 
      uz = -dy/dz;
   }
   double D1 = sqrt(ux*ux+uy*uy+uz*uz);
   double X1 = ux/D1;
   double Y1 = uy/D1;
   double Z1 = uz/D1;
   //  Cross product gives the third vector
   double X2 = Y0*Z1-Y1*Z0;
   double Y2 = Z0*X1-Z1*X0;
   double Z2 = X0*Y1-X1*Y0;

   double mat[16];
   mat[0] = X0;   mat[4] = X1;   mat[ 8] = X2;   mat[12] = 0;
   mat[1] = Y0;   mat[5] = Y1;   mat[ 9] = Y2;   mat[13] = 0;
   mat[2] = Z0;   mat[6] = Z1;   mat[10] = Z2;   mat[14] = 0;
   mat[3] =  0;   mat[7] =  0;   mat[11] =  0;   mat[15] = 1;
   glPushMatrix();

   glTranslated(x,y,z);
   glMultMatrixd(mat);
   glColor3f(0.588, 0.294, 0);
   glBegin(GL_QUAD_STRIP);
   for (int th=0;th<=360;th+=60)
   {
      glVertex3d(length,r2*Cos(th),r2*Sin(th));
      glVertex3d(0,r1*Cos(th),r1*Sin(th));
   }
   glEnd();
   glBegin(GL_TRIANGLE_FAN);
   glVertex3d(length, 0.0, 0.0);
   for (int th=0;th<=360;th+=60)
      glVertex3d(length,r2*Cos(th),r2*Sin(th));
   glEnd();
   // cylinders do not need another end cap, since 
   // all should originate from inside the trunk of the tree

   glPopMatrix();
}
void leaf(double x, double y, double z, double dx, double dy, double dz, double ux, double uy, double uz, double s) {
   // double sx = dx - x;
   // double sy = dy - y;
   // double sz = dz - z; 
   // double length = sqrt(sx*sx + sy*sy + sz*sz);
   //  Unit vector in direction of flght
   double D0 = sqrt(dx*dx+dy*dy+dz*dz);
   double X0 = dx/D0;
   double Y0 = dy/D0;
   double Z0 = dz/D0;
   // up direction
   double D1 = sqrt(ux*ux+uy*uy+uz*uz);
   double X1 = ux/D1;
   double Y1 = uy/D1;
   double Z1 = uz/D1;
   //  Cross product gives the third vector
   double X2 = Y0*Z1-Y1*Z0;
   double Y2 = Z0*X1-Z1*X0;
   double Z2 = X0*Y1-X1*Y0;

   double mat[16];
   mat[0] = X0;   mat[4] = X1;   mat[ 8] = X2;   mat[12] = 0;
   mat[1] = Y0;   mat[5] = Y1;   mat[ 9] = Y2;   mat[13] = 0;
   mat[2] = Z0;   mat[6] = Z1;   mat[10] = Z2;   mat[14] = 0;
   mat[3] =  0;   mat[7] =  0;   mat[11] =  0;   mat[15] = 1;
   glPushMatrix();

   glTranslated(x,y,z);
   glMultMatrixd(mat);
   glScaled(s,s,s);
   glColor3f(0,1,0);
   glBegin(GL_QUADS);
   
   // glVertex3d(0, -0.1, 0);
   // glVertex3d(0, 0.1, 0);
   // glVertex3d(1, 0.1, 0);
   // glVertex3d(1, -0.1, 0);

   glVertex3d(0, 0, -0.15);
   glVertex3d(0, 0, 0.15);
   glVertex3d(5, 0, 0.1);
   glVertex3d(5, 0, -0.1);
   
   glEnd();
   glBegin(GL_TRIANGLE_FAN);
   glVertex3d(10, 0.0, 0.0);
   for (int th=0;th<=360;th+=60)
      glVertex3d(10+5*Cos(th),0, 5*Sin(th));
   glEnd();
   // cylinders do not need another end cap, since 
   // all should originate from inside the trunk of the tree

   glPopMatrix();
}

void branch(double x, double y, double z, double dx, double dy, double dz, double l, double r1) {
   double r2 = r1/2;
   // double sx = dx - x;
   // double sy = dy - y;
   // double sz = dz - z; 
   // double length = sqrt(sx*sx + sy*sy + sz*sz);
   //  Unit vector in direction of flght
   double D0 = sqrt(dx*dx+dy*dy+dz*dz);
   double X0 = dx/D0;
   double Y0 = dy/D0;
   double Z0 = dz/D0;
   //  Unit vector in "up" direction
   // 0 = ux*dx + uy*dy + uz*dz
   // ux = 0
   // uy = 1
   // 0 = dy + uz*dz
   // uz = -dy/dz 
   double ux;
   double uy;
   double uz; 
   if (dz == 0) {
      uz = 1;
      ux = 0;
      uy = 0; 
   }
   else {
      ux = 0;
      uy = 1; 
      uz = -dy/dz;
   }
   double D1 = sqrt(ux*ux+uy*uy+uz*uz);
   double X1 = ux/D1;
   double Y1 = uy/D1;
   double Z1 = uz/D1;
   //  Cross product gives the third vector
   double X2 = Y0*Z1-Y1*Z0;
   double Y2 = Z0*X1-Z1*X0;
   double Z2 = X0*Y1-X1*Y0;

   double mat[16];
   mat[0] = X0;   mat[4] = X1;   mat[ 8] = X2;   mat[12] = 0;
   mat[1] = Y0;   mat[5] = Y1;   mat[ 9] = Y2;   mat[13] = 0;
   mat[2] = Z0;   mat[6] = Z1;   mat[10] = Z2;   mat[14] = 0;
   mat[3] =  0;   mat[7] =  0;   mat[11] =  0;   mat[15] = 1;
   glPushMatrix();

   glTranslated(x,y,z);
   glMultMatrixd(mat);
   glColor3f(0.588, 0.294, 0);
   glBegin(GL_QUAD_STRIP);
   for (int th=0;th<=360;th+=60)
   {
      glVertex3d(l,r2*Cos(th),r2*Sin(th));
      glVertex3d(0,r1*Cos(th),r1*Sin(th));
   }
   glEnd();
   glBegin(GL_TRIANGLE_FAN);
   glVertex3d(l, 0.0, 0.0);
   for (int th=0;th<=360;th+=60)
      glVertex3d(l, r2*Cos(th),r2*Sin(th));
   glEnd();
   int th;
   for (int i=0; i<10; i++){
      th = i * 140;
      leaf((i/10.)*l,0,0, l,Cos(th),Sin(th), 1,0,0, 0.5*r1);
   }
   for (int i=0; i<5; i++){
      th = i * 160;
      leaf(l,0,0, i+1,Cos(th),Sin(th), 1,1,0, 0.5*r1);
   }
   // cylinders do not need another end cap, since 
   // all should originate from inside the trunk of the tree

   glPopMatrix();
}

void cone(double x, double y, double z, double h, double r, int rot, int overall_rot) {
   //  Save transformation
   glPushMatrix();

   glColor3f(0,1,0);
   //  Offset and scale
   glTranslated(x,y,z);
   glRotatef(overall_rot,0,1,0);
   glBegin(GL_TRIANGLE_FAN);
   glVertex3d(0,h,0);
   for (int th=0;th<=360;th+=rot)
   {
      glVertex3d(r*Cos(th),0,r*Sin(th));
   }
   glEnd();
   glBegin(GL_TRIANGLE_FAN);
   glVertex3d(0,0,0);
   for (int th=0;th<=360;th+=rot)
   {
      glVertex3d(r*Cos(th),0,r*Sin(th));
   }
   glEnd();

   //  Undo transformations
   glPopMatrix();
}

void branches(double x, double y, double z, double dx, double dy, double dz, int stop, int current, double angle_factor) {
   // double sx = dx - x;
   // double sy = dy - y;
   // double sz = dz - z; 
   // double length = sqrt(sx*sx + sy*sy + sz*sz);
   //  Unit vector in direction of flght
   double D0 = sqrt(dx*dx+dy*dy+dz*dz);
   double X0 = dx/D0;
   double Y0 = dy/D0;
   double Z0 = dz/D0;
   //  Unit vector in "up" direction
   // 0 = ux*dx + uy*dy + uz*dz
   // ux = 0
   // uy = 1
   // 0 = dy + uz*dz
   // uz = -dy/dz 
   double ux;
   double uy;
   double uz; 
   if (dz == 0) {
      uz = 1;
      ux = 0;
      uy = 0; 
   }
   else {
      ux = 0;
      uy = 1; 
      uz = -dy/dz;
   }
   double D1 = sqrt(ux*ux+uy*uy+uz*uz);
   double X1 = ux/D1;
   double Y1 = uy/D1;
   double Z1 = uz/D1;
   //  Cross product gives the third vector
   double X2 = Y0*Z1-Y1*Z0;
   double Y2 = Z0*X1-Z1*X0;
   double Z2 = X0*Y1-X1*Y0;

   double mat[16];
   mat[0] = X0;   mat[4] = X1;   mat[ 8] = X2;   mat[12] = 0;
   mat[1] = Y0;   mat[5] = Y1;   mat[ 9] = Y2;   mat[13] = 0;
   mat[2] = Z0;   mat[6] = Z1;   mat[10] = Z2;   mat[14] = 0;
   mat[3] =  0;   mat[7] =  0;   mat[11] =  0;   mat[15] = 1;
   glPushMatrix();

   glTranslated(x,y,z);
   glMultMatrixd(mat);
   
   if (current >= stop){
      int th;
      double current_f = current;
      for (int i=4; i<=5; i++){
         th = i * 135;
         // leaf(i/10,0,0, 2,Cos(th),Sin(th), 1,0,0, 0.03);
         branch(0,0,0, angle_factor,Cos(th),Sin(th), 0.25, 0.1*(1.0-((current_f-1)/stop)));
      }
   }
   else {
      double current_f = current;
      double r1 = 0.1*(1.0-((current_f-1)/stop));
      double r2 = 0.1*(1.0-((current_f)/stop));
      cyl(0,0,0, 0.5,0,0, r1, r2);
      int th;
      // double rand_f;
      int i; 
      for (i=3; i<=4; i++){
         th = i * 135;
         // leaf(i/10,0,0, 2,Cos(th),Sin(th), 1,0,0, 0.03);
         branches(0.5*(i/5.),0,0, angle_factor,Cos(th),Sin(th), stop, current+1, angle_factor);
      }
      th = i * 135;
      branches(0.5,0,0, angle_factor,Cos(th),Sin(th), stop, current+1, angle_factor);
   }
   
   // cylinders do not need another end cap, since 
   // all should originate from inside the trunk of the tree

   glPopMatrix();
}

void trunk_seg(double wid, double wid2, double h1, double h2) {
   glColor3f(0.588, 0.294, 0);
   glBegin(GL_QUAD_STRIP);
   for (int th=0;th<=360;th+=30)
   {
      glVertex3d(wid*Cos(th),h1,wid*Sin(th));
      glVertex3d(wid2*Cos(th),h2,wid2*Sin(th));
   }
   glEnd();
}

void tree1_simple(double x, double y, double z, int branch_rot, int overall_rot, double scale){
   glPushMatrix();
   glTranslated(x,y,z);
   glTranslated(0,1.4,0);
   glRotatef(overall_rot, 0,1,0);
   glScaled(scale,scale,scale);
   for (int i = 0; i<=10;i++){
      int scale = branch_rot * i;
      double y_offset = i / 10.0;
      glRotatef(scale,0,1,0);
      cyl(0,y_offset,0, 0,1+y_offset,1 , 0.1, 0.1);
      sphere(0,1+y_offset,1, 1,.4,1, 45);
   }
   glPopMatrix();

   glPushMatrix();
   glTranslated(x,y,z);
   double wid = 0.5;
   double wid2 = 0.3;
   double height = 0.2;
   trunk_seg(wid, wid2, 0, height); 
   glTranslated(0,height,0);
   wid = wid2;
   wid2 = 0.2;
   height = 0.4;
   trunk_seg(wid, wid2, 0, height); 
   glTranslated(0,height,0);
   wid = wid2;
   wid2 = 0.15;
   height = 1.9;
   trunk_seg(wid, wid2, 0, height); 
   sphere(0,height,0, 1,.4,1, 45);

   glPopMatrix();
}

void tree1(double x, double y, double z, int branch_rot, int overall_rot, double scale){
   glPushMatrix();
   glTranslated(x,y,z);
   glTranslated(0,1.4,0);
   glRotatef(overall_rot, 0,1,0);
   glScaled(scale,scale,scale);
   for (int i = 0; i<=5;i++){
      int scale = branch_rot * i;
      double y_offset = 0.15+i / 5.0;
      glRotatef(scale,0,1,0);
      branches(0,y_offset,0, 0,1+y_offset,1, 5-(i/2), 1, 2);
      // cyl(0,y_offset,0, 0,1+y_offset,1 , 0.1, 0.1);
      // sphere(0,1+y_offset,1, 1,.4,1, 45);
   }
   glPopMatrix();

   glPushMatrix();
   glTranslated(x,y,z);
   glScaled(scale,scale,scale);
   double wid = 0.3;
   double wid2 = 0.15;
   trunk_seg(wid, wid2, 0, 0.2); 

   wid = wid2;
   wid2 = 0.1;
   trunk_seg(wid, wid2, 0.2, 0.6); 

   wid = wid2;
   wid2 = 0.02;
   trunk_seg(wid, wid2, 0.6, 2.5);

   glPopMatrix();
}

void bush(double x, double y, double z, int branch_rot, int overall_rot, double scale){
   glPushMatrix();
   glTranslated(x,y,z);
   glTranslated(0,0,0);
   glRotatef(overall_rot, 0,1,0);
   glScaled(scale,scale,scale);
   branches(0,0,0, 0,1,0, 4, 0.2, 0.5);
   glPopMatrix();
}

void tree2_simple(double x, double y, double z, int overall_rot, double r, double scale){
   // cone(0,1,0, 1, 0.5, 60, 11);
   glPushMatrix();
   glTranslated(x,y,z);
   glTranslated(0,1.4,0);
   glRotatef(overall_rot, 0,1,0);
   glScaled(scale,scale,scale);
   double cone_height = 1.3;
   for (int i = 0; i<=4;i++){
      int scale = 8 * i;
      cone_height -= (cone_height/6);
      r -= (r/8);
      double y_offset = i / 4.0;
      glRotatef(scale,0,1,0);
      cone(0,y_offset,0, 1, r, 60, 0);
   }
   glPopMatrix();

   glPushMatrix();
   glTranslated(x,y,z);
   glScaled(scale,scale,scale);
   double wid = 0.35;
   double wid2 = 0.15;
   double height = 0.2;
   trunk_seg(wid, wid2, 0, height); 
   glTranslated(0,height,0);
   wid = wid2;
   wid2 = 0.1;
   height = 0.4;
   trunk_seg(wid, wid2, 0, height); 
   glTranslated(0,height,0);
   wid = wid2;
   wid2 = 0.08;
   height = 1.9;
   trunk_seg(wid, wid2, 0, height); 

   glPopMatrix();
}

void fence_plank(double x1, double y1, double z1, double x2, double y2, double z2, double s) {
   double dx = x2-x1;
   double dy = y2-y1; 
   double dz = z2-z1; 
   // double sx = dx - x1;
   // double sy = dy - y1;
   // double sz = dz - z1; 
   // double length = sqrt(sx*sx + sy*sy + sz*sz);
   double length = sqrt(dx*dx + dy*dy + dz*dz);
   //  Unit vector in direction of flght
   double D0 = sqrt(dx*dx+dy*dy+dz*dz);
   double X0 = dx/D0;
   double Y0 = dy/D0;
   double Z0 = dz/D0;
   //  Unit vector in "up" direction
   // 0 = ux*dx + uy*dy + uz*dz
   // ux = 0
   // uy = 1
   // 0 = dy + uz*dz
   // uz = -dy/dz 
   double ux;
   double uy;
   double uz; 
   uz = 0;
   ux = 0;
   uy = 1; 

   double D1 = sqrt(ux*ux+uy*uy+uz*uz);
   double X1 = ux/D1;
   double Y1 = uy/D1;
   double Z1 = uz/D1;
   //  Cross product gives the third vector
   double X2 = Y0*Z1-Y1*Z0;
   double Y2 = Z0*X1-Z1*X0;
   double Z2 = X0*Y1-X1*Y0;

   double mat[16];
   mat[0] = X0;   mat[4] = X1;   mat[ 8] = X2;   mat[12] = 0;
   mat[1] = Y0;   mat[5] = Y1;   mat[ 9] = Y2;   mat[13] = 0;
   mat[2] = Z0;   mat[6] = Z1;   mat[10] = Z2;   mat[14] = 0;
   mat[3] =  0;   mat[7] =  0;   mat[11] =  0;   mat[15] = 1;
   glPushMatrix();

   glTranslated(x1,y1,z1);
   glMultMatrixd(mat);
   glScaled(1,s,s);
   glColor3f(0.588, 0.294, 0);
   glBegin(GL_QUAD_STRIP);
   for (int th=45;th<=405;th+=90)
   {
      glVertex3d(length,0.5*Cos(th),0.1*Sin(th));
      glVertex3d(0,0.5*Cos(th),0.1*Sin(th));
   }
   glEnd();
   glBegin(GL_TRIANGLE_FAN);
   glVertex3d(length, 0.0, 0.0);
   for (int th=45;th<=405;th+=90)
      glVertex3d(length,0.5*Cos(th),0.1*Sin(th));
   glEnd();
    glBegin(GL_TRIANGLE_FAN);
   glVertex3d(0, 0.0, 0.0);
   for (int th=45;th<=405;th+=90)
      glVertex3d(0,0.5*Cos(th),0.1*Sin(th));
   glEnd();
   // cylinders do not need another end cap, since 
   // all should originate from inside the trunk of the tree

   glPopMatrix();
}

void fence_post(double x, double y, double z, double s) {
   
   //  Save transformation
   glPushMatrix(); 
   glColor3f(0.588, 0.294, 0);
   glTranslated(x,y,z); 
   glBegin(GL_QUAD_STRIP);
   for (int th=45;th<=405;th+=90)
   {
      glVertex3d(0.5*Cos(th),0,0.5*Sin(th));
      glVertex3d(0.5*Cos(th),s,0.5*Sin(th));
   }
   glEnd();
   glBegin(GL_TRIANGLE_FAN);
   glVertex3d(0.0, s,0.0);
   for (int th=45;th<=405;th+=90)
      glVertex3d(0.5*Cos(th),s,0.5*Sin(th));
   glEnd();
   glPopMatrix(); 

   // chain fence post from previous 
   if (!(fp_x == 0 && fp_y == 0 && fp_z == 0)) {
      fence_plank(fp_x, fp_y+(10*s/27), fp_z, x,y+(10*s/27),z, 1);
      fence_plank(fp_x, fp_y+(19*s/27), fp_z, x,y+(19*s/27),z, 1);
   }
   fp_x = x;
   fp_y = y;
   fp_z = z; 
   

}

double dot(double x1, double y1, double z1, double x2, double y2, double z2) {
   return x1*x2 + y1*y2 + z1*z2;
}
double norm(double dx,double dy,double dz) {
   return sqrt(dx*dx + dy*dy + dz*dz);
}

void init_boids() {
   for (int i=0; i<N; i++){
      double x = rand()/(0.5*RAND_MAX)-1; 
      double y = rand()/(0.5*RAND_MAX)-1;
      double z = rand()/(0.5*RAND_MAX)-1; 
      Boid temp = {{x,y,z}, {0,0,0}, 1};
      boids[i] = temp; 
   }
}
void update() {
   double sight_radius = 10; 
   double avoid_radius = 0.5;
   double dt = 0.1;
   #pragma omp parallel for
   for (int k=0;k<N;k++) {
      // for a single boid k:
      Boid current = boids[k];
      double m = current.mass; 
      double cx = current.p[0];
      double cy = current.p[1];
      double cz = current.p[2];
      double cvx = current.v[0];
      double cvy = current.v[1];
      double cvz = current.v[2];
      double current_velocity_norm = norm(cvx, cvy, cvz);
      if (current_velocity_norm != 0) {
         cvx = cvx / current_velocity_norm;
         cvy = cvy / current_velocity_norm;
         cvz = cvz / current_velocity_norm;
      }

      double containment = 0.1; 
      double Fx = -cx*containment;
      double Fy = -cy*containment;
      double Fz = -cz*containment; 
      double total_x = 0;
      double total_y = 0;
      double total_z = 0;

      double total_xa = 0;
      double total_ya = 0;
      double total_za = 0;

      double total_vx = 0;
      double total_vy = 0;
      double total_vz = 0; 

      int total_neighbors = 0;
      for (int i=0;i<N;i++) {
         // for a single boid k:
         if (i == k) {
            continue;
         }
         Boid neighbor = boids[i];
         double dx = neighbor.p[0]-cx;
         double dy = neighbor.p[1]-cy;
         double dz = neighbor.p[2]-cz;
         double nvx = neighbor.v[0];
         double nvy = neighbor.v[1];
         double nvz = neighbor.v[2];

         
         double d = sqrt(dx*dx + dy*dy + dz*dz);
         if (d > sight_radius) {
            continue;
         }
         total_neighbors += 1; 
         // force calculations: attraction and replusion
         if (d < avoid_radius) {
            total_xa += dx;
            total_ya += dy;
            total_za += dz; 
         }
         total_x += dx;
         total_y += dy;
         total_z += dz; 

         // force calculation: alignment
         total_vx += nvx;
         total_vy += nvy;
         total_vz += nvz;
      }
      if (total_neighbors == 0) {
         continue; 
      }
      // fprintf(stdout, "(%f, %f, %f)",  total_x,total_y,total_z);
      double dot_product;


      double avg_x = total_x;
      double avg_y = total_y;
      double avg_z = total_z;
      double distance_norm = norm(avg_x, avg_y, avg_z); 
      if (distance_norm > 0) {
         avg_x = avg_x/distance_norm;
         avg_y = avg_y/distance_norm;
         avg_z = avg_z/distance_norm;
         dot_product = dot(avg_x,avg_y,avg_z, cvx, cvy, cvz);
         Fx += 0.1 * (avg_x - cvx*dot_product); 
         Fy += 0.1 * (avg_y - cvy*dot_product); 
         Fz += 0.1 * (avg_z - cvz*dot_product); 
      }

      double avg_xa = total_xa;
      double avg_ya = total_ya;
      double avg_za = total_za;
      double avoid_distance_norm = norm(avg_xa, avg_ya, avg_za); 
      if (avoid_distance_norm > 0) {
         avg_xa = avg_xa/avoid_distance_norm;
         avg_ya = avg_ya/avoid_distance_norm;
         avg_za = avg_za/avoid_distance_norm;
         dot_product = dot(avg_xa,avg_ya,avg_za, cvx, cvy, cvz);
         Fx -= 0.3 * (avg_xa - cvx*dot_product); 
         Fy -= 0.3 * (avg_ya - cvy*dot_product); 
         Fz -= 0.3 * (avg_za - cvz*dot_product); 
      }
      

      double avg_vx = total_vx;
      double avg_vy = total_vy;
      double avg_vz = total_vz;
      double velocity_norm = norm(avg_vx, avg_vy, avg_vz); 
      if (velocity_norm > 0) {
         avg_vx = avg_vx/velocity_norm;
         avg_vy = avg_vy/velocity_norm;
         avg_vz = avg_vz/velocity_norm;
         dot_product = dot(avg_vx,avg_vy,avg_vz, cvx, cvy, cvz);
         Fx += 0.3 * (avg_vx - cvx*dot_product); 
         Fy += 0.3 * (avg_vy - cvy*dot_product); 
         Fz += 0.3 * (avg_vz - cvz*dot_product); 
      }

      

      // fprintf(stdout, "(%f, %f, %f)",  distance_norm, velocity_norm, dot_product);
   
      
      // apply force
      current.v[0] = cvx + Fx/m*dt;
      current.v[1] = cvy + Fy/m*dt;
      current.v[2] = cvz + Fz/m*dt;

      current.p[0] = cx + dt*current.v[0];
      current.p[1] = cy + dt*current.v[1];
      current.p[2] = cz + dt*current.v[2];
      boids[k] = current; 
   }
}

void render_boids() {
   glBegin(GL_POINTS);
   for (int i=0; i<N; i++){
      Boid temp = boids[i];
      double x = temp.p[0];
      double y = temp.p[1];
      double z = temp.p[2];
      // sphere(x,y,z, 0.1, 0.1, 0.1, 0); 
      
      glVertex3d(x,y,z);
      
   }
   glEnd();
}
/*
 *  OpenGL (GLUT) calls this routine to display the scene
 */

void display()
{
   //  Erase the window and the depth buffer
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   //  Enable Z-buffering in OpenGL
   glEnable(GL_DEPTH_TEST);
   //  Undo previous transformations
   glLoadIdentity();
   //  Set view angle
   glRotatef(ph,1,0,0);
   glRotatef(th,0,1,0);
   glTranslated(g_x,g_y,g_z);

   update(); 
   render_boids();

   // tree1(0,0,0,55, 0, 6);
   // bush(0,0,0,60, 0, 6);
   // tree2(-1.9,0,3, 0, 0.9, 1);
   // branch(0,0,0, -1,-1,-1, 1,0.8);
   // fence_plank(0,0,0, 1,0.5,5, 1);
   // fence_post(1,0,0, 3); 
   // fence_post(4,0,0, 3); 
   // fence_post(4,0,4, 3); 
   // fence_post(0,0,4, 3); 
   // fence_post(8,0,4, 3); 


   glColor3f(1,1,1);
   //  Draw axes
   if (1)
   {
      const double len=1.5;  //  Length of axes
      glBegin(GL_LINES);
      glVertex3d(0.0,0.0,0.0);
      glVertex3d(len,0.0,0.0);
      glVertex3d(0.0,0.0,0.0);
      glVertex3d(0.0,len,0.0);
      glVertex3d(0.0,0.0,0.0);
      glVertex3d(0.0,0.0,len);
      glEnd();
      //  Label axes
      glRasterPos3d(len,0.0,0.0);
      Print("X");
      glRasterPos3d(0.0,len,0.0);
      Print("Y");
      glRasterPos3d(0.0,0.0,len);
      Print("Z");
   }

   
   //  Render the scene
   ErrCheck("display");
   glFlush();
   glutSwapBuffers();
}

/*
 *  GLUT calls this routine when an arrow key is pressed
 */
void special(int key,int x,int y)
{
   //  Right arrow key - increase angle by 5 degrees
   if (key == GLUT_KEY_RIGHT)
      th += 5;
   //  Left arrow key - decrease angle by 5 degrees
   else if (key == GLUT_KEY_LEFT)
      th -= 5;
   //  Up arrow key - increase elevation by 5 degrees
   else if (key == GLUT_KEY_UP)
      ph += 5;
   //  Down arrow key - decrease elevation by 5 degrees
   else if (key == GLUT_KEY_DOWN)
      ph -= 5;
   //  Keep angles to +/-360 degrees
   th %= 360;
   ph %= 360;
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when a key is pressed
 */
void key(unsigned char ch,int x,int y)
{
   //  Exit on ESC
   if (ch == 27)
      exit(0);
   //  Reset view angle
   else if (ch == '0')
      th = ph = 0;
   else if (ch == '9')
      g_x = g_y = g_z = 0;
   else if (ch == 'r')
      g_x += 0.1;
   else if (ch == 'f')
      g_x -= 0.1;
   else if (ch == 'd')
      g_z -= 0.1;
   else if (ch == 'g')
      g_z += 0.1;
   else if (ch == 'o')
      g_y += 0.1;
   else if (ch == 'l')
      g_y -= 0.1;
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when the window is resized
 */
void reshape(int width,int height)
{
   //  Set the viewport to the entire window
   glViewport(0,0, RES*width,RES*height);
   //  Tell OpenGL we want to manipulate the projection matrix
   glMatrixMode(GL_PROJECTION);
   //  Undo previous transformations
   glLoadIdentity();
   //  Orthogonal projection
   const double dim=10;
   double asp = (height>0) ? (double)width/height : 1;
   glOrtho(-asp*dim,+asp*dim, -dim,+dim, -2*dim,+2*dim);
   //  Switch to manipulating the model matrix
   glMatrixMode(GL_MODELVIEW);
   //  Undo previous transformations
   glLoadIdentity();
}

/*
 *  GLUT calls this routine when there is nothing else to do
 */
void idle()
{
   // t = glutGet(GLUT_ELAPSED_TIME);
   glutPostRedisplay();
}

/*
 *  Start up GLUT and tell it what to do
 */
int main(int argc,char* argv[])
{
   //  Initialize GLUT and process user parameters
   init_boids();
   glutInit(&argc,argv);
   //  Request double buffered, true color window with Z buffering at 600x600
   glutInitWindowSize(600,600);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   //  Create the window
   glutCreateWindow("Objects");
#ifdef USEGLEW
   //  Initialize GLEW
   if (glewInit()!=GLEW_OK) Fatal("Error initializing GLEW\n");
#endif
   //  Tell GLUT to call "idle" when there is nothing else to do
   glutIdleFunc(idle);
   //  Tell GLUT to call "display" when the scene should be drawn
   glutDisplayFunc(display);
   //  Tell GLUT to call "reshape" when the window is resized
   glutReshapeFunc(reshape);
   //  Tell GLUT to call "special" when an arrow key is pressed
   glutSpecialFunc(special);
   //  Tell GLUT to call "key" when a key is pressed
   glutKeyboardFunc(key);
   //  Pass control to GLUT so it can interact with the user
   glutMainLoop();
   return 0;
}


