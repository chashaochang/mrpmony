#include "FrameBuffer.h"

FrameBuffer::FrameBuffer(int32_t width, int32_t height)
    : width_(width), height_(height), rgba_(static_cast<size_t>(width) * static_cast<size_t>(height) * 4U, 0)
{
}

void FrameBuffer::FillTestPattern(int64_t frameId)
{
    for (int32_t y = 0; y < height_; ++y) {
        for (int32_t x = 0; x < width_; ++x) {
            const size_t index = static_cast<size_t>(y * width_ + x) * 4U;
            rgba_[index] = static_cast<uint8_t>((x + frameId) % 255);
            rgba_[index + 1] = static_cast<uint8_t>((y + frameId) % 255);
            rgba_[index + 2] = static_cast<uint8_t>((x + y) % 255);
            rgba_[index + 3] = 255;
        }
    }
}

const std::vector<uint8_t> &FrameBuffer::Data() const
{
    return rgba_;
}
