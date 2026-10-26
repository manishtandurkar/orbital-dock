#include "core/FrameRecorder.h"

#include "audio/AudioEngine.h"

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void writeLittleEndian(std::ofstream& out, std::uint32_t value, int bytes)
{
    for (int i = 0; i < bytes; ++i) {
        out.put(static_cast<char>((value >> (8 * i)) & 0xFFu));
    }
}

std::int16_t toPcm16(float sample)
{
    const float clamped = std::clamp(sample, -1.0f, 1.0f);
    return static_cast<std::int16_t>(std::lround(clamped * 32767.0f));
}

}

FrameRecorder::FrameRecorder(std::filesystem::path directory)
    : directory_(std::move(directory))
{
    std::filesystem::create_directories(directory_);
}

FrameRecorder::~FrameRecorder()
{
    finish();
}

void FrameRecorder::openVideoStream(const glm::ivec2& framebufferSize)
{
    frameSize_ = framebufferSize;
    {
        std::ofstream sizeFile(directory_ / "video_size.txt");
        sizeFile << frameSize_.x << ' ' << frameSize_.y << '\n';
    }
    video_.open(directory_ / "video.fifo", std::ios::binary);
    if (!video_) {
        throw std::runtime_error("Cannot open video stream in " + directory_.string());
    }
    pixels_.resize(static_cast<size_t>(frameSize_.x) * static_cast<size_t>(frameSize_.y) * 3);
}

void FrameRecorder::captureFrame(const glm::ivec2& framebufferSize)
{
    if (finished_) {
        return;
    }
    if (frameCount_ == 0) {
        openVideoStream(framebufferSize);
    }
    if (framebufferSize != frameSize_) {
        throw std::runtime_error("Framebuffer size changed while recording");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, frameSize_.x, frameSize_.y, GL_RGB, GL_UNSIGNED_BYTE, pixels_.data());
    video_.write(reinterpret_cast<const char*>(pixels_.data()), static_cast<std::streamsize>(pixels_.size()));
    ++frameCount_;
}

std::vector<float>& FrameRecorder::audioBuffer()
{
    return audio_;
}

void FrameRecorder::finish()
{
    if (finished_) {
        return;
    }
    finished_ = true;
    video_.close();
    writeAudioFile();
    std::cout << "Recorded " << frameCount_ << " frames to " << directory_.string() << std::endl;
}

void FrameRecorder::writeAudioFile() const
{
    std::ofstream out(directory_ / "audio.wav", std::ios::binary);
    const std::uint32_t channels = AudioEngine::kChannels;
    const std::uint32_t sampleRate = AudioEngine::kSampleRate;
    const std::uint32_t bytesPerSample = 2;
    const std::uint32_t dataBytes = static_cast<std::uint32_t>(audio_.size()) * bytesPerSample;
    out.write("RIFF", 4);
    writeLittleEndian(out, 36 + dataBytes, 4);
    out.write("WAVEfmt ", 8);
    writeLittleEndian(out, 16, 4);
    writeLittleEndian(out, 1, 2);
    writeLittleEndian(out, channels, 2);
    writeLittleEndian(out, sampleRate, 4);
    writeLittleEndian(out, sampleRate * channels * bytesPerSample, 4);
    writeLittleEndian(out, channels * bytesPerSample, 2);
    writeLittleEndian(out, bytesPerSample * 8, 2);
    out.write("data", 4);
    writeLittleEndian(out, dataBytes, 4);
    for (const float sample : audio_) {
        writeLittleEndian(out, static_cast<std::uint16_t>(toPcm16(sample)), 2);
    }
}

int FrameRecorder::frameCount() const
{
    return frameCount_;
}
