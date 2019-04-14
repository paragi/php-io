/*============================================================================*\

  USB IO header

  provide access to lowlevel IO functions

  (c) By Simon Rigét @ Paragi 2018
  License:

\*============================================================================*/
#include <string>

Php::Value io_usb_list(Php::Parameters &params);
Php::Value io_usb_open(Php::Parameters &params);
Php::Value io_usb_close(Php::Parameters &params);
Php::Value io_usb_parameter_test(Php::Parameters &params);
/*
Php::Value io_usb_open(Php::Parameters &params);
Php::Value io_usb_write(Php::Parameters &params);
Php::Value io_usb_read(Php::Parameters &params);
Php::Value io_usb_close(Php::Parameters &params);
*/
