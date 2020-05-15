#ifndef CAMERA_H_
#define CAMERA_H_

#include "vector3.h"
#include "matrix3x3.h"
#include "matrix4x4.h"

/*! \class Camera
\brief A simple pin-hole camera.

\author Tomáš Fabián
\version 1.0
\date 2018-2019
*/
class Camera
{
public:
	Camera() { }

	Camera(const int width, const int height, const float fov_y,
		const Vector3 view_from, const Vector3 view_at);

	Vector3 view_from() const;
	Matrix3x3 M_c_w() const;
	float focal_length() const;

	void set_fov_y(const float fov_y);

	Matrix4x4 BuildMLPMatrix(Vector3 lightPosition);

	void Update();

	void MoveForward(const float dt);


	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;
	Matrix4x4 MLP;

	int width_{ 640 }; // image width (px)
	int height_{ 480 };  // image height (px)
	Vector3 view_from_; // ray origin or eye or O
	Vector3 view_at_; // target T

private:
	float fov_y_{ 0.785f }; // vertical field of view (rad)

	Vector3 up_{ Vector3(0.0f, 0.0f, 1.0f) }; // up vector


	float f_y_{ 1.0f }; // focal lenght (px)

	Matrix3x3 M_c_w_; // transformation matrix from CS -> WS	
};

#endif
