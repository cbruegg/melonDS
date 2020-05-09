#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else

#include <unistd.h>
#include <atomic>

#endif // _WIN32

#include "../NDS.h"
#include "../GPU3D.h"
#include "../SPU.h"
#include "../GPU.h"
#include "NamedOutPipe.h"
#include "NamedInPipe.h"
#include "Semaphore.h"
#include "CommandHandler.h"

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;
typedef std::chrono::duration<float> fsec;

const int videoFps = 60;
const int videoFrameDurationMs = 1000 / videoFps;
const int emulationTargetFps = 60;
const int emulationTargetFrameDurationMs = 1000 / emulationTargetFps;

void sleepCp(long milliseconds) {
    if (milliseconds <= 0) {
        return;
    }

#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif // _WIN32
}

// TODO Ignore sigpipe, handle manually

static std::string biosDirPath;

int main(int argc, char **argv) {
    if (argc < 6) {
        std::cerr << "Usage: biosDirPath screenPipePath audioPipePath inputPipePath romPath" << std::endl;
    }

    std::mutex bufMutex;

    const int singleScreenSize = 256 * 192 * 4; // BGRA8
    u32 *readyScreenBuffer = new u32[singleScreenSize * 2]{0};
    u32 *wipScreenBuffer = new u32[singleScreenSize * 2]{0};

    const int audioBufferSize = 0x1000;
    int16_t audioBuffer[audioBufferSize] = {0}; // PCM16, 32768 Hz, 2 Channels, Interleaved

    biosDirPath = std::string(argv[1]);
    NamedOutPipe screenPipe(argv[2]);
    NamedOutPipe audioPipe(argv[3]);
    NamedInPipe inputPipe(argv[4]);
    std::string romPath(argv[5]);

    std::atomic<double> speedup(1.0);

    NDS::Init();
    GPU3D::InitRenderer(false);

    const auto ndsIdx = romPath.rfind(".nds");
    if (ndsIdx == -1) {
        std::cerr << "ROM needs to have .nds extension!" << std::endl;
        return 1;
    }

    std::string romDsvPath = romPath.substr(0, ndsIdx) + ".dsv";
    if (!NDS::LoadROM(romPath.c_str(), romDsvPath.c_str(), true)) {
        std::cerr << "Failed to load ROM!" << std::endl;
        return 1;
    }

//    NDS::LoadBIOS();

    auto globalStart = std::chrono::system_clock::now();
    double frames = 0;

    NDS::RunFrame();

    std::thread writer([&screenPipe, &readyScreenBuffer, &audioPipe, &audioBuffer, &bufMutex]() {
        while (true) {
            auto start = Time::now();
            bufMutex.lock();
            {
                screenPipe.writeData(readyScreenBuffer, singleScreenSize);
                screenPipe.writeData(readyScreenBuffer + singleScreenSize, singleScreenSize);
            }
            bufMutex.unlock();
            auto end = Time::now();
            ms elapsed = std::chrono::duration_cast<ms>(end - start);
            sleepCp(videoFrameDurationMs - elapsed.count());
        }
    });

    std::thread commandReader([&inputPipe, &speedup]() {
        while (true) {
            const auto line = inputPipe.readLine();
            const auto command = parseCommand(line);
            const auto commandType = &command.commandType;

            if (commandType == &CommandType::Stop) {
                std::cout << "Received Stop" << std::endl;
            } else if (commandType == &CommandType::Pause) {
                std::cout << "Received Pause" << std::endl;
            } else if (commandType == &CommandType::Resume) {
                std::cout << "Received Resume" << std::endl;
            } else if (commandType == &CommandType::KeyPress) {
                std::cout << "Received KeyPress" << std::endl;
            } else if (commandType == &CommandType::KeyRelease) {
                std::cout << "Received KeyRelease" << std::endl;
            } else if (commandType == &CommandType::Touch) {
                std::cout << "Received Touch" << std::endl;
            } else if (commandType == &CommandType::TouchRelease) {
                std::cout << "Received TouchRelease" << std::endl;
            } else if (commandType == &CommandType::ResetInput) {
                std::cout << "Received ResetInput" << std::endl;
            } else if (commandType == &CommandType::SaveGameSave) {
                std::cout << "Received SaveGameSave" << std::endl;
            } else if (commandType == &CommandType::LoadGameSave) {
                std::cout << "Received LoadGameSave" << std::endl;
            } else if (commandType == &CommandType::SaveState) {
                std::cout << "Received SaveState" << std::endl;
            } else if (commandType == &CommandType::LoadState) {
                std::cout << "Received LoadState" << std::endl;
            } else if (commandType == &CommandType::AddCheat) {
                std::cout << "Received AddCheat" << std::endl;
            } else if (commandType == &CommandType::ResetCheats) {
                std::cout << "Received ResetCheats" << std::endl;
            } else if (commandType == &CommandType::SetSpeed) {
                const auto data = static_cast<SetSpeedCommandData*>(command.commandData);
                std::cout << "Received SetSpeed " << data->speed << std::endl;
                speedup = data->speed;
                delete data;
            } else if (commandType == &CommandType::Malformed) {
                std::cerr << "Received malformed command!" << std::endl;
            }
        }
    });

    while (true) {
        // TODO Apply input
        auto start = Time::now();

        NDS::RunFrame();

        u32 availableBytes = SPU::GetOutputSize();
        availableBytes = std::max(availableBytes, (u32) (sizeof(audioBuffer) / (2 * sizeof(int16_t))));
        int samplesToWrite = SPU::ReadOutput(audioBuffer, availableBytes);
        audioPipe.writeData(audioBuffer, samplesToWrite * 4);

        memcpy(wipScreenBuffer, GPU::Framebuffer[GPU::FrontBuffer][0], singleScreenSize);
        memcpy(wipScreenBuffer + singleScreenSize, GPU::Framebuffer[GPU::FrontBuffer][1], singleScreenSize);

        bufMutex.lock();
        {
            u32 *tmp = readyScreenBuffer;
            readyScreenBuffer = wipScreenBuffer;
            wipScreenBuffer = tmp;
        }
        bufMutex.unlock();

        auto curEnd = std::chrono::system_clock::now();
        frames++;
        std::chrono::duration<double> elapsedSeconds = curEnd - globalStart;
        if ((uint64_t) frames % 30 == 0) {
            auto fps = frames / elapsedSeconds.count();
            std::cout << "FPS: " << (int) fps << std::endl;
        }

        auto end = Time::now();
        ms elapsed = std::chrono::duration_cast<ms>(end - start);
        sleepCp(emulationTargetFrameDurationMs / speedup - elapsed.count());
    }

    // TODO Kill threads

    screenPipe.closePipe();
    audioPipe.closePipe();
    inputPipe.closePipe();

}

