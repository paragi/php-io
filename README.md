# PHP-IO
PHP extension for direct access to operating system I/O drivers 

Features:
- Operate on file descriptors
- IOCTL access to driver control functions
- Easy serial port setup

PHP is fast becoming an important language in unexpected areas, primary do to the gentle learning curve and versatility. However it lacks access to lower level I/O functions, to be fully useful with embedded  programming and IOT. This package is an attempt to remedy that.

#Install
Currently only compiles under unix.

Debian/Ubuntu linux:

    sudo make install


#Functions:

##Basics:
- io_open: Open device or file with file descriptor
- io_close: Close file descriptor
- io_read: Read fromk device
- io_write: Write to device

## Serial:
- io_set_serial: get/set subset of serial port settings: speed bit format, hard and soft flow control.

## Generel:
- io_ioctl: Driver specific control function for common interfaces.
- io_ioctl_raw: Driver specific binary control function access. (Not in Safe_mode)


# Changes
* 0.7.2		Published on GitHub
		Name and function prefix changed to the shorter “io”
		Exceptions thrown on all failures
		Parameter interpretation strategy loosed towards best guess and reasonable defaults, rather than failing.
		

# Help
I appreciate contributions. Code should look good and compact, and be covered by a test case or example.
Please don't change the formatting style laid out, without a good reason. I know its not the most common standard, but its a rather efficient one with node.

Don't hesitate to submit an issue on github. But please provide a reproducible example.

Help is needed to:
- Create a windows compatible version of the library
- Create a better test environment
- Bring code and documentation up to version 1.0 level.
- Create USB functions
- Create HID functions


