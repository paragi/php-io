/*============================================================================*\

  PHP device package

  Define all functions contained in the extension

  This package provides access to the most common library functions for low-level
  I/O operations of the operating system.
  The purpose is to extend PHP with the functionality needed for embedded
  programming, as an easy to use alternative to C/C++
  This is not suitable for most web applications.

  This is partly a replacement for the now discontinued DIO package by Melanie
  Rhianna Lewis.

  (c) By Simon Rigét @ Paragi 2018
  License: Apache

  0.6.1 01.07.2018
    throwing errors instad of returning false.
    set_serial fixed

\*============================================================================*/
#include "phpcpp.h"
#include "basic.h"
//#include "usb.h"
//#include "hello.h"
//#include "hid.h"


// tell the compiler that the get_module is a pure C function
extern "C" {

    /**
     *  Function that is called by PHP right after the PHP process
     *  has started, and that returns an address of an internal PHP
     *  strucure with all the details and features of your extension
     *
     *  @return void*   a pointer to an address that is understood by PHP
     */
    PHPCPP_EXPORT void *get_module()
    {
        // static(!) Php::Extension object that should stay in memory
        // for the entire duration of the process (that's why it's static)
        static Php::Extension extension("io", "0.6.3");

        // Add public functions

        // Basic IO interface
        extension.add<io_open>("io_open", {
             Php::ByVal("File name",  Php::Type::String)
            ,Php::ByVal("flags", Php::Type::String)
        });

        extension.add<io_write>("io_write", {
             Php::ByVal("file descriptor",  Php::Type::Numeric)
            ,Php::ByVal("buffer", Php::Type::String)
        });

        extension.add<io_read>("io_read", {
             Php::ByVal("file descriptor",  Php::Type::Numeric)
            ,Php::ByVal("length", Php::Type::Numeric)
        });

        extension.add<io_close>("io_close", {
             Php::ByVal("file descriptor",  Php::Type::Numeric)
        });

        extension.add<io_ioctl>("io_ioctl", {
             Php::ByVal("file descriptor",  Php::Type::Numeric)
            ,Php::ByVal("command", Php::Type::Numeric)
        });

        extension.add<io_ioctl>("io_ioctl_raw", {
              Php::ByVal("file descriptor",  Php::Type::Numeric)
             ,Php::ByVal("command", Php::Type::Numeric)
        });

        extension.add<io_set_serial>("io_set_serial", {
             Php::ByVal("Path",  Php::Type::String)
            ,Php::ByVal("Settings", Php::Type::String)
        });

        extension.add<io_test_flag>("io_test_flag", {
             Php::ByVal("flags",  Php::Type::String)
        });
/*
        // Hid interface
        extension.add<io_hid_enumerate>("io_hid_enumerate", {
             Php::ByVal("vendor_ID",  Php::Type::Numeric)
            ,Php::ByVal("product_ID", Php::Type::Numeric)
        });

        extension.add<io_hid_open>("io_hid_open", {
             Php::ByVal("vendor_ID",  Php::Type::Numeric)
            ,Php::ByVal("product_ID", Php::Type::Numeric)
        });

        extension.add<io_hid_open_path>("io_hid_open_path", {
            Php::ByVal("Path", Php::Type::String)
        });

        extension.add<io_hid_write>("io_hid_write", {
             Php::ByVal("file descriptor",  Php::Type::Numeric)
            ,Php::ByVal("buffer", Php::Type::String)
        });

        extension.add<io_hid_read>("io_hid_read", {
             Php::ByVal("file descriptor",  Php::Type::Numeric)
            ,Php::ByVal("timeout", Php::Type::Numeric)
        });

        extension.add<io_hid_send_feature_report>("io_hid_send_feature_report", {
             Php::ByVal("file descriptor",  Php::Type::Numeric)
            ,Php::ByVal("buffer", Php::Type::String)
        });

        extension.add<io_hid_get_feature_report>("io_hid_get_feature_report", {
             Php::ByVal("file descriptor",  Php::Type::Numeric)
            ,Php::ByRef("buffer", Php::Type::String)
        });

        extension.add<io_hid_close>("io_hid_close", {
             Php::ByVal("file descriptor",  Php::Type::Numeric)
        });

        extension.add<io_hid_set_nonblocking>("io_hid_set_nonblocking", {
             Php::ByVal("file descriptor",  Php::Type::Numeric)
            ,Php::ByVal("timeout", Php::Type::Numeric)
        });


        // USB
        extension.add<io_usb_list>("io_usb_list", {});
        */

        // return the extension
        return extension;
    }
}
