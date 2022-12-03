/*
 *  HW 6
 *  Author: Connor Davel
 *  Date: 10/5/2022
 *  Sources: Lots of code is used from examples 1-8 and example 13 with calls to the opengl lighting functions
 */
#include "CSCIx229.h"

void reshape(int, int);

int th=0;     // view angles used in class
int ph=0;
double l_x = 0;
double l_y = 1;
double l_z = 0;
double g_x = -10;
double g_z = 0;
int window_width = 600;
int window_height = 600;
int shader; 
int nw,ng,n;         //  Work group size and count
unsigned int posbuf; //  Position buffer
unsigned int velbuf; //  Velocity buffer
unsigned int colbuf; //  Color buffer
unsigned int wood_texture;
unsigned int leaves_texture;
unsigned int ground_texture;
int sky; 
#define N 10000
#define O 10

int mode = 3;
int emission  =   0;
float shiny   =   1;

typedef struct
{
   double p[3];
   double v[3];
   double a[3];
   double mass;  
   double s; 
}Boid; 

typedef struct 
{
   double x;
   double y; 
   double z; 
   double ux;
   double uy; 
   double uz; 
   double r; 
   int type; // 1 = sphere, 2 = cyliner, 3 = sky plane
}CollisionObject;

void sphere_collision_function(double *x, double *y, double *z, double *r); 

CollisionObject collision_objects[O]; 
Boid boids[N]; 

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

static void Vertex(double th,double ph, double r, double g, double b)
{
   double x = Sin(th)*Cos(ph);
   double z = Cos(th)*Cos(ph);
   double y =         Sin(ph);

   glNormal3d(x,y,z);
   glTexCoord2f((ph+90)/180,fmod(th/90, 1)); 
   glVertex3d(x, y ,z); //polar -> cartesian coordinates
   
}

void sphere(double x, double y, double z, double x_scale, double y_scale, double z_scale, int rot) {
   //  Save transformation
   double r = 0;
   double g = 1; 
   double b = 0;
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,leaves_texture);

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
   glDisable(GL_TEXTURE_2D);
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
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D, wood_texture);
   glBegin(GL_QUAD_STRIP);
   for (int th=0;th<=360;th+=45)
   {
      glNormal3f(0, Cos(th), Sin(th));
      glTexCoord2f(th/360.,0); glVertex3d(length,r2*Cos(th),r2*Sin(th));
      glTexCoord2f(th/360.,1); glVertex3d(0,r1*Cos(th),r1*Sin(th));
   }
   glEnd();
   glBegin(GL_TRIANGLE_FAN);
   glNormal3d(1,0,0);
   glVertex3d(length, 0.0, 0.0);
   for (int th=0;th<=360;th+=45)
      glVertex3d(length,r2*Cos(th),r2*Sin(th));
   glEnd();
   glDisable(GL_TEXTURE_2D);
   // cylinders do not need another end cap, since 
   // all should originate from inside the trunk of the tree

   glPopMatrix();
}

void cone(double x, double y, double z, double h, double r, int rot, int overall_rot) {
   //  Save transformation
   glPushMatrix();
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,leaves_texture);
   //  Offset and scale
   glTranslated(x,y,z);
   glRotatef(overall_rot,0,1,0);
   glScalef(2*r, h, 2*r);
   glBegin(GL_TRIANGLE_STRIP);
    
   for (int th=0;th<=360;th+=rot)
   {  
      glNormal3d(Cos(th),0.2,Sin(th));
      glTexCoord2f(0.5,0.5); glVertex3d(0,1,0);
      glTexCoord2f(0.5+0.5*Cos(th), 0.5+0.5*Sin(th)); glVertex3d(0.5*Cos(th),0,0.5*Sin(th));
   }
   glEnd();
   glBegin(GL_TRIANGLE_FAN);
   glNormal3d(0,-1,0);
   glTexCoord2f(0.5,0.5);
   glVertex3d(0,0,0);
   for (int th=0;th<=360;th+=rot)
   {
      glTexCoord2f(0.5+0.5*Cos(th), 0.5+0.5*Sin(th));
      glVertex3d(0.5*Cos(th),0,0.5*Sin(th));
   }
   glEnd();
   glDisable(GL_TEXTURE_2D);
   //  Undo transformations
   glPopMatrix();
}

