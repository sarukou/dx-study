#include "Camera.h"
#include <windows.h>

using namespace DirectX;


float Camera::ClampPitch(float pitch)
{
    const float limit = XMConvertToRadians(89.0f);

    if (pitch > limit) {
        pitch = limit;
    }
    if (pitch < -limit) {
        pitch = -limit;
    }

    return pitch;
}

XMVECTOR Camera::GetForward() const
{
    const float cosPitch = cosf(m_pitch);
    const float sinPitch = sinf(m_pitch);
    const float cosYaw = cosf(m_yaw);
    const float sinYaw = sinf(m_yaw);

    XMVECTOR forward = XMVectorSet(cosPitch * sinYaw, sinPitch, cosPitch * cosYaw, 0.0f);

    return XMVector3Normalize(forward);
}

void Camera::Update(int mouseDx, int mouseDy, float deltaTime)
{
    m_mouseLook = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    if (m_mouseLook) {
        m_yaw += mouseDx * m_mouseSensitivity;
        m_pitch -= mouseDy * m_mouseSensitivity;
        m_pitch = ClampPitch(m_pitch);
    }

    XMVECTOR forward = GetForward();
    XMVECTOR up = XMLoadFloat3(&m_up);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));

    XMVECTOR move = XMVectorZero();

    if (GetAsyncKeyState('W') & 0x8000) {
        move += forward;
    }
    if (GetAsyncKeyState('S') & 0x8000) {
        move -= forward;
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        move += right;
    }
    if (GetAsyncKeyState('A') & 0x8000) {
        move -= right;
    }
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        move += up;
    }
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
        move -= up;
    }

    if (!XMVector3Equal(move, XMVectorZero())) {
        move = XMVector3Normalize(move) * m_moveSpeed * deltaTime;

        XMVECTOR pos = XMLoadFloat3(&m_position);
        pos += move;
        XMStoreFloat3(&m_position, pos);
    }
}