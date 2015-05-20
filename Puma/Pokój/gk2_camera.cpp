#include "gk2_camera.h"
#include <ctime>

using namespace gk2;

Camera::Camera(float minDistance, float maxDistance, float distance)
	: m_angleX(0.0f), m_angleY(0.0f), m_distance(distance), m_position(0, 0, 10, 1)
{
	
	SetRange(minDistance, maxDistance);
}

void Camera::ClampDistance()
{
	if (m_distance < m_minDistance)
		m_distance = m_minDistance;
	if (m_distance > m_maxDistance)
		m_distance = m_maxDistance;
}

void Camera::SetRange(float minDistance, float maxDistance)
{
	if (minDistance < 0)
		minDistance = 0;
	if (maxDistance < minDistance)
		maxDistance = minDistance;
	m_minDistance = minDistance;
	m_maxDistance = maxDistance;
	ClampDistance();
}

void Camera::Zoom(float d)
{
	m_distance += d;
	ClampDistance();
}

void Camera::Rotate(float dx, float dy)
{
	m_angleX = XMScalarModAngle(m_angleX + dx);
	m_angleY = XMScalarModAngle(m_angleY + dy);
}

XMMATRIX Camera::GetViewMatrix()
{
	XMMATRIX viewMtx;
	GetViewMatrix(viewMtx);
	return viewMtx;
}

void Camera::GetViewMatrix(XMMATRIX& viewMtx)
{
	XMVECTOR DIR, UP, CROSS;
	CalculateVectors(UP, DIR, CROSS);

	XMVECTOR POS = XMLoadFloat4(&m_position);

	viewMtx = XMMatrixLookToLH(POS, DIR, UP);
	
}

XMFLOAT4 Camera::GetPosition()
{
	return m_position;
}

void Camera::MoveCamera(float dx, float dy, float dz)
{
	XMVECTOR DIR, UP, CROSS;
	CalculateVectors(UP, DIR, CROSS);
	XMVECTOR POS = XMLoadFloat4(&m_position);

	auto dxx = dx > 0 ? dx : -dx; 
	auto dyy = dy > 0 ? dy : -dy; 
	auto dzz = dz > 0 ? dz : -dz; 

	XMVECTOR X_MOVE = XMVector3ClampLength(DIR, dxx, dxx);
	XMVECTOR Y_MOVE = XMVector3ClampLength(CROSS, dyy, dyy);
	XMVECTOR Z_MOVE = XMVector3ClampLength(UP, dzz, dzz);

	POS += (dx > 0 ? X_MOVE : -X_MOVE);
	POS += (dy > 0 ? Y_MOVE : -Y_MOVE);
	POS += (dz > 0 ? Z_MOVE : -Z_MOVE);
	XMStoreFloat4(&m_position, POS);

}

void Camera::CalculateVectors(XMVECTOR& UP, XMVECTOR& DIR, XMVECTOR& CROSS){
	XMFLOAT4 dir = XMFLOAT4(0, 0, -1, 1);
	XMFLOAT4 up = XMFLOAT4(0, 1, 0, 1);

	DIR = XMLoadFloat4(&dir);
	UP = XMLoadFloat4(&up);
	
	XMMATRIX MX = XMMatrixRotationX(-m_angleX);
	XMMATRIX MY = XMMatrixRotationY(m_angleY);
	XMMATRIX SUM = MX * MY;

	DIR = XMVector4Transform(DIR, SUM);
	UP = XMVector4Transform(UP, SUM);
	CROSS = XMVector3Cross(DIR, UP);
}


