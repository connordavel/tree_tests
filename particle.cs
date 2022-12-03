//
//  Particle compute shader
//
#version 440 compatibility
#extension GL_ARB_compute_variable_group_size : enable

//  Array positions
layout(binding=4) buffer posbuf {vec4 pos[];};
layout(binding=5) buffer velbuf {vec4 vel[];};
layout(binding=6) buffer sizebuf {vec4 col[];};
//  Work group size
layout(local_size_variable) in;

//  Sphere
uniform vec3 xyz;
uniform uint dim;
//  Time step
const float  dt = 0.06;
const float sight_radius = 0.7;
const float avoid_radius = 0.3; 

//  Compute shader
void main()
{
   //  Global Thread ID
   uint  gid = gl_GlobalInvocationID.x;
   float size = col[gid].w + 0.05;
   if (size >=1) {
      size = -size;
   }
   col[gid].w = size;

   //  Get position and velocity
   vec3 p0 = pos[gid].xyz;
   vec3 v0 = vel[gid].xyz;
   // float current_velocity_norm = length(v0);
   // if (current_velocity_norm != 0) {
   //       v0 = v0/current_velocity_norm;
   //    }
   vec3 F = vec3(0.0,0.0,0.0);
   vec3 total_attract = vec3(0.0,0.0,0.0);
   vec3 total_avoid = vec3(0.0,0.0,0.0);
   vec3 total_vel = vec3(0.0,0.0,0.0);
   vec3 dx = vec3(0.0,0.0,0.0);
   vec3 nvx = vec3(0.0,0.0,0.0);
   uint total_neighbors = 0;
   uint i;
   for (i=0; i<5000; i++) { // looks like this needs to be hardcoded? 
      // v0 = vec3(0.001,0,0); 
      if (i==gid){
         continue;
      }
      dx = pos[i].xyz - p0;
      nvx = vel[i].xyz;
      if (length(dx)>sight_radius) {
         continue;
      }
      if (length(dx) < avoid_radius) {
         total_avoid += dx;
      }
      total_attract += dx;
      total_vel += nvx;
      total_neighbors += 1; 
   }
   if (total_neighbors != 0) {
      if (length(total_attract) > 0) {
         total_attract = normalize(total_attract);
         F += 0.35*total_attract - v0*dot(total_attract, v0);
      }
      if (length(total_avoid) > 0) {
         total_avoid = normalize(total_avoid);
         F -= 0.6*(total_avoid - v0*dot(total_avoid, v0));
      }
      if (length(total_vel) > 0) {
         total_vel = normalize(total_vel);
         F += 0.3*(total_vel - v0*dot(total_vel, v0));
      }
   }
   // add boundnig box
   float bounding_box[6] = {-10, 10, 5, 12, -10, 10};
   if (p0.x < bounding_box[0]) {
         F += vec3(1.0,0.0,0.0);
      }
      else if (p0.x > bounding_box[1]) {
         F -= vec3(1.0,0.0,0.0);
      }
      if (p0.y < bounding_box[2]) {
         F += vec3(0.0,1.0,0.0);
      }
      else if (p0.y > bounding_box[3]) {
         F -= vec3(0.0,1.0,0.0);
      }
      if (p0.z < bounding_box[4]) {
         F += vec3(0.0,0.0,1.0);
      }
      else if (p0.z > bounding_box[5]) {
         F -= vec3(0.0,0.0,1.0);
      }

   //  Compute new position and velocity
   vec3 v = v0 + F*dt; 
   if (length(v) > 1) { // set a speed limit 
      v = 1* normalize(v); 
   }
   vec3 p = p0 + dt*v;

   //  Update position and velocity
   pos[gid].xyz = p;
   vel[gid].xyz = v;
}
