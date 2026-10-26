#pragma once

#include <glm/vec2.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

class FrameRecorder {
public:
    explicit FrameRecorder(std::filesystem::path directory);
    ~FrameRecorder();

    FrameRecorder(const FrameRecorder&) = delete;
    FrameRecorder& operator=(const FrameRecorder&) = delete;

    void captureFrame(const glm::ivec2& framebufferSize);
    std::vector<float>& audioBuffer();
    void finish();
    int frameCount() const;

private:
    void openVideoStream(const glm::ivec2& framebufferSize);
    void writeAudioFile() const;

    std::filesystem::path directory_;
    std::ofstream video_;
    glm::ivec2 frameSize_{0};
    std::vector<std::uint8_t> pixels_;
    std::vector<float> audio_;
    int frameCount_ = 0;
    bool finished_ = false;
};
