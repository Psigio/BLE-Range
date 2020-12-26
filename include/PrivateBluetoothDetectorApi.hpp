#pragma once
#include <string>
#include <memory>
#include "BluetoothDetectorApi.hpp"
#include "BluetoothDetector.hpp"
#include "rxcpp/rx.hpp"

class PrivateBluetoothDetectorApi : public IBluetoothDetectorApi
{
public:
    PrivateBluetoothDetectorApi(std::string bluetoothDevice,
                                std::vector<std::string> addresses,
                                std::function<void(bool)> onDetectedStateChangedFunc,
                                std::function<void(std::exception_ptr)> onErrorFunc,
                                std::function<void()> onCompletedFunc);
    PrivateBluetoothDetectorApi(std::string bluetoothDevice,
                                std::vector<std::string> addresses,
                                int rssiThreshold,
                                std::function<void(bool)> onDetectedStateChangedFunc,
                                std::function<void(std::exception_ptr)> onErrorFunc,
                                std::function<void()> onCompletedFunc);
    ~PrivateBluetoothDetectorApi();

private:
    void Initialise(std::vector<std::string> addresses,
                    std::function<void(bool)> onDetectedStateChangedFunc,
                    std::function<void(std::exception_ptr)> onErrorFunc,
                    std::function<void()> onCompletedFunc);
    std::shared_ptr<BluetoothDetector> _instance;
    rxcpp::composite_subscription _subscription;
};