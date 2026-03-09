#ifndef MRP_RENDER_FRAME_BUFFER_H
#define MRP_RENDER_FRAME_BUFFER_H

#include <cstdint>
#include <vector>

class FrameBuffer {
public:
    FrameBuffer(int32_t width, int32_t height);

    void FillTestPattern(int64_t frameId);
    const std::vector<uint8_t> &Data() const;

private:
    int32_t width_;
    int32_t height_;
    std::vector<uint8_t> rgba_;
};

#endif
