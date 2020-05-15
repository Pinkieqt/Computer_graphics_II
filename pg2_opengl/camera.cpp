#include "pch.h"
#include "camera.h"

Camera::Camera(const int width, const int height, const float fov_y,
	const Vector3 view_from, const Vector3 view_at)
{
	width_ = width;
	height_ = height;
	fov_y_ = fov_y;

	view_from_ = view_from;
	view_at_ = view_at;
	Update();
}

Vector3 Camera::view_from() const
{
	return view_from_;
}

Matrix3x3 Camera::M_c_w() const
{
	return M_c_w_;
}

float Camera::focal_length() const
{
	return f_y_;
}

void Camera::set_fov_y(const float fov_y)
{
	assert(fov_y > 0.0);

	fov_y_ = fov_y;
}

Matrix4x4 Camera::BuildMLPMatrix(Vector3 lightPosition) {
	f_y_ = height_ / (2.0f * tanf(fov_y_ * 0.5f));

	Vector3 z_c = lightPosition - view_at_;
	z_c.Normalize();
	Vector3 x_c = up_.CrossProduct(z_c);
	x_c.Normalize();
	Vector3 y_c = z_c.CrossProduct(x_c);
	y_c.Normalize();
	M_c_w_ = Matrix3x3(x_c, y_c, z_c);

	//pdf page 20
	Matrix4x4 VM = Matrix4x4(x_c, y_c, z_c, lightPosition);
	VM.EuclideanInverse();

	//pdf page 20 a dál
	float n = 1.0f;
	float f = 1000.0f;
	float aspectRatio = width_ / (float)height_;

	//orig: 2* n * tanf(...)
	float nearPlaneHeight = 1 * n * tanf(fov_y_ / 2);
	float nearPlaneWidth = aspectRatio * nearPlaneHeight;

	float a = (n + f) / (n - f);
	float b = (2 * n * f) / (n - f);

	Matrix4x4 MP;
	MP.set(0, 0, n / nearPlaneWidth);
	MP.set(1, 1, n / nearPlaneHeight);
	MP.set(2, 2, a);
	MP.set(2, 3, b);
	MP.set(3, 2, -1);

	MLP = MP * VM;
	return MLP;
}

void Camera::Update()
{
	f_y_ = height_ / (2.0f * tanf(fov_y_ * 0.5f));

	Vector3 z_c = view_from_ - view_at_;
	z_c.Normalize();
	Vector3 x_c = up_.CrossProduct(z_c);
	x_c.Normalize();
	Vector3 y_c = z_c.CrossProduct(x_c);
	y_c.Normalize();
	M_c_w_ = Matrix3x3(x_c, y_c, z_c);

	//pdf page 20
	viewMatrix = Matrix4x4(x_c, y_c, z_c, view_from_);
	viewMatrix.EuclideanInverse();

	//pdf page 20 a dál
	float n = 1.0f;
	float f = 1000.0f;
	float aspectRatio = width_ / (float)height_;

	//orig: 2* n * tanf(...)
	float nearPlaneHeight = 1 * n * tanf(fov_y_ / 2);
	//h = w/aspect -> w = aspect * h
	float nearPlaneWidth = aspectRatio * nearPlaneHeight;

	float a = (n + f) / (n - f);
	float b = (2 * n * f) / (n - f);

	projectionMatrix.set(0, 0, n / nearPlaneWidth);
	projectionMatrix.set(1, 1, n / nearPlaneHeight);
	projectionMatrix.set(2, 2, a);
	projectionMatrix.set(2, 3, b);
	projectionMatrix.set(3, 2, -1);
}

void Camera::MoveForward(const float dt)
{
	Vector3 ds = view_at_ - view_from_;
	ds.Normalize();
	ds *= dt;

	view_from_ += ds;
	view_at_ += ds;
}
