#pragma once
#include <iostream>
#include <memory>
#include <gio/gio.h>
class AdapterProxy
{
public:
    AdapterProxy(std::string adapterName);
    bool StartDiscovery();
    void StopDiscovery();
    std::string GetDBusPath();

private:
    std::shared_ptr<GDBusProxy> _busProxy;
    std::string _dBusPath;
};