void trunk_seg(double wid, double wid2, double start_h, double end_h, double tot_h) {
   // float white[] = {1,1,1,1};
   // float Emission[]  = {0.0,0.0,0.01*emission,1.0};
   // glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
   // glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   // glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);

   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D, wood_texture);
   glBegin(GL_QUAD_STRIP);
   for (int th=0;th<=360;th+=30)
   {
      glNormal3f(Cos(th), wid2-wid,Sin(th));
      glTexCoord2f(th/360.,(start_h/tot_h)); glVertex3d(wid*Cos(th),start_h,wid*Sin(th));
      glTexCoord2f((th)/360.,(end_h/tot_h)); glVertex3d(wid2*Cos(th),end_h,wid2*Sin(th));
   }
   glEnd();
   glDisable(GL_TEXTURE_2D);
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

   double total_h = 2.5;
   glPushMatrix();
   glTranslated(x,y,z);
   double wid = 0.5;
   double wid2 = 0.3;
   trunk_seg(wid, wid2, 0, 0.2, total_h); 
   wid = wid2;
   wid2 = 0.2;
   trunk_seg(wid, wid2, 0.2, 0.6, total_h); 
   wid = wid2;
   wid2 = 0.15;
   trunk_seg(wid, wid2, 0.6, 2.5, total_h); 
   sphere(0,2.5,0, 1,.4,1, 45);

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

void tree1(double x, double y, double z, int branch_rot, int overall_rot, double scale){
   glPushMatrix();
   glTranslated(x,y,z);
   glTranslated(0,scale/3,0);
   glRotatef(overall_rot, 0,1,0);
   glScaled(scale,scale,scale);
   for (int i = 0; i<=5;i++){
      int rot_scale = branch_rot * i;
      double y_offset = scale/20 + i/(5*scale);
      glRotatef(rot_scale,0,1,0);
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
   trunk_seg(wid, wid2, 0, 0.2, 2.5); 

   wid = wid2;
   wid2 = 0.1;
   trunk_seg(wid, wid2, 0.2, 0.6, 2.5); 

   wid = wid2;
   wid2 = 0.02;
   trunk_seg(wid, wid2, 0.6, 2.5, 2.5);

   glPopMatrix();
}

void tree2(double x, double y, double z, int overall_rot, double r, double scale){
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

   double total_h = 2.5;
   glPushMatrix();
   glTranslated(x,y,z);
   double wid = 0.35;
   double wid2 = 0.15;
   trunk_seg(wid, wid2, 0, 0.2, total_h); 
   wid = wid2;
   wid2 = 0.1;
   trunk_seg(wid, wid2, 0.2, 0.6, total_h); 
   wid = wid2;
   wid2 = 0.08;
   trunk_seg(wid, wid2, 0.6, 2.5, total_h); 

   glPopMatrix();
}

/*
 *  Draw a ball
 *     at (x,y,z)
 *     radius (r)
 */
static void light_ball(double x,double y,double z,double r)
{
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glScaled(r,r,r);
   //  White ball with yellow specular
   glColor3f(0,1,0);
   //  Bands of latitude
   int inc = 10;
   for (int ph=-90;ph<90;ph+=inc)
   {
      glBegin(GL_QUAD_STRIP);
      for (int th=0;th<=360;th+=2*inc)
      {
         Vertex(th,ph,1,1,1);
         Vertex(th,ph+inc,1,1,1);
      }
      glEnd();
   }
   //  Undo transofrmations
   glPopMatrix();
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
      double d = 5;
      Boid temp = {{d*x,5+d*abs(y),d*z}, {0,0,0}, {0,0,0}, 1, fmod((i/100.), 0.1)};
      boids[i] = temp; 
   }
   CollisionObject tree = {0,0,0, 0,0,0, 5, 0}; 
   collision_objects[0] = tree; 
}
void update() {
   double sight_radius = 3; 
   double avoid_radius = 0.7;
   double dt = 0.05;
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

      double containment = 0; 
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
      if (total_neighbors != 0) {
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
            Fx -= 0.6 * (avg_xa - cvx*dot_product); 
            Fy -= 0.6 * (avg_ya - cvy*dot_product); 
            Fz -= 0.6 * (avg_za - cvz*dot_product); 
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
            Fx += 0.1 * (avg_vx - cvx*dot_product); 
            Fy += 0.1 * (avg_vy - cvy*dot_product); 
            Fz += 0.1 * (avg_vz - cvz*dot_product); 
         }
      }
      
      // add basic object avoidance...
      for (int o=0; o<O; o++) {
         CollisionObject obj = collision_objects[o];
         if (obj.type == 0) {
            if (norm(obj.x-cx, obj.y-cy, obj.z-cz) < obj.r + 0.3){
            Fx -= 10*(obj.x-cx);
            Fy -= 10*(obj.y-cy);
            Fz -= 10*(obj.z-cz); 
            }
         }
      }
      double bounding_box[6] = {-10, 10, 5, 15, -10, 10};
      if (cx < bounding_box[0]) {
         Fx += 1;
      }
      else if (cx > bounding_box[1]) {
         Fx -= 1;
      }
      if (cy < bounding_box[2]) {
         Fy += 1;
      }
      else if (cy > bounding_box[3]) {
         Fy -= 1;
      }
      if (cz < bounding_box[4]) {
         Fz += 1;
      }
      else if (cz > bounding_box[5]) {
         Fz -= 1;
      }
      // apply force
      current.a[0] = Fx;
      current.a[1] = Fy;
      current.a[2] = Fz;

      cvx = current.v[0] + Fx/m*dt;
      cvy = current.v[1] + Fy/m*dt;
      cvz = current.v[2] + Fz/m*dt;
      double vel_norm = norm(cvx, cvy, cvz);
      cvx = cvx/vel_norm;
      cvy = cvy/vel_norm;
      cvz = cvz/vel_norm;
      current.v[0] = cvx;
      current.v[1] = cvy;
      current.v[2] = cvz;

      current.p[0] = cx + dt*cvx;
      current.p[1] = cy + dt*cvy;
      current.p[2] = cz + dt*cvz;
      current.s = fmod((current.s+0.002), 0.1);
      boids[k] = current; 
   }
}

