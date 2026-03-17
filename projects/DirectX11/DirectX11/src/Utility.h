#pragma once

#include <windows.h>
#include <stdexcept>

// —áŠOˆ—
inline void ThrowIfFailed(HRESULT hr, const char* msg)
{
    if (FAILED(hr)) {
        throw std::runtime_error(msg);
    }
}