#pragma once
#include <string>
#include <memory>
#include <functional>
#include <vector>

class IBluetoothDetectorApi
{
};
class BluetoothDetectorApiFactory
{
public:
    std::shared_ptr<IBluetoothDetectorApi> create(std::string bluetoothDevice,
                                                  std::vector<std::string> addresses,
                                                  std::function<void(bool)> onDetectedStateChangedFunc,
                                                  std::function<void(std::exception_ptr)> onErrorFunc,
                                                  std::function<void()> onCompletedFunc);
    std::shared_ptr<IBluetoothDetectorApi> create(std::string bluetoothDevice,
                                                  std::vector<std::string> addresses,
                                                  int rssiThreshold,
                                                  std::function<void(bool)> onDetectedStateChangedFunc,
                                                  std::function<void(std::exception_ptr)> onErrorFunc,
                                                  std::function<void()> onCompletedFunc);
};
