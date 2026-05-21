#include "MrpAudioBridge.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <hilog/log.h>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <fcntl.h>
#include <multimedia/player_framework/avplayer.h>
#include <multimedia/player_framework/avplayer_base.h>
#include <multimedia/player_framework/native_averrors.h>
#include <multimedia/player_framework/native_avformat.h>
#include <ohaudio/native_audiorenderer.h>
#include <ohaudio/native_audiostream_base.h>
#include <ohaudio/native_audiostreambuilder.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include "header/types.h"
}

namespace {
constexpr int32_t kOutputSampleRate = 48000;
constexpr int32_t kOutputChannels = 2;
constexpr int32_t kCallbackFrames = 480; // 10 ms at 48 kHz, low enough for keypad-era sound effects.
constexpr size_t kStopRampFrames = 240;  // 5 ms fade-out to avoid clicks on stop/replacement.
constexpr double kPi = 3.14159265358979323846;

enum MrSoundType {
    MR_SOUND_MIDI = 0,
    MR_SOUND_WAV = 1,
    MR_SOUND_MP3 = 2,
    MR_SOUND_PCM = 3,
    MR_SOUND_M4A = 4,
    MR_SOUND_AMR = 5,
    MR_SOUND_AMR_WB = 6,
};

uint16_t ReadLe16(const uint8_t *data)
{
    return static_cast<uint16_t>(data[0] | (data[1] << 8U));
}

uint32_t ReadLe32(const uint8_t *data)
{
    return static_cast<uint32_t>(data[0] | (data[1] << 8U) | (data[2] << 16U) | (data[3] << 24U));
}

uint16_t ReadBe16(const uint8_t *data)
{
    return static_cast<uint16_t>((data[0] << 8U) | data[1]);
}

uint32_t ReadBe32(const uint8_t *data)
{
    return static_cast<uint32_t>((data[0] << 24U) | (data[1] << 16U) | (data[2] << 8U) | data[3]);
}

int16_t ClampSample(int32_t value)
{
    return static_cast<int16_t>(std::max<int32_t>(-32768, std::min<int32_t>(32767, value)));
}

bool ReadVarLen(const uint8_t *data, uint32_t len, uint32_t &offset, uint32_t &value)
{
    value = 0;
    for (int32_t i = 0; i < 4; ++i) {
        if (offset >= len) {
            return false;
        }
        const uint8_t byte = data[offset++];
        value = (value << 7U) | (byte & 0x7FU);
        if ((byte & 0x80U) == 0) {
            return true;
        }
    }
    return false;
}

struct MidiNoteEvent {
    uint32_t tick = 0;
    uint8_t note = 0;
    uint8_t velocity = 0;
    bool on = false;
};

struct MidiTempoEvent {
    uint32_t tick = 0;
    uint32_t microsecondsPerQuarter = 500000;
};

struct MidiNoteInterval {
    uint32_t startTick = 0;
    uint32_t endTick = 0;
    uint8_t note = 0;
    uint8_t velocity = 0;
};

class MidiTickConverter {
public:
    MidiTickConverter(uint16_t ticksPerQuarter, std::vector<MidiTempoEvent> tempos)
        : ticksPerQuarter_(ticksPerQuarter > 0 ? ticksPerQuarter : 96), tempos_(std::move(tempos))
    {
        tempos_.push_back(MidiTempoEvent{0, 500000});
        std::sort(tempos_.begin(), tempos_.end(), [](const MidiTempoEvent &a, const MidiTempoEvent &b) {
            return a.tick < b.tick;
        });
        std::vector<MidiTempoEvent> unique;
        for (const MidiTempoEvent &tempo : tempos_) {
            if (!unique.empty() && unique.back().tick == tempo.tick) {
                unique.back() = tempo;
            } else {
                unique.push_back(tempo);
            }
        }
        tempos_ = std::move(unique);
    }

    double ToSeconds(uint32_t tick) const
    {
        double seconds = 0.0;
        uint32_t lastTick = 0;
        uint32_t currentTempo = 500000;
        for (const MidiTempoEvent &tempo : tempos_) {
            if (tempo.tick > tick) {
                break;
            }
            if (tempo.tick > lastTick) {
                seconds += TicksToSeconds(tempo.tick - lastTick, currentTempo);
            }
            lastTick = tempo.tick;
            currentTempo = tempo.microsecondsPerQuarter;
        }
        if (tick > lastTick) {
            seconds += TicksToSeconds(tick - lastTick, currentTempo);
        }
        return seconds;
    }

private:
    double TicksToSeconds(uint32_t ticks, uint32_t tempo) const
    {
        return (static_cast<double>(ticks) * static_cast<double>(tempo)) /
            (1000000.0 * static_cast<double>(ticksPerQuarter_));
    }

