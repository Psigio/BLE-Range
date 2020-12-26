#pragma once
#include <thread>
#include <mutex>
#include "GDBusProxyAuto.hpp"

class GMainLoopProxy
{
public:
    GMainLoopProxy();
    ~GMainLoopProxy();

    void Start();
    void Stop();

private:
    void StartThread();

    GMainLoopAuto _mainLoop;
    std::thread _thread;
    std::mutex _loopMutex;
    bool _mainLoopRunning = false;
};