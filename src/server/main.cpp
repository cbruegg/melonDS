#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <unistd.h>
#include <atomic>

#ifdef _WIN32

#include <windows.h>

#else
#endif // _WIN32

#include "../NDS.h"
#include "../GPU3D.h"
#include "../SPU.h"
#include "../GPU.h"
#include "Socket.h"
#include "Semaphore.h"
#include "CommandHandler.h"

typedef std::chrono::high_resolution_clock Time;

const unsigned int MelonDSGameInputTouchScreenX = 4096;
const unsigned int MelonDSGameInputTouchScreenY = 8192;

// TODO Ignore sigpipe, handle manually

static std::string biosDirPath;

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: biosDirPath romPath saveGamePath" << std::endl;
        return 1;
    }

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    std::mutex bufMutex;

    const int singleScreenSize = 256 * 192; // BGRA8

    const int audioBufferSize = 0x1000;
    int16_t audioBuffer[audioBufferSize] = {0}; // PCM16, 32768 Hz, 2 Channels, Interleaved

    biosDirPath = std::string(argv[1]);
    std::string romPath(argv[2]);
    std::string saveGamePath(argv[3]);

    std::atomic<double> speedup(1.0);
    std::atomic<bool> stop(false);
    std::atomic<uint32_t> activatedInputs(0);
    std::atomic<u16> touchX(0);
    std::atomic<u16> touchY(0);

    NDS::Init();
    GPU3D::InitRenderer(false);

    const auto ndsIdx = romPath.rfind(".nds");
    if (ndsIdx == -1) {
        std::cerr << "ROM needs to have .nds extension!" << std::endl;
        return 1;
    }

    if (!NDS::LoadROM(romPath.c_str(), saveGamePath.c_str(), true)) {
        std::cerr << "Failed to load ROM!" << std::endl;
        return 1;
    }

    Socket screenPipe;
    Socket audioPipe;
    Socket inputPipe;
    std::cout << "[SERVOUT] screen: :" << screenPipe.port << std::endl;
    std::cout << "[SERVOUT] audio: :" << audioPipe.port << std::endl;
    std::cout << "[SERVOUT] input: :" << inputPipe.port << std::endl;

//    NDS::LoadBIOS();

    auto globalStart = std::chrono::system_clock::now();
    double frames = 0;

    NDS::RunFrame();

    std::thread commandReader([&inputPipe, &speedup, &stop, &activatedInputs, &touchX, &touchY]() {
        while (!stop) {
            std::cout << "Reading next command..." << std::endl;
            const auto line = inputPipe.readLine();
            if (line.empty()) {
                std::cout << "Command was empty!" << std::endl;
                continue;
            }

            const auto command = parseCommand(line);
            const auto commandType = &command.commandType;

            if (commandType == &CommandType::Stop) {
                std::cout << "Received Stop" << std::endl;
                stop = true;
            } else if (commandType == &CommandType::Pause) {
                std::cout << "Received Pause" << std::endl;
            } else if (commandType == &CommandType::Resume) {
                std::cout << "Received Resume" << std::endl;
            } else if (commandType == &CommandType::ActivateInput) {
                const auto data = static_cast<ActivateInputCommandData *>(command.commandData);
                std::cout << "Received ActivateInput " << data->input << " " << data->value << std::endl;
                activatedInputs |= (uint32_t) data->input;

                switch (data->input) {
                    case MelonDSGameInputTouchScreenX:
                        touchX = data->value * (256 - 1);
                        break;
                    case MelonDSGameInputTouchScreenY:
                        touchY = data->value * (192 - 1);
                        break;
                }
                delete data;
            } else if (commandType == &CommandType::DeactivateInput) {
                const auto data = static_cast<DeactivateInputCommandData *>(command.commandData);
                std::cout << "Received DeactivateInput " << data->input << std::endl;
                activatedInputs &= ~((uint32_t) data->input);
                switch (data->input) {
                    case MelonDSGameInputTouchScreenX:
                        touchX = 0;
                        break;
                    case MelonDSGameInputTouchScreenY:
                        touchY = 0;
                        break;
                }
                delete data;
            } else if (commandType == &CommandType::ResetInput) {
                std::cout << "Received ResetInput" << std::endl;
                activatedInputs = 0;
                touchX = 0;
                touchY = 0;
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
                const auto data = static_cast<SetSpeedCommandData *>(command.commandData);
                std::cout << "Received SetSpeed " << data->speed << std::endl;
                speedup = data->speed;
                delete data;
            } else if (commandType == &CommandType::Malformed) {
                std::cerr << "Received malformed command!" << std::endl;
            }
        }
    });

    while (!stop) {
        auto start = Time::now();

        auto inputs = activatedInputs.load();
        for (uint8_t i = 0; i < 12; i++) {
            uint8_t key = i > 9 ? i + 6 : i;
            bool isActivated = ((inputs >> i) & 1u) != 0;

            if (isActivated) {
                NDS::PressKey(key);
            } else {
                NDS::ReleaseKey(key);
            }
        }

        if (inputs & MelonDSGameInputTouchScreenX || inputs & MelonDSGameInputTouchScreenY) {
            NDS::TouchScreen(touchX, touchY);
            NDS::PressKey(16 + 6);
        } else {
            NDS::ReleaseScreen();
            NDS::ReleaseKey(16 + 6);
        }

        NDS::RunFrame();

        u32 availableBytes = SPU::GetOutputSize();
        availableBytes = std::min(availableBytes, (u32) (sizeof(audioBuffer) / (2 * sizeof(int16_t))));
        int samplesToWrite = SPU::ReadOutput(audioBuffer, availableBytes);
        audioPipe.writeSizeInBytes(audioBuffer, samplesToWrite * 2);
        audioPipe.writeData(audioBuffer, samplesToWrite * 2);

        screenPipe.writeData(GPU::Framebuffer[GPU::FrontBuffer][0], singleScreenSize);
        screenPipe.writeData(GPU::Framebuffer[GPU::FrontBuffer][1], singleScreenSize);

        auto curEnd = std::chrono::system_clock::now();
        frames++;
        std::chrono::duration<double> elapsedSeconds = curEnd - globalStart;
        auto end = Time::now();
        std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        if ((uint64_t) frames % 30 == 0) {
            auto fps = frames / elapsedSeconds.count();
            std::cout << "FPS: " << (int) fps << std::endl;
        }
    }

    screenPipe.closePipe();
    audioPipe.closePipe();
    inputPipe.closePipe();

#ifdef _WIN32
    WSACleanup();
#endif
}

namespace Platform {

    void StopEmu() {
        // TODO
    }

    FILE *OpenFile(const char *path, const char *mode, bool mustexist) {
        // TODO Intercept this, send to client

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

    size_t FileWrite(void* data, size_t size, size_t count, FILE* file) {
        // TODO Intercept this, send to client
        fwrite(data, size, count, file);
    }

    int FileClose(FILE* file) {
        // TODO Intercept this, send to client
        fclose(file);
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