void render_boids() {
   // glBegin(GL_POINTS);
   for (int i=0; i<N; i++){
      Boid temp = boids[i];
      double x = temp.p[0];
      double y = temp.p[1];
      double z = temp.p[2];
      double size = fabs(temp.s-0.05);
      light_ball(x,y,z, size); 
      
      // fence_post(x,y,z, 0.4); 
      
   }
   // glEnd();
}

/* 
 *  Draw sky box
 */
static void Sky(double x, double y, double z, double D)
{
   //  Textured white box dimension (-D,+D)
   glPushMatrix();
   glTranslated(x,y,z); 
   glScaled(D,D,D);
   glEnable(GL_TEXTURE_2D);
   glColor3f(1,1,1);

   //  Sides
   glBindTexture(GL_TEXTURE_2D,sky);
   glBegin(GL_QUADS);
   glTexCoord2f(0,0.34); glVertex3f(-1,-1,-1);
   glTexCoord2f(0.25,0.34); glVertex3f(+1,-1,-1);
   glTexCoord2f(0.25,0.66); glVertex3f(+1,+1,-1);
   glTexCoord2f(0,0.66); glVertex3f(-1,+1,-1);

   glTexCoord2f(0.25,0.34); glVertex3f(+1,-1,-1);
   glTexCoord2f(0.50,0.34); glVertex3f(+1,-1,+1);
   glTexCoord2f(0.50,0.66); glVertex3f(+1,+1,+1);
   glTexCoord2f(0.25,0.66); glVertex3f(+1,+1,-1);

   glTexCoord2f(0.50,0.34); glVertex3f(+1,-1,+1);
   glTexCoord2f(0.75,0.34); glVertex3f(-1,-1,+1);
   glTexCoord2f(0.75,0.66); glVertex3f(-1,+1,+1);
   glTexCoord2f(0.50,0.66); glVertex3f(+1,+1,+1);

   glTexCoord2f(0.75,0.34); glVertex3f(-1,-1,+1);
   glTexCoord2f(1.00,0.34); glVertex3f(-1,-1,-1);
   glTexCoord2f(1.00,0.66); glVertex3f(-1,+1,-1);
   glTexCoord2f(0.75,0.66); glVertex3f(-1,+1,+1);
   glEnd();

   //  Top and bottom
   glBindTexture(GL_TEXTURE_2D,sky);
   glBegin(GL_QUADS);
   glTexCoord2f(0.25,0.66); glVertex3f(+1,+1,-1);
   glTexCoord2f(0.5,0.66); glVertex3f(+1,+1,+1);
   glTexCoord2f(0.5,1.0); glVertex3f(-1,+1,+1);
   glTexCoord2f(0.25,1.0); glVertex3f(-1,+1,-1);

   glEnd();

   //  Undo
   glDisable(GL_TEXTURE_2D);
   glPopMatrix();
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
   if (mode == 3) {
      gluLookAt(g_x,1,g_z, g_x+Cos(th),0.5+Sin(ph),g_z+Sin(th), 0,1,0);
   }
   else {
      glRotatef(ph,1,0,0);
      glRotatef(th,0,1,0);
   }
   

   int ambient   =  0;
   int diffuse   =  10;
   int specular  =   0; 
   int local     =   0;
   float Position[4];

   //  Translate intensity to color vectors
   float Ambient[]   = {0.005*ambient ,0.005*ambient ,0.01*ambient ,1.0};
   float Diffuse[]   = {0.005*diffuse ,0.005*diffuse ,0.01*diffuse ,1.0};
   float Specular[]  = {0.005*specular,0.005*specular,0.01*specular,1.0};
   //  Light position
   if (mode == 3)
   {
      // Position[0] = g_x + Cos(th);
      // Position[1] = 0.4 + Sin(ph);
      // Position[2] = g_z + Sin(th);
      // Position[3] = 1.0;
      Position[0] = 0;
      Position[1] = 20;
      Position[2] = 0;
      Position[3] = 0;
   }
   else
   {
      Position[0] = l_x;
      Position[1] = l_y;
      Position[2] = l_z;
      Position[3] = 1;
      //  Draw light position as ball (still no lighting here)
      
   }
   
   //  OpenGL should normalize normal vectors
   glEnable(GL_NORMALIZE);

   glEnable(GL_LIGHTING);

   glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,local);
   glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);

   glEnable(GL_LIGHT0);
   glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
   glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Specular);
   glLightfv(GL_LIGHT0,GL_POSITION,Position);

   if (mode == 0 || mode == 3) {
      glEnable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
      glColor3f(1,1,1);
      glBindTexture(GL_TEXTURE_2D, ground_texture);
      glPushMatrix();
      glScaled(40,0,40);
      glBegin(GL_QUADS);
      glNormal3f(0,1,0);
      double ground_inc = 0.01;
      double tex_mag;
      if (mode == 0)
         tex_mag = 6;
      else
         tex_mag = 30;
      for (double i=-1;i<=1;i+=ground_inc){
         for (double j=-1;j<=1;j+=ground_inc){
            glTexCoord2f(-1+fmod(tex_mag*(i),2),-1+fmod(tex_mag*(j),2));glVertex3f(i,0,j);
            glTexCoord2f(-1+fmod(tex_mag*(i+ground_inc),2),-1+fmod(tex_mag*(j),2));glVertex3f(i+ground_inc,0,j);
            glTexCoord2f(-1+fmod(tex_mag*(i+ground_inc),2),-1+fmod(tex_mag*(j+ground_inc),2));glVertex3f(i+ground_inc,0,j+ground_inc);
            glTexCoord2f(-1+fmod(tex_mag*(i),2),-1+fmod(tex_mag*(j+ground_inc),2));glVertex3f(i,0,j+ground_inc);
         }
      }
      glEnd();
      glDisable(GL_TEXTURE_2D);
      glPopMatrix();

      // rings of trees
      for (int r = 20; r<40; r=r+1) {
         for (int th=0;th<=360;th+=r)
         {
            tree1_simple(r*Cos(th+5*r), 0, r*Sin(th+5*r)+th/360., 41 + th, 0, 0.5+(r%5)/5.);
         }
      }
      
      // tree1_simple(9,0,0,30, 0, 1.5);
      // tree1_simple(8,0,1,40, 0, 1);
      // tree1_simple(7,0,3,50, 0, 0.8);
      // tree1_simple(-1,0,1.5,60, 0, 0.6);
      // tree1_simple(-3,0,0.5,70, 0, 1);
      // tree1_simple(0,0,7,80, 0, 1.1);
      // tree1_simple(0,0,-5,15, 0, 1);
      // tree2(-1.9,0,3, 0, 0.9, 1);
      // tree2(-3,0,4, 0, 1.3, 1);
      // tree2(-3.1,0,-3, 0, 1.3, 1);
      // tree2(-1.4,0,-4, 0, 0.9, 1);
      glPushMatrix();
      glRotatef(180, 0,1,0);
      tree1(0,0,0,55, 0, 3);
      glPopMatrix(); 
      tree1(4,0,0,41, 0, 4);
      
   }
   else if (mode == 1) {
      tree1_simple(0,0,0,30, 0, 1.5);
   }
   else if (mode == 2) {
      tree2(0,0,0, 0, 0.9, 1);
   }
   
   glDisable(GL_LIGHTING);
   glDisable(GL_LIGHT0); 
   glDisable(GL_COLOR_MATERIAL);
   update(); 
   render_boids();
   Sky(g_x,0, g_z, 50);
   glColor3f(1,1,1);
   // light_ball(Position[0],Position[1],Position[2] , 0.1);
   
   glWindowPos2i(5,5);
   if (mode == 0) {
      Print("overall scene view (use wasd + e + q to move light)");
   }
   else if (mode == 1) {
      Print("first tree model (no light)");
   }
   else if (mode == 2) {
      Print("second tree model (no light)");
   }
   else if (mode == 3) {
      Print("ground inspection mode");
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
   //  Keep angles to +/-360 degrees
   if (mode == 0 || mode == 1 || mode == 2)
   {
      th %= 360;
      ph %= 360;
   }
   else {
      th %= 360;
      if (ph > 90) {
         ph = 90;
      }
      else if (ph < -90) {
         ph = -90;
      }
   }
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
      l_x = l_y = l_z = 0;
   else if (ch == 'm')
   {
      // mode = (mode + 1)%4;
      th = ph = 0;
      g_z = 0;
      g_x = -10;
      reshape(window_width, window_height);
   }
   if (mode == 3) {
      if (ch == 'w')
      {
         g_x += 0.1*Cos(th);
         g_z += 0.1*Sin(th);
      }
      else if (ch == 's')
      {
         g_x -= 0.1*Cos(th);
         g_z -= 0.1*Sin(th);
      }
      else if (ch == 'a')
      {
         g_x += 0.1*Sin(th);
         g_z -= 0.1*Cos(th);
      }
      else if (ch == 'd')
      {
         g_x -= 0.1*Sin(th);
         g_z += 0.1*Cos(th);
      } 
   }
   else {
      if (ch == 'w')
         l_x += 0.1;
      else if (ch == 's')
         l_x -= 0.1;
      else if (ch == 'a')
         l_z += 0.1;
      else if (ch == 'd')
         l_z -= 0.1;
      else if (ch == 'q')
         l_y += 0.1;
      else if (ch == 'e')
         l_y -= 0.1;
   }
   
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
   if (mode == 3) {
      gluPerspective(60, asp, 0.001, 100.0);
   }
   else {
      glOrtho(-asp*dim,+asp*dim, -dim,+dim, -2*dim,+2*dim);
   }
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
   init_boids();
   //  Initialize GLUT and process user parameters
   glutInit(&argc,argv);
   //  Request double buffered, true color window with Z buffering at 600x600
   glutInitWindowSize(600,600);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   //  Create the window
   glutCreateWindow("HW6: light and textures, Connor Davel");
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
   wood_texture = LoadTexBMP("bark.bmp");
   leaves_texture = LoadTexBMP("leaves.bmp");
   ground_texture = LoadTexBMP("ground.bmp");
   sky = LoadTexBMP("sky.bmp");
   glutMainLoop();
   return 0;
}


