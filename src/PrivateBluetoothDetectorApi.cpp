#include <string>
#include <memory>
#include "BluetoothDetector.hpp"
#include "PrivateBluetoothDetectorApi.hpp"

PrivateBluetoothDetectorApi::PrivateBluetoothDetectorApi(std::string bluetoothDevice,
                                                         std::vector<std::string> addresses,
                                                         std::function<void(bool)> onDetectedStateChangedFunc,
                                                         std::function<void(std::exception_ptr)> onErrorFunc,
                                                         std::function<void()> onCompletedFunc)
{
    _instance = std::move(std::make_shared<BluetoothDetector>(bluetoothDevice));
    Initialise(addresses, onDetectedStateChangedFunc, onErrorFunc, onCompletedFunc);
}
PrivateBluetoothDetectorApi::PrivateBluetoothDetectorApi(std::string bluetoothDevice,
                                                         std::vector<std::string> addresses,
                                                         int rssiThreshold,
                                                         std::function<void(bool)> onDetectedStateChangedFunc,
                                                         std::function<void(std::exception_ptr)> onErrorFunc,
                                                         std::function<void()> onCompletedFunc)
{
    _instance = std::move(std::make_shared<BluetoothDetector>(bluetoothDevice, rssiThreshold));
    Initialise(addresses, onDetectedStateChangedFunc, onErrorFunc, onCompletedFunc);
}
void PrivateBluetoothDetectorApi::Initialise(std::vector<std::string> addresses,
                                             std::function<void(bool)> onDetectedStateChangedFunc,
                                             std::function<void(std::exception_ptr)> onErrorFunc,
                                             std::function<void()> onCompletedFunc)
{
    auto eventLoop = rxcpp::observe_on_event_loop();
    _subscription =
        _instance.get()->ScanBleDBus(addresses)
            // Move observation off on to event loop to free up gmainloop
            .observe_on(eventLoop)
            .subscribe(
                [onDetectedStateChangedFunc](bool isAvailable) {
                    onDetectedStateChangedFunc(isAvailable);
                },
                [onErrorFunc](std::exception_ptr ep) {
                    onErrorFunc(ep);
                },
                [onCompletedFunc]() {
                    onCompletedFunc();
                });
}

PrivateBluetoothDetectorApi::~PrivateBluetoothDetectorApi()
{
    _subscription.unsubscribe();
}