    uint16_t ticksPerQuarter_;
    std::vector<MidiTempoEvent> tempos_;
};

bool ParseMidiTrack(const uint8_t *track, uint32_t trackLen, std::vector<MidiNoteEvent> &notes,
                    std::vector<MidiTempoEvent> &tempos)
{
    uint32_t offset = 0;
    uint32_t tick = 0;
    uint8_t runningStatus = 0;
    while (offset < trackLen) {
        uint32_t delta = 0;
        if (!ReadVarLen(track, trackLen, offset, delta)) {
            return false;
        }
        tick += delta;
        if (offset >= trackLen) {
            return false;
        }

        uint8_t status = track[offset++];
        if (status < 0x80U) {
            if (runningStatus == 0) {
                return false;
            }
            --offset;
            status = runningStatus;
        } else if (status < 0xF0U) {
            runningStatus = status;
        }

        if (status == 0xFFU) {
            if (offset >= trackLen) {
                return false;
            }
            const uint8_t metaType = track[offset++];
            uint32_t metaLen = 0;
            if (!ReadVarLen(track, trackLen, offset, metaLen) || offset + metaLen > trackLen) {
                return false;
            }
            if (metaType == 0x51U && metaLen == 3) {
                const uint32_t tempo = (static_cast<uint32_t>(track[offset]) << 16U) |
                    (static_cast<uint32_t>(track[offset + 1U]) << 8U) | track[offset + 2U];
                tempos.push_back(MidiTempoEvent{tick, tempo});
            }
            offset += metaLen;
            continue;
        }

        if (status == 0xF0U || status == 0xF7U) {
            uint32_t sysexLen = 0;
            if (!ReadVarLen(track, trackLen, offset, sysexLen) || offset + sysexLen > trackLen) {
                return false;
            }
            offset += sysexLen;
            continue;
        }

        const uint8_t eventType = status & 0xF0U;
        const bool twoDataBytes = eventType != 0xC0U && eventType != 0xD0U;
        if (offset + (twoDataBytes ? 2U : 1U) > trackLen) {
            return false;
        }
        const uint8_t data1 = track[offset++];
        const uint8_t data2 = twoDataBytes ? track[offset++] : 0;

        if (eventType == 0x90U && data2 > 0) {
            notes.push_back(MidiNoteEvent{tick, data1, data2, true});
        } else if (eventType == 0x80U || (eventType == 0x90U && data2 == 0)) {
            notes.push_back(MidiNoteEvent{tick, data1, data2, false});
        }
    }
    return true;
}

std::vector<int16_t> DecodeMidiToPcm(const uint8_t *data, uint32_t len)
{
    if (data == nullptr || len < 14 || std::memcmp(data, "MThd", 4) != 0) {
        return {};
    }
    const uint32_t headerLen = ReadBe32(data + 4);
    if (headerLen < 6 || 8U + headerLen > len) {
        return {};
    }
    const uint16_t trackCount = ReadBe16(data + 10);
    const uint16_t division = ReadBe16(data + 12);
    if ((division & 0x8000U) != 0 || trackCount == 0) {
        return {};
    }

    std::vector<MidiNoteEvent> noteEvents;
    std::vector<MidiTempoEvent> tempoEvents;
    uint32_t offset = 8U + headerLen;
    for (uint16_t trackIndex = 0; trackIndex < trackCount && offset + 8U <= len; ++trackIndex) {
        if (std::memcmp(data + offset, "MTrk", 4) != 0) {
            return {};
        }
        const uint32_t trackLen = ReadBe32(data + offset + 4U);
        offset += 8U;
        if (offset + trackLen > len) {
            return {};
        }
        (void)ParseMidiTrack(data + offset, trackLen, noteEvents, tempoEvents);
        offset += trackLen;
    }

    if (noteEvents.empty()) {
        return {};
    }
    std::sort(noteEvents.begin(), noteEvents.end(), [](const MidiNoteEvent &a, const MidiNoteEvent &b) {
        if (a.tick != b.tick) {
            return a.tick < b.tick;
        }
        return a.on < b.on;
    });

    std::vector<MidiNoteInterval> intervals;
    std::map<uint8_t, std::vector<MidiNoteEvent>> activeNotes;
    uint32_t maxTick = 0;
    for (const MidiNoteEvent &event : noteEvents) {
        maxTick = std::max(maxTick, event.tick);
        if (event.on) {
            activeNotes[event.note].push_back(event);
        } else {
            auto &stack = activeNotes[event.note];
            if (!stack.empty()) {
                const MidiNoteEvent start = stack.back();
                stack.pop_back();
                if (event.tick > start.tick) {
                    intervals.push_back(MidiNoteInterval{start.tick, event.tick, start.note, start.velocity});
                }
            }
        }
    }
    for (auto &entry : activeNotes) {
        for (const MidiNoteEvent &start : entry.second) {
            if (maxTick > start.tick) {
                intervals.push_back(MidiNoteInterval{start.tick, maxTick, start.note, start.velocity});
            }
        }
    }
    if (intervals.empty()) {
        return {};
    }

    MidiTickConverter converter(division, std::move(tempoEvents));
    double durationSeconds = converter.ToSeconds(maxTick) + 0.35;
    durationSeconds = std::min<double>(durationSeconds, 90.0);
    const size_t frameCount = static_cast<size_t>(durationSeconds * kOutputSampleRate);
    std::vector<float> mix(frameCount, 0.0F);

    for (const MidiNoteInterval &interval : intervals) {
        const double startSeconds = converter.ToSeconds(interval.startTick);
        const double endSeconds = std::min<double>(converter.ToSeconds(interval.endTick), durationSeconds);
        if (endSeconds <= startSeconds) {
            continue;
        }
        const size_t startFrame = std::min(frameCount, static_cast<size_t>(startSeconds * kOutputSampleRate));
        const size_t endFrame = std::min(frameCount, static_cast<size_t>(endSeconds * kOutputSampleRate));
        const double frequency = 440.0 * std::pow(2.0, (static_cast<double>(interval.note) - 69.0) / 12.0);
        const float amplitude = 0.10F * (static_cast<float>(interval.velocity) / 127.0F);
        for (size_t frame = startFrame; frame < endFrame; ++frame) {
            const double t = static_cast<double>(frame - startFrame) / static_cast<double>(kOutputSampleRate);
            const size_t noteFrame = frame - startFrame;
            const size_t noteFrames = std::max<size_t>(1, endFrame - startFrame);
            const float attack = std::min<float>(1.0F, static_cast<float>(noteFrame) / 320.0F);
            const float release = std::min<float>(1.0F, static_cast<float>(noteFrames - noteFrame) / 960.0F);
            const float envelope = std::min(attack, release);
            mix[frame] += static_cast<float>(std::sin(2.0 * kPi * frequency * t)) * amplitude * envelope;
        }
    }

    std::vector<int16_t> output(frameCount * kOutputChannels);
    for (size_t frame = 0; frame < frameCount; ++frame) {
        const int16_t sample = ClampSample(static_cast<int32_t>(mix[frame] * 32767.0F));
        output[frame * 2U] = sample;
        output[frame * 2U + 1U] = sample;
    }
    return output;
}

std::vector<int16_t> ResampleToStereo48k(const int16_t *input, size_t inputFrames, int32_t inputRate, int32_t inputChannels)
{
    if (input == nullptr || inputFrames == 0 || inputRate <= 0 || inputChannels <= 0) {
        return {};
    }
    const double ratio = static_cast<double>(inputRate) / static_cast<double>(kOutputSampleRate);
    const size_t outputFrames = std::max<size_t>(1, static_cast<size_t>(
        (static_cast<double>(inputFrames) * static_cast<double>(kOutputSampleRate)) / static_cast<double>(inputRate)));
    std::vector<int16_t> output(outputFrames * kOutputChannels);
    for (size_t frame = 0; frame < outputFrames; ++frame) {
        const double src = std::min<double>(static_cast<double>(inputFrames - 1U), static_cast<double>(frame) * ratio);
        const size_t src0 = static_cast<size_t>(src);
        const size_t src1 = std::min(inputFrames - 1U, src0 + 1U);
        const double frac = src - static_cast<double>(src0);
        const int32_t left0 = input[src0 * static_cast<size_t>(inputChannels)];
        const int32_t left1 = input[src1 * static_cast<size_t>(inputChannels)];
        const int32_t right0 = inputChannels > 1 ? input[src0 * static_cast<size_t>(inputChannels) + 1U] : left0;
        const int32_t right1 = inputChannels > 1 ? input[src1 * static_cast<size_t>(inputChannels) + 1U] : left1;
        const int32_t left = static_cast<int32_t>((static_cast<double>(left0) * (1.0 - frac)) +
            (static_cast<double>(left1) * frac));
        const int32_t right = static_cast<int32_t>((static_cast<double>(right0) * (1.0 - frac)) +
            (static_cast<double>(right1) * frac));
        output[frame * 2U] = ClampSample(left);
        output[frame * 2U + 1U] = ClampSample(right);
    }
    return output;
}

std::vector<int16_t> DecodeRawPcm8k16Mono(const uint8_t *data, uint32_t len)
{
    const size_t sampleCount = len / sizeof(int16_t);
    if (data == nullptr || sampleCount == 0) {
        return {};
    }
    std::vector<int16_t> mono(sampleCount);
    std::memcpy(mono.data(), data, sampleCount * sizeof(int16_t));
    return ResampleToStereo48k(mono.data(), sampleCount, 8000, 1);
}

std::vector<int16_t> DecodePcmWav(const uint8_t *data, uint32_t len)
{
    if (data == nullptr || len < 44 || std::memcmp(data, "RIFF", 4) != 0 || std::memcmp(data + 8, "WAVE", 4) != 0) {
        return {};
    }

    uint16_t audioFormat = 0;
    uint16_t channelCount = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    const uint8_t *audioData = nullptr;
    uint32_t audioDataLen = 0;

    uint32_t offset = 12;
    while (offset + 8 <= len) {
        const uint8_t *chunk = data + offset;
        const uint32_t chunkLen = ReadLe32(chunk + 4);
        const uint32_t nextOffset = offset + 8U + chunkLen + (chunkLen & 1U);
        if (nextOffset > len + 1U) {
            break;
        }
        if (std::memcmp(chunk, "fmt ", 4) == 0 && chunkLen >= 16) {
            const uint8_t *fmt = chunk + 8;
            audioFormat = ReadLe16(fmt);
            channelCount = ReadLe16(fmt + 2);
            sampleRate = ReadLe32(fmt + 4);
            bitsPerSample = ReadLe16(fmt + 14);
        } else if (std::memcmp(chunk, "data", 4) == 0) {
            audioData = chunk + 8;
            audioDataLen = std::min<uint32_t>(chunkLen, len - offset - 8U);
        }
        offset = nextOffset;
    }

    if (audioFormat != 1 || audioData == nullptr || audioDataLen == 0 || channelCount == 0 || sampleRate == 0) {
        return {};
    }

    std::vector<int16_t> decoded;
    if (bitsPerSample == 16) {
        const size_t sampleCount = audioDataLen / sizeof(int16_t);
        decoded.resize(sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) {
            decoded[i] = static_cast<int16_t>(ReadLe16(audioData + i * sizeof(int16_t)));
        }
    } else if (bitsPerSample == 8) {
        decoded.reserve(audioDataLen);
        for (uint32_t i = 0; i < audioDataLen; ++i) {
            decoded.push_back(static_cast<int16_t>((static_cast<int32_t>(audioData[i]) - 128) << 8));
        }
    } else if (bitsPerSample == 24) {
        const size_t sampleCount = audioDataLen / 3U;
        decoded.reserve(sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) {
            const uint8_t *sample = audioData + i * 3U;
            int32_t value = static_cast<int32_t>(sample[0]) | (static_cast<int32_t>(sample[1]) << 8) |
                (static_cast<int32_t>(sample[2]) << 16);
            if ((value & 0x800000) != 0) {
                value |= ~0xFFFFFF;
            }
            decoded.push_back(ClampSample(value >> 8));
        }
    } else {
        return {};
    }

    const size_t inputFrames = decoded.size() / channelCount;
    return ResampleToStereo48k(decoded.data(), inputFrames, static_cast<int32_t>(sampleRate), channelCount);
}

bool IsCompressedSoundType(int32_t type)
{
    return type == MR_SOUND_MP3 || type == MR_SOUND_M4A || type == MR_SOUND_AMR || type == MR_SOUND_AMR_WB;
}

const char *CompressedSoundExtension(int32_t type)
{
    switch (type) {
        case MR_SOUND_MP3:
            return "mp3";
        case MR_SOUND_M4A:
            return "m4a";
        case MR_SOUND_AMR:
            return "amr";
        case MR_SOUND_AMR_WB:
            return "awb";
        default:
            return "bin";
    }
}

bool WriteAll(int32_t fd, const uint8_t *data, uint32_t len)
{
    uint32_t written = 0;
    while (written < len) {
        const ssize_t ret = write(fd, data + written, len - written);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (ret == 0) {
            return false;
        }
        written += static_cast<uint32_t>(ret);
    }
    return true;
}

class MrpAudioPlayer {
public:
    int32_t Play(int32_t type, const void *data, uint32_t dataLen, int32_t loop)
    {
        if (data == nullptr || dataLen == 0) {
            OH_LOG_Print(LOG_APP, LOG_WARN, 0xFF00, "MRP",
                         "audio invalid play type=%{public}d bytes=%{public}u loop=%{public}d",
                         type, dataLen, loop);
            return MR_FAILED;
        }

        std::vector<int16_t> samples;
        const auto *bytes = static_cast<const uint8_t *>(data);
        if (type == MR_SOUND_MIDI) {
            samples = DecodeMidiToPcm(bytes, dataLen);
        } else if (type == MR_SOUND_PCM) {
            samples = DecodeRawPcm8k16Mono(bytes, dataLen);
        } else if (type == MR_SOUND_WAV) {
            samples = DecodePcmWav(bytes, dataLen);
        } else if (IsCompressedSoundType(type)) {
            return PlayCompressed(type, bytes, dataLen, loop);
        }

        if (samples.empty()) {
            OH_LOG_Print(LOG_APP, LOG_WARN, 0xFF00, "MRP",
                         "audio unsupported type=%{public}d bytes=%{public}u loop=%{public}d",
                         type, dataLen, loop);
            return MR_SUCCESS;
        } else {
            OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                         "audio play type=%{public}d bytes=%{public}u frames=%{public}zu loop=%{public}d",
                         type, dataLen, samples.size() / 2U, loop);
        }

