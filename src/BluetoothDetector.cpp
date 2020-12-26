#include <iostream>
#include <iomanip>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "BluetoothDetector.hpp"
#include "GDBusProxyFactory.hpp"
#include <chrono>
#include <unistd.h>
#include <vector>
#include <glib.h>
#include <gio/gio.h>
#include <algorithm>
#include <map>
#include <atomic>
#include <memory>
#include <algorithm>
#include <tuple>
#include "rxcpp/rx.hpp" // See http://reactivex.io/RxCpp/
#include "rxcppextras/CombineLatest.hpp"

BluetoothDetector::BluetoothDetector(std::string adapterName)
    : BluetoothDetector::BluetoothDetector(adapterName, -80)
{
}

BluetoothDetector::BluetoothDetector(std::string adapterName, int rssiThreshold)
{
    // Construct AdapterProxy for adapterName (eg hci0)
    _adapterProxy = std::unique_ptr<AdapterProxy>(new AdapterProxy(adapterName));
    _adapterDBusPath = _adapterProxy.get()->GetDBusPath();
    _rssiThreshold = rssiThreshold;
}

rxcpp::observable<bool> BluetoothDetector::ScanBleDBus(std::vector<std::string> addresses)
{
    auto discoveryStart = rxcpp::observable<>::create<bool>([this](rxcpp::subscriber<bool> obs) {
        bool started = _adapterProxy.get()->StartDiscovery();
        if (!started)
        {
            obs.on_error(std::make_exception_ptr(std::runtime_error("Start Discovery failed")));
        }
        else
        {
            obs.on_next(started);
            obs.on_completed();
        }
    });
    auto eventLoop = rxcpp::observe_on_event_loop();
    auto mainStream = discoveryStart
                          // Flatmap to trigger inline delay (on event loop)
                          .flat_map(
                              [eventLoop](bool _) { return rxcpp::observable<>::timer(std::chrono::seconds(11), eventLoop); },
                              [](bool _, long __) { return 0; })
                          // Discovery startup timer expired, get rssi
                          .flat_map(
                              [this, addresses](int _) {
                                  return Run(addresses);
                              },
                              [](int _, bool isActive) { return isActive; });
    // Add a shutdown action to stop discovery
    auto withFinally = mainStream
                           .finally([&]() {
                               _adapterProxy.get()->StopDiscovery();
                           });
    return withFinally;
}

rxcpp::observable<bool> BluetoothDetector::Run(std::vector<std::string> addresses)
{
    return rxcpp::observable<>::create<bool>([this, addresses](rxcpp::subscriber<bool> obs) {
        // Create enclosing GMainLoop to run bus thread
        auto mainLoop = std::make_shared<GMainLoopProxy>();
        // Project each address into an individual polling stream
        auto addressesLength = addresses.size();
        std::vector<rxcpp::observable<bool>> mapped;
        for (auto i : addresses)
        {
            auto stream = PollForAvailability(i);
            mapped.push_back(stream);
        }

        // Main subcription - return true if any values in the Vector are true
        auto proxySubscription = rxcppextras::combine_latest<bool, bool>(
                                     mapped,
                                     [](std::vector<bool> latest) -> bool {
                                         // See if any are "true"
                                         for (int i = 0; i < latest.size(); i++)
                                         {
                                             if (latest[i] == true)
                                             {
                                                 return true;
                                             }
                                         }
                                         return false;
                                     })
                                     .distinct_until_changed()
                                     .subscribe(obs);
        // Start mainloop to receive deltas
        mainLoop->Start();
        // Disposal action
        obs.add([mainLoop, proxySubscription]() {
            mainLoop->Stop();
            proxySubscription.unsubscribe();
        });
    });
}

rxcpp::observable<bool> BluetoothDetector::PollForAvailability(std::string address)
{
    return rxcpp::observable<>::create<bool>([this, address](rxcpp::subscriber<bool> obs) {
        GError *error = nullptr;

        // Create a published delta stream to allow each period to listen to it
        auto publishedDeltaRssiStream = GetRssiDeltaStream(_adapterDBusPath, address)
                                            .publish();

        // Create a long-lived proxy for the Dbus to allow us to re-use for instantaneous RSSI readings
        // Use GDbusProxy to make a single call
        auto fullPath = GetDevicePath(_adapterDBusPath, address);
        std::shared_ptr<GDBusProxy> proxy(
            BuildDbusProxy(fullPath.c_str(), "org.freedesktop.DBus.Properties", error),
            [](GDBusProxy *ptr) {
                g_object_unref(ptr);
            });
        if (!proxy.get())
        {
            auto ex = std::make_exception_ptr("Proxy connection failed " + std::string(error->message));
            g_clear_error(&error);
            obs.on_error(ex);
            return;
        }

        // Tick every 60 seconds to compare against latest Rssi
        auto eventLoop = rxcpp::observe_on_event_loop();
        auto clockStream = rxcpp::observable<>::interval(std::chrono::seconds(60), eventLoop)
                               .map([this, publishedDeltaRssiStream, proxy, address](long _) -> rxcpp::observable<int> {
                                   return GetInstantaneousRssi(proxy, address)
                                       // Merge with delta stream to get intra-period updates
                                       .merge(publishedDeltaRssiStream);
                               });
        auto newPeriodStream = clockStream.switch_on_next()
                                   .map([this](int rssi) -> bool { return rssi > _rssiThreshold; });
        auto proxySubscription = newPeriodStream
                                     .distinct_until_changed()
                                     .subscribe(obs);
        // Connect delta stream to start
        auto connection = publishedDeltaRssiStream.connect();
        // Add to dispoables
        obs.add([proxySubscription, connection, proxy]() {
            proxySubscription.unsubscribe();
            connection.unsubscribe();
        });
    });
}

rxcpp::observable<int> BluetoothDetector::GetRssiDeltaStream(std::string adapterName,
                                                             std::string address)
{
    auto adapterNameLocal = adapterName;
    auto addressLocal = address;
    auto stream = rxcpp::observable<>::create<int>(
        [this, adapterNameLocal, addressLocal](rxcpp::subscriber<int> obs) {
            GError *error = nullptr;
            auto fullPath = GetDevicePath(adapterNameLocal, addressLocal);
            // Get raw bus
            std::shared_ptr<GDBusConnection> rawBusConnection(
                g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error), [](GDBusConnection *ptr) {
                    g_object_unref(ptr);
                });
            if (!rawBusConnection.get())
            {
                auto ex = std::make_exception_ptr("Raw bus connection failed " + std::string(error->message));
                g_clear_error(&error);
                obs.on_error(ex);
                return;
            }
            // Callback for updates
            auto callback = [](GDBusConnection *connection,
                               const gchar *sender_name,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *signal_name,
                               GVariant *parameters,
                               gpointer user_data) {
                // Tuple returned of Interface, dictionary of {changedParameter:variantvalue}
                auto secondTupleItem = GVariantAuto(g_variant_get_child_value(parameters, 1));
                auto inner = secondTupleItem.get();
                // Filter out non-RSSI updates
                auto newRssiValue = GVariantAuto(g_variant_lookup_value(inner, "RSSI", G_VARIANT_TYPE_INT16));
                if (newRssiValue != nullptr)
                {
                    gint16 rawRssiValue = g_variant_get_int16(newRssiValue.get());
                    auto castObs = (std::function<void(gint16)> *)user_data;
                    auto f = *castObs;
                    f(rawRssiValue);
                }
            };
            // Create func to capture obs reference
            auto onNextFunc = std::make_shared<std::function<void(gint16)>>(std::function<void(gint16)>(
                [this, obs, addressLocal](gint16 value) {
                    // Pass new RSSI value to observer
                    obs.on_next(value);
                }));
            auto castCallback = (GDBusSignalCallback)callback;
            // Subscribe to updates
            guint handle = g_dbus_connection_signal_subscribe(
                rawBusConnection.get(),
                "org.bluez",
                "org.freedesktop.DBus.Properties",
                "PropertiesChanged",
                fullPath.c_str(),
                NULL, G_DBUS_SIGNAL_FLAGS_NONE,
                castCallback,
                onNextFunc.get(), NULL);

            // We don't complete as the stream lasts until unsubcribed

            // Disposable actions
            // We need to explictly capture these values to prevent them going out of scope and getting destroyed early
            obs.add([rawBusConnection, handle, onNextFunc]() {
                g_dbus_connection_signal_unsubscribe(rawBusConnection.get(), handle);
            });
        });
    return stream;
}

std::string BluetoothDetector::GetDevicePath(std::string adapterName, std::string address)
{
    std::string transformedAddress = std::string(address);
    // replace : with _
    std::replace(transformedAddress.begin(), transformedAddress.end(), ':', '_');
    std::string fullPath = adapterName + "/dev_" + transformedAddress;
    return fullPath;
}

rxcpp::observable<int> BluetoothDetector::GetInstantaneousRssi(std::shared_ptr<GDBusProxy> proxy,
                                                               std::string addressForLogging)
{
    return rxcpp::observable<>::create<int>([proxy, addressForLogging](rxcpp::subscriber<int> obs) {
        try
        {
            GError *error = nullptr;
            auto underlying = proxy.get();
            auto ret = GVariantAuto(g_dbus_proxy_call_sync(underlying,
                                                           "Get",
                                                           g_variant_new("(ss)",
                                                                         "org.bluez.Device1",
                                                                         "RSSI"),
                                                           G_DBUS_CALL_FLAGS_NONE,
                                                           -1,
                                                           NULL, &error));
            // Returns a 1-ary tuple of a variant of int16 (<int16 -50>,)
            if (!ret.get())
            {
                auto ex = std::make_exception_ptr("Proxy connection failed " + std::string(error->message));
                g_clear_error(&error);
                obs.on_error(ex);
                return;
            }
            auto firstTupleItem = GVariantAuto(g_variant_get_child_value(ret.get(), 0));
            // firstTupleItem is a variant, eg <int16 -50>
            // Get first and only field(?) in variant
            auto innerValue = GVariantAuto(g_variant_get_child_value(firstTupleItem.get(), 0));
            gint16 rssi = g_variant_get_int16(innerValue.get());
            // Pass to observer
            obs.on_next(rssi);
            obs.on_completed();
        }
        catch (const std::exception &e)
        {
            auto ex = std::make_exception_ptr(e);
            obs.on_error(ex);
        }
    });
}
