#include <math.h>
#include <iostream>

#define PI 3.141592653589793

using namespace std;

float length(float* v);

float length2D(float* v);

void normalise(float* vn);

void normalise2D(float* vn);

float dot2D(float* a, float* b);
float dot(float* a, float* b);

float* cross(float* a, float* b);





// t={p1x, p1y,  p2x, p2y, p3x, p3y }
float triangleArea2D(float* triangle);

// tests: triangle area X point--sub-triangles areas
bool triangleCollidePoint2D(float* triangle, float* point);

bool collideByDotProduct(float* triangle, float* point);


