#include <iostream>
#include "GMainLoopProxy.hpp"

GMainLoopProxy::GMainLoopProxy()
{
}
GMainLoopProxy::~GMainLoopProxy()
{
    Stop();
}

void GMainLoopProxy::Start()
{
    {
        std::lock_guard<std::mutex> lock(_loopMutex);
        if (_mainLoopRunning)
        {
            return;
        }
        // Create wrapper gmainloop
        _mainLoop = GMainLoopAuto(g_main_loop_new(nullptr, false));
        // Set flag inside of lock statement then start thread
        _mainLoopRunning = true;
    }
    _thread = std::thread([this]() {
        StartThread();
    });
}

void GMainLoopProxy::StartThread()
{
    g_main_loop_run(_mainLoop.get());
}

void GMainLoopProxy::Stop()
{
    bool doShutdown = false;
    {
        std::lock_guard<std::mutex> lock(_loopMutex);
        if (_mainLoopRunning)
        {
            doShutdown = true;
            _mainLoopRunning = false;
        }
    }
    if (doShutdown)
    {
        // Request stop of loop
        g_main_loop_quit(_mainLoop.get());
        if (_thread.joinable())
        {
            _thread.join();
        }
        // unref is called in the delete functor of GMainLoopAuto
    }
}