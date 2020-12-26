#pragma once
#include <gio/gio.h>
#include <string>
#include <memory>
#include <iostream>
#include <functional>
#include <glib.h>

struct deleteGVariantFunctor
{
    void operator()(GVariant *ptr)
    {
        if (ptr)
        {
            g_variant_unref(ptr);
        }
    }
};
typedef std::unique_ptr<GVariant, deleteGVariantFunctor> GVariantAuto;

struct deleteGMainLoopFunctor
{
    void operator()(GMainLoop *ptr)
    {
        if (ptr)
        {
            g_main_loop_unref(ptr);
        }
    }
};
typedef std::unique_ptr<GMainLoop, deleteGMainLoopFunctor> GMainLoopAuto;
