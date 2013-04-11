
#ifndef MATH_UTILS_IMAO
#define MATH_UTILS_IMAO

#define PI_PER_DEG (3.1415927f/180.0f)
#define DEG_PER_PI (180.0f/3.1415927f)

template <typename T>
void pprintMat(T& mat, int size, std::string name)
{
	printf("\n%s\n", name.c_str());
	for(int i=0; i< size; i++)
	{
		for (int j=0; j< size; j++)
			printf("%8.5f ", mat[j][i]);

		printf("\n");
	}
};

class Triplet
{
public:
	Triplet(const Triplet& t):x(t.x),y(t.y),z(t.z){};
	Triplet(float a=.0f, float b=.0f, float c=.0f):x(a),y(b),z(c){};
	float x, y, z;
};

template <typename T>
Triplet rot2eularZYX(T& m)
{
	float rx = atan2(m[1][2], m[2][2])*DEG_PER_PI;
	float c2 = sqrt(m[0][0]*m[0][0] + m[0][1]*m[0][1]);
	float ry = atan2(-m[0][2], c2)*DEG_PER_PI;
	float rz = atan2(m[0][1], m[0][0])*DEG_PER_PI;

	return Triplet(rz, ry, rx);
}
#endif
