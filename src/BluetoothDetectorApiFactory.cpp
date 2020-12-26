#include <memory>
#include "BluetoothDetectorApi.hpp"
#include "PrivateBluetoothDetectorApi.hpp"

std::shared_ptr<IBluetoothDetectorApi>
BluetoothDetectorApiFactory::create(std::string bluetoothDevice,
                                    std::vector<std::string> addresses,
                                    std::function<void(bool)> onDetectedStateChangedFunc,
                                    std::function<void(std::exception_ptr)> onErrorFunc,
                                    std::function<void()> onCompletedFunc)
{
    return std::make_shared<PrivateBluetoothDetectorApi>(bluetoothDevice,
                                                         addresses,
                                                         onDetectedStateChangedFunc,
                                                         onErrorFunc,
                                                         onCompletedFunc);
}

std::shared_ptr<IBluetoothDetectorApi>
BluetoothDetectorApiFactory::create(std::string bluetoothDevice,
                                    std::vector<std::string> addresses,
                                    int rssiThreshold,
                                    std::function<void(bool)> onDetectedStateChangedFunc,
                                    std::function<void(std::exception_ptr)> onErrorFunc,
                                    std::function<void()> onCompletedFunc)
{
    return std::make_shared<PrivateBluetoothDetectorApi>(bluetoothDevice,
                                                         addresses,
                                                         rssiThreshold,
                                                         onDetectedStateChangedFunc,
                                                         onErrorFunc,
                                                         onCompletedFunc);
}