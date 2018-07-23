/*============================================================================*\

  Basic IO header

  provide access to lowlevel IO functions

  (c) By Simon Rigét @ Paragi 2018
  License:

\*============================================================================*/
Php::Value io_open(Php::Parameters &params);
Php::Value io_write(Php::Parameters &params);
Php::Value io_read(Php::Parameters &params);
Php::Value io_close(Php::Parameters &params);
Php::Value io_error(Php::Parameters &params);
Php::Value io_tcgetattr(Php::Parameters &params);
Php::Value io_tcsetattr(Php::Parameters &params);
Php::Value io_ioctl(Php::Parameters &params);
Php::Value io_set_serial(Php::Parameters &params);

Php::Value io_test_flag(Php::Parameters &params);