namespace Platform {

    void StopEmu() {
        // TODO
    }

    FILE *OpenFile(const char *path, const char *mode, bool mustexist) {
        FILE *ret;

        if (mustexist) {
            ret = fopen(path, "rb");
            if (ret) ret = freopen(path, mode, ret);
        } else {
            ret = fopen(path, mode);
        }
        return ret;
    }

    FILE *OpenLocalFile(const char *path, const char *mode) {
        const auto completePath = biosDirPath + "/" + path;
        return fopen(completePath.c_str(), mode);
    }

    FILE *OpenDataFile(const char *path) {
        return fopen(path, "rb");
    }

    void *Thread_Create(void (*func)()) {
        return new std::thread(func);
    }

    void Thread_Free(void *thread) {
        delete static_cast<std::thread *>(thread);
    }

    void Thread_Wait(void *thread) {
        static_cast<std::thread *>(thread)->join();
    }

    void *Semaphore_Create() {
        return new Semaphore();
    }

    void Semaphore_Free(void *semaphore) {
        delete static_cast<Semaphore *>(semaphore);
    }

    void Semaphore_Reset(void *semaphore) {
        static_cast<Semaphore *>(semaphore)->reset();
    }

    void Semaphore_Wait(void *semaphore) {
        static_cast<Semaphore *>(semaphore)->wait();
    }

    void Semaphore_Post(void *semaphore) {
        static_cast<Semaphore *>(semaphore)->notify();
    }

    void *GL_GetProcAddress(const char *proc) {
        return nullptr;
    }

    bool MP_Init() {
        return false;
    }

    void MP_DeInit() {
    }

    int MP_SendPacket(u8 *bytes, int len) {
        return 0;
    }

    int MP_RecvPacket(u8 *bytes, bool block) {
        return 0;
    }

    bool LAN_Init() {
        return false;
    }

    void LAN_DeInit() {
    }

    int LAN_SendPacket(u8 *data, int len) {
        return 0;
    }

    int LAN_RecvPacket(u8 *data) {
        return 0;
    }
}

namespace GPU3D {
    namespace GLRenderer {
        bool Init() {
            return false;
        }

        void DeInit() {
        }

        void Reset() {
        }

        void UpdateDisplaySettings() {
        }

        void RenderFrame() {
        }

        void PrepareCaptureFrame() {
        }

        u32 *GetLine(int line) {
            return nullptr;
        }

        void SetupAccelFrame() {
        }
    }
}

