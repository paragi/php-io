# PHP-IO
is a PHP extension for accessing hardware and peripherals directly through the operating systems driver controls.
PHP is fast becoming an important language in unexpected areas, primary do to the gentle learning curve and versatility. This package makes PHP an option for embedded programming and IoT.

At this point, PHP-IO is only compiled for unix.


Features:
- Direct access to file descriptors
- IOCTL access to driver control functions
- Easy serial port setup

Full documentation is located in the wiki: [Wiki](https://github.com/paragi/PHP-IO/wiki)

## Basics:
- io_open: Open device or file with file descriptor
- io_close: Close file descriptor
- io_read: Read fromk device
- io_write: Write to device

## Serial:
- io_set_serial: get/set subset of serial port settings: speed bit format, hard and soft flow control.

## Controle:
- io_ioctl: Driver specific control function for common interfaces.
- io_ioctl_raw: Driver specific binary control function access. (Not in Safe_mode)

## Install
Binarys are available at https://github.com/paragi/PHP-IO-bin/tree/master

If you want to complie the extension to linux:

Install depandencies.

Find your PHP vsersion:

    php -v
    // PHP 7.2.15.....

Install PHP delevoper package:

    sudo apt-get install php7.2-dev

Compile extension:

    make install

On other systems, you have to compile the files into a PHP library file and place it in the extension directory. That is if the code is compatible, which is not guaranteed.

# Changes
* 0.6.3 File locking added.

* 0.6.2 Libraries merges to one file.
    Added io_ioctl and io_ioctl_raw
    Removed special error handling and applied PHP exceptions to all errors.

* 0.6.1	Published on GitHub.
	Name and function prefix changed to the shorter “io”
	Exceptions thrown on all failures.
	Parameter interpretation strategy loosed towards best guess and reasonable defaults, rather than failing.
	testing is currently only performed on debian linux.


# Help
I really appreciate all kinds of contribution.

Don't hesitate to submit an issue on github. But please provide a reproducible example.

Code should look good and compact, and be covered by a test case or example where possible.
Please don't change the formatting style laid out, without a very good reason.

Things that needs doing is:
- Create a windows compatible version of the library
- Create a better test environment
- Bring code and documentation up to production level.
- Create USB functions
- Create HID functions
