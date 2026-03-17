#pragma once

#include <DirectXMath.h>

class Camera
{
public:
    Camera() = default;

    void Update(int mouseDx, int mouseDy, float deltaTime);

    DirectX::XMVECTOR GetForward() const;

    const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
    const DirectX::XMFLOAT3& GetUp() const { return m_up; }

    float GetFovY() const { return m_fovY; }
    float GetAspect() const { return m_aspect; }
    float GetNearZ() const { return m_nearZ; }
    float GetFarZ() const { return m_farZ; }

    void SetAspect(float aspect) { m_aspect = aspect; }

    bool GetMouseLook() const { return m_mouseLook; }

private:
    // 真上を向けないようにクランプ
    static float ClampPitch(float pitch);

private:
    DirectX::XMFLOAT3 m_position = { 0.0f, 1.0f, -5.0f };
    DirectX::XMFLOAT3 m_up = { 0.0f, 1.0f, 0.0f };

    float m_fovY = DirectX::XMConvertToRadians(60.0f);
    float m_aspect = 1280.0f / 720.0f;
    float m_nearZ = 0.1f;
    float m_farZ = 1000.0f;

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;

    float m_moveSpeed = 3.0f;

    // マウス操作
    bool m_mouseLook = false;
    float m_mouseSensitivity = 0.0025f;
};