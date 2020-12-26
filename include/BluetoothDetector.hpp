#pragma once
#include <glib.h>
#include <gio/gio.h>
#include <string>
#include <map>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include "AdapterProxy.hpp"
#include "GMainLoopProxy.hpp"
#include "GDBusProxyAuto.hpp"
#include "rxcpp/rx.hpp"

class BluetoothDetector
{

public:
    BluetoothDetector(std::string adapterName);
    BluetoothDetector(std::string adapterName, int rssiThreshold);
    rxcpp::observable<bool> ScanBleDBus(std::vector<std::string> addresses);

private:
    rxcpp::observable<bool> Run(std::vector<std::string> addresses);
    rxcpp::observable<int> GetRssiDeltaStream(std::string adapterName,
                                              std::string address);
    std::string GetDevicePath(std::string adapterName, std::string address);
    rxcpp::observable<int> GetInstantaneousRssi(std::shared_ptr<GDBusProxy> proxy, std::string addressForLogging);
    rxcpp::observable<bool> PollForAvailability(std::string address);

    std::string _adapterDBusPath;
    std::unique_ptr<AdapterProxy> _adapterProxy = nullptr;
    int _rssiThreshold;
};