        StopCompressed(-1);
        if (!EnsureRenderer()) {
            return MR_FAILED;
        }
        if (rendererStarted_) {
            (void)OH_AudioRenderer_Stop(renderer_);
            (void)OH_AudioRenderer_Flush(renderer_);
            rendererStarted_ = false;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            samples_ = std::move(samples);
            position_ = 0;
            loop_ = loop != 0;
            activeType_ = type;
            active_ = true;
            stopping_ = false;
            stopRampFramesLeft_ = 0;
        }
        const OH_AudioStream_Result ret = OH_AudioRenderer_Start(renderer_);
        rendererStarted_ = ret == AUDIOSTREAM_SUCCESS;
        if (!rendererStarted_) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP", "audio start failed ret=%{public}d", ret);
        }
        return rendererStarted_ ? MR_SUCCESS : MR_FAILED;
    }

    int32_t Stop(int32_t type)
    {
        StopCompressed(type);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!active_) {
                return MR_SUCCESS;
            }
            if (type >= 0 && type != activeType_) {
                OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                             "audio stop ignored type=%{public}d activeType=%{public}d", type, activeType_);
                return MR_SUCCESS;
            }
            stopping_ = true;
            loop_ = false;
            stopRampFramesLeft_ = kStopRampFrames;
        }
        return MR_SUCCESS;
    }

    void Release()
    {
        StopCompressed(-1);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_ = false;
            stopping_ = false;
            position_ = 0;
            samples_.clear();
        }
        if (renderer_ != nullptr) {
            if (rendererStarted_) {
                (void)OH_AudioRenderer_Stop(renderer_);
                rendererStarted_ = false;
            }
            (void)OH_AudioRenderer_Release(renderer_);
            renderer_ = nullptr;
        }
    }

    int32_t OnWriteData(void *buffer, int32_t length)
    {
        if (buffer == nullptr || length <= 0) {
            return 0;
        }
        auto *out = static_cast<uint8_t *>(buffer);
        std::fill(out, out + length, 0);

        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_ || samples_.empty()) {
            return 0;
        }

        size_t bytesWritten = 0;
        const size_t totalBytes = samples_.size() * sizeof(int16_t);
        auto *sampleBytes = reinterpret_cast<const uint8_t *>(samples_.data());
        while (bytesWritten < static_cast<size_t>(length) && active_) {
            const size_t remainingSource = totalBytes - position_;
            const size_t remainingTarget = static_cast<size_t>(length) - bytesWritten;
            const size_t copyBytes = std::min(remainingSource, remainingTarget);
            std::memcpy(out + bytesWritten, sampleBytes + position_, copyBytes);
            if (stopping_) {
                ApplyStopRamp(out + bytesWritten, copyBytes);
                if (!active_) {
                    bytesWritten += copyBytes;
                    break;
                }
            }
            bytesWritten += copyBytes;
            position_ += copyBytes;
            if (position_ >= totalBytes) {
                if (loop_) {
                    position_ = 0;
                } else {
                    active_ = false;
                }
            }
        }
        return 0;
    }

private:
    int32_t PlayCompressed(int32_t type, const uint8_t *data, uint32_t dataLen, int32_t loop)
    {
        StopRawPlayback();
        StopCompressed(-1);

        (void)mkdir(".tmp", S_IRWXU | S_IRWXG | S_IRWXO);
        const std::string path = NextCompressedTempPath(type);
        const int32_t writeFd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (writeFd < 0) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP",
                         "audio compressed temp open failed type=%{public}d path=%{public}s errno=%{public}d",
                         type, path.c_str(), errno);
            return MR_FAILED;
        }
        const bool writeOk = WriteAll(writeFd, data, dataLen);
        const int32_t closeWriteRet = close(writeFd);
        if (!writeOk || closeWriteRet != 0) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP",
                         "audio compressed temp write failed type=%{public}d path=%{public}s errno=%{public}d",
                         type, path.c_str(), errno);
            (void)unlink(path.c_str());
            return MR_FAILED;
        }

        const int32_t readFd = open(path.c_str(), O_RDONLY);
        if (readFd < 0) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP",
                         "audio compressed temp read failed type=%{public}d path=%{public}s errno=%{public}d",
                         type, path.c_str(), errno);
            (void)unlink(path.c_str());
            return MR_FAILED;
        }

        OH_AVPlayer *player = OH_AVPlayer_Create();
        if (player == nullptr) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP", "audio avplayer create failed");
            (void)close(readFd);
            (void)unlink(path.c_str());
            return MR_FAILED;
        }

        {
            std::lock_guard<std::mutex> lock(mediaMutex_);
            compressedPlayer_ = player;
            compressedFd_ = readFd;
            compressedPath_ = path;
            compressedActiveType_ = type;
            compressedLoop_ = loop != 0;
            compressedStarted_ = false;
        }

        OH_AVErrCode ret = OH_AVPlayer_SetOnInfoCallback(player, &MrpAudioPlayer::MediaInfoCallback, this);
        if (ret == AV_ERR_OK) {
            ret = OH_AVPlayer_SetOnErrorCallback(player, &MrpAudioPlayer::MediaErrorCallback, this);
        }
        if (ret == AV_ERR_OK) {
            ret = OH_AVPlayer_SetFDSource(player, readFd, 0, dataLen);
        }
        if (ret == AV_ERR_OK) {
            (void)OH_AVPlayer_SetAudioRendererInfo(player, AUDIOSTREAM_USAGE_MUSIC);
            (void)OH_AVPlayer_SetLooping(player, loop != 0);
            ret = OH_AVPlayer_Prepare(player);
        }
        if (ret != AV_ERR_OK) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP",
                         "audio avplayer prepare failed type=%{public}d bytes=%{public}u ret=%{public}d",
                         type, dataLen, ret);
            StopCompressed(type);
            return MR_FAILED;
        }

        OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                     "audio avplayer prepare type=%{public}d bytes=%{public}u loop=%{public}d path=%{public}s",
                     type, dataLen, loop, path.c_str());
        return MR_SUCCESS;
    }

    std::string NextCompressedTempPath(int32_t type)
    {
        std::lock_guard<std::mutex> lock(mediaMutex_);
        ++compressedSequence_;
        char path[160] = {0};
        (void)snprintf(path, sizeof(path), ".tmp/mrp_audio_%zu_type%d.%s",
                       compressedSequence_, type, CompressedSoundExtension(type));
        return std::string(path);
    }

    void StopRawPlayback()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_ = false;
            stopping_ = false;
            position_ = 0;
            loop_ = false;
            samples_.clear();
        }
        if (renderer_ != nullptr && rendererStarted_) {
            (void)OH_AudioRenderer_Stop(renderer_);
            (void)OH_AudioRenderer_Flush(renderer_);
            rendererStarted_ = false;
        }
    }

    void StopCompressed(int32_t type)
    {
        OH_AVPlayer *player = nullptr;
        int32_t fd = -1;
        std::string path;
        {
            std::lock_guard<std::mutex> lock(mediaMutex_);
            if (compressedPlayer_ == nullptr) {
                return;
            }
            if (type >= 0 && type != compressedActiveType_) {
                return;
            }
            player = compressedPlayer_;
            fd = compressedFd_;
            path = compressedPath_;
            compressedPlayer_ = nullptr;
            compressedFd_ = -1;
            compressedPath_.clear();
            compressedActiveType_ = -1;
            compressedLoop_ = false;
            compressedStarted_ = false;
        }
        (void)OH_AVPlayer_Stop(player);
        (void)OH_AVPlayer_ReleaseSync(player);
        if (fd >= 0) {
            (void)close(fd);
        }
        if (!path.empty()) {
            (void)unlink(path.c_str());
        }
    }

    void OnMediaInfo(OH_AVPlayer *player, AVPlayerOnInfoType type, OH_AVFormat *infoBody)
    {
        if (type == AV_INFO_TYPE_EOS) {
            OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", "audio avplayer eos");
            return;
        }
        if (type != AV_INFO_TYPE_STATE_CHANGE) {
            return;
        }

        int32_t stateValue = -1;
        if (infoBody == nullptr || !OH_AVFormat_GetIntValue(infoBody, OH_PLAYER_STATE, &stateValue)) {
            return;
        }

        bool shouldPlay = false;
        bool loop = false;
        {
            std::lock_guard<std::mutex> lock(mediaMutex_);
            if (player != compressedPlayer_) {
                return;
            }
            if (stateValue == AV_PREPARED && !compressedStarted_) {
                compressedStarted_ = true;
                shouldPlay = true;
                loop = compressedLoop_;
            }
        }
        if (shouldPlay) {
            (void)OH_AVPlayer_SetLooping(player, loop);
            const OH_AVErrCode ret = OH_AVPlayer_Play(player);
            OH_LOG_Print(LOG_APP, ret == AV_ERR_OK ? LOG_INFO : LOG_ERROR, 0xFF00, "MRP",
                         "audio avplayer play ret=%{public}d loop=%{public}d", ret, loop ? 1 : 0);
        }
    }

    void OnMediaError(OH_AVPlayer *player, int32_t errorCode, const char *errorMsg)
    {
        bool current = false;
        {
            std::lock_guard<std::mutex> lock(mediaMutex_);
            current = player == compressedPlayer_;
        }
        if (current) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP",
                         "audio avplayer error code=%{public}d message=%{public}s",
                         errorCode, errorMsg != nullptr ? errorMsg : "");
        }
    }

    static void MediaInfoCallback(OH_AVPlayer *player, AVPlayerOnInfoType type, OH_AVFormat *infoBody, void *userData)
    {
        auto *self = static_cast<MrpAudioPlayer *>(userData);
        if (self != nullptr) {
            self->OnMediaInfo(player, type, infoBody);
        }
    }

    static void MediaErrorCallback(OH_AVPlayer *player, int32_t errorCode, const char *errorMsg, void *userData)
    {
        auto *self = static_cast<MrpAudioPlayer *>(userData);
        if (self != nullptr) {
            self->OnMediaError(player, errorCode, errorMsg);
        }
    }

    bool EnsureRenderer()
    {
        if (renderer_ != nullptr) {
            return true;
        }

        OH_AudioStreamBuilder *builder = nullptr;
        if (OH_AudioStreamBuilder_Create(&builder, AUDIOSTREAM_TYPE_RENDERER) != AUDIOSTREAM_SUCCESS || builder == nullptr) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP", "audio builder create failed");
            return false;
        }

        (void)OH_AudioStreamBuilder_SetSamplingRate(builder, kOutputSampleRate);
        (void)OH_AudioStreamBuilder_SetChannelCount(builder, kOutputChannels);
        (void)OH_AudioStreamBuilder_SetSampleFormat(builder, AUDIOSTREAM_SAMPLE_S16LE);
        (void)OH_AudioStreamBuilder_SetEncodingType(builder, AUDIOSTREAM_ENCODING_TYPE_RAW);
        (void)OH_AudioStreamBuilder_SetRendererInfo(builder, AUDIOSTREAM_USAGE_MUSIC);
        (void)OH_AudioStreamBuilder_SetLatencyMode(builder, AUDIOSTREAM_LATENCY_MODE_FAST);
        (void)OH_AudioStreamBuilder_SetFrameSizeInCallback(builder, kCallbackFrames);

        OH_AudioRenderer_Callbacks callbacks;
        std::memset(&callbacks, 0, sizeof(callbacks));
        callbacks.OH_AudioRenderer_OnWriteData = &MrpAudioPlayer::WriteDataCallback;
        (void)OH_AudioStreamBuilder_SetRendererCallback(builder, callbacks, this);

        const OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &renderer_);
        (void)OH_AudioStreamBuilder_Destroy(builder);
        if (result != AUDIOSTREAM_SUCCESS || renderer_ == nullptr) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP", "audio renderer create failed ret=%{public}d", result);
            renderer_ = nullptr;
            return false;
        }
        int32_t actualFrameSize = 0;
        (void)OH_AudioRenderer_GetFrameSizeInCallback(renderer_, &actualFrameSize);
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", "audio renderer ready callbackFrames=%{public}d", actualFrameSize);
        return true;
    }

    void ApplyStopRamp(uint8_t *data, size_t byteCount)
    {
        auto *samples = reinterpret_cast<int16_t *>(data);
        const size_t sampleCount = byteCount / sizeof(int16_t);
        for (size_t i = 0; i + 1U < sampleCount; i += kOutputChannels) {
            const float gain = stopRampFramesLeft_ == 0 ? 0.0F :
                static_cast<float>(stopRampFramesLeft_) / static_cast<float>(kStopRampFrames);
            samples[i] = ClampSample(static_cast<int32_t>(static_cast<float>(samples[i]) * gain));
            samples[i + 1U] = ClampSample(static_cast<int32_t>(static_cast<float>(samples[i + 1U]) * gain));
            if (stopRampFramesLeft_ > 0) {
                --stopRampFramesLeft_;
            }
            if (stopRampFramesLeft_ == 0) {
                active_ = false;
                position_ = 0;
                stopping_ = false;
                const size_t clearOffset = (i + kOutputChannels) * sizeof(int16_t);
                if (clearOffset < byteCount) {
                    std::memset(data + clearOffset, 0, byteCount - clearOffset);
                }
                break;
            }
        }
    }

    static int32_t WriteDataCallback(OH_AudioRenderer *renderer, void *userData, void *buffer, int32_t length)
    {
        (void)renderer;
        auto *player = static_cast<MrpAudioPlayer *>(userData);
        return player != nullptr ? player->OnWriteData(buffer, length) : 0;
    }

    std::mutex mutex_;
    OH_AudioRenderer *renderer_ = nullptr;
    std::vector<int16_t> samples_;
    size_t position_ = 0;
    int32_t activeType_ = -1;
    size_t stopRampFramesLeft_ = 0;
    bool loop_ = false;
    bool active_ = false;
    bool stopping_ = false;
    bool rendererStarted_ = false;

    std::mutex mediaMutex_;
    OH_AVPlayer *compressedPlayer_ = nullptr;
    int32_t compressedFd_ = -1;
    int32_t compressedActiveType_ = -1;
    size_t compressedSequence_ = 0;
    bool compressedLoop_ = false;
    bool compressedStarted_ = false;
    std::string compressedPath_;
};

MrpAudioPlayer g_audioPlayer;
} // namespace

extern "C" int32_t vmrpHostPlaySound(int32_t type, const void *data, uint32_t dataLen, int32_t loop)
{
    return g_audioPlayer.Play(type, data, dataLen, loop);
}

extern "C" int32_t vmrpHostStopSound(int32_t type)
{
    return g_audioPlayer.Stop(type);
}

extern "C" void vmrpHostReleaseAudio()
{
    g_audioPlayer.Release();
}
