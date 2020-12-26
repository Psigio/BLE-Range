#pragma once
#include <gio/gio.h>
#include <memory>
#include <glib.h>

static GDBusProxy *BuildDbusProxy(const char *objectPath, const char *interfaceName, GError *lastError)
{
    auto proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        nullptr, "org.bluez", objectPath,
        interfaceName, nullptr, &lastError);
    return proxy;
}