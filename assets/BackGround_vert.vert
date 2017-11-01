#version 300 es

precision highp float;


#define VERTEX_LOCATION 1
#define NORMAL_LOCATION 2
#define COLOR_INPUT_LOCATION 3
#define UV_LOCATION 4
#define MATRIX1_LOCATION 5 // AND 6 7 8
#define MATRIX2_LOCATION 9 // AND 10 11 12


layout(location=VERTEX_LOCATION) in vec4 vert;
out vec4 gradient;

void main()
{
   if(vert.y > 0.0)
   {
      gradient = vec4(0.0, 0.0, 0.0, 1.0);
   }
   else
   {
      gradient = vec4(25.0 / 255.0, 81.0 / 255.0, 98.0 / 255.0, 1.0);
   }
   
   gl_Position = vert;
}