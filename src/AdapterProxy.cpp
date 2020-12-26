#include "AdapterProxy.hpp"
#include "GDBusProxyFactory.hpp"
#include "GDBusProxyAuto.hpp"
#include <iostream>
#include <memory>

AdapterProxy::AdapterProxy(std::string adapterName)
{
    std::string rootPath = "/org/bluez/";
    _dBusPath = rootPath + adapterName;
    // Create bus proxy
    GError *lastError = nullptr;
    std::shared_ptr<GDBusProxy> proxy(BuildDbusProxy(_dBusPath.c_str(),
                                                     "org.bluez.Adapter1", lastError),
                                      [](GDBusProxy *ptr) {
                                          g_object_unref(ptr);
                                      });
    _busProxy = std::move(proxy);
    if (!_busProxy.get())
    {
        g_clear_error(&lastError);
    }
}

bool AdapterProxy::StartDiscovery()
{
    GError *lastError = nullptr;
    auto ret = GVariantAuto(g_dbus_proxy_call_sync(_busProxy.get(), "StartDiscovery",
                                                   nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &lastError));
    if (!ret.get())
    {
        g_clear_error(&lastError);
        return false;
    }
    else
    {
        return true;
    }
}

void AdapterProxy::StopDiscovery()
{
    GError *lastError = nullptr;
    auto ret = GVariantAuto(g_dbus_proxy_call_sync(_busProxy.get(), "StopDiscovery",
                                                   nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &lastError));
    if (!ret.get())
    {
        g_clear_error(&lastError);
    }
}

std::string AdapterProxy::GetDBusPath()
{
    return _dBusPath;
}