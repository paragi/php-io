/*============================================================================*\
  Basic IO functions

  (c) By Simon Rigét @ Paragi 2018
  License: Apache

  This is a simplified interface to libusb. Some acpects of operation, is hidden from the PHP programmer, in order to reduce the required knowledge, for accessing straight forward USB devices. However that might be a disatvantage in some special cases.
  I hope to remedy that, as det package matures.

  List: returns an array of devices (if specified, Matching description)
  Each device has one or more endponints, represented in an array.
  the array keys to a device and each of its endpoints are indentified by a string:

  device ID:
    <vendor ID>, <Product ID>, <Serial number>[,<Physical bus address>]
  (If the device has a serial number, the bus address is not used)

  endpoint ID:
    <configuration> - <alternative> - <interface> - <endpoint>

  The programmer can just use the array keys to open the device ans access the endpoints.



  1. initialize the library by calling the function libusb_init and creating a session
  2. Call the function libusb_get_device_list to get a list of connected devices. This creates an array of libusb_device containing all usb devices connected to the system.
  3. Loop through all these devices and check their options
  4. Discover the one and open the device either by libusb_open or libusb_open_device_with_vid_pid(when you know vendor and product id of the device) to open the device
  5. Clear the list you got from libusb_get_device_list by using libusb_free_device_list
  6. Claim the interface with libusb_claim_interface (requires you to know the interface numbers of device)
  7. Do desired I/O
  8. Release the device by using libusb_release_interface
  9. Close the device you openedbefore, by using libusb_close
  10. Close the session by using libusb_exit



\*============================================================================*/
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <locale>
#include <map>
#include <algorithm>
#include <math.h>

#include "phpcpp.h"

#include "usb.h"
#include "common.h"
#include "usb_class_names.h"

#include <libusb-1.0/libusb.h>

#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <linux/limits.h>
#include <boost/algorithm/string.hpp>

using namespace std;

// globals
typedef struct {
  libusb_device_handle * libusb_handle;
  bool detach;
  uint8_t configuration;
  uint8_t altSetting;
  uint8_t claimed_interface;
} Handle;

typedef struct {
  int configuration;
  int interface;
  int alternate;
  int endpoint;
  int maxPacketSize;
  string direction;
  string type;
  string sync;
} Short_descriptor;

typedef struct {
  int vendorID;
  int productID;
  std::string serial;
  int bus;
  int address;
} DeviceID;

typedef struct {
  int configuration;
  int altSetting;
  int interface;
  int endpoint;
} EndpointID;

class Usb {
  public:
  libusb_context *ctx;
  vector<Handle> handle_list;

  Usb(){
    ctx = NULL;
  	int rc = libusb_init(&ctx); //initialize a library session
  	if(rc < 0)
      throw Php::Exception("io_usb unable to initialize libusb context. Return code(" + to_string( rc ) + ")"
      );
  	libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation
  }

  ~Usb(){
    for ( auto &e : handle_list )
      libusb_close(e.libusb_handle);
  	libusb_exit(ctx); //close the session
  }

  int add_handle(Handle handle){
    handle_list.push_back( handle );
    return handle_list.size() - 1;
  }

  void remove_handle(int handle_index){
    handle_list.erase( handle_list.begin() + handle_index );
  }
};

Usb * usb = new Usb();

/*============================================================================*\
  get_descriptor_string

  Read at string at index, on an open device.

\*============================================================================*/
string get_descriptor_string(
   libusb_device_handle * dev_handle
  ,uint8_t string_index
){

  int rc;
  unsigned char cstr128[128] ="";

  if( string_index > 0){
    rc = libusb_get_string_descriptor_ascii(
       dev_handle
      ,string_index
      ,cstr128
      ,sizeof(cstr128)
    );

  	if (rc < 0)
      throw Php::Exception(string ("io_usb unable to read device string at index " + to_string(string_index) + ". code (") + to_string(rc) + string(")"));
  }

  return string((char * )cstr128);
}

/*============================================================================*\
  Interface descriptor

  The interface descriptor could be seen as a header or grouping of the endpoints into a functional group performing a single feature of the device.

\*============================================================================*/
class Interface {
  private:
	string epType[4] = {
		"Control",
		"Isochronous",
		"Bulk",
		"Interrupt"
	};
	string epSync[4] = {
		"None",
		"Asynchronous",
		"Adaptive",
		"Synchronous"
	};
	string usage[4] = {
		"Data",
		"Feedback",
		"Implicit feedback Data",
		"(reserved)"
	};

  public:
  Php::Value descriptor;
  vector<Short_descriptor> short_descriptor;


  Interface(
     libusb_device_handle * dev_handle
    ,int number_of_interfaces
    ,const struct libusb_interface * interface_header
  ){
    const struct libusb_interface_descriptor * interface_descriptor;
    USBClassNames usb_class_string;
    int inum, anum;
    bool iset;
    descriptor = Php::Array();

  	for (int i = 0; i < number_of_interfaces ; i++) {
      iset = false;

      for (int a = 0; a < interface_header[i].num_altsetting; a++) {
        interface_descriptor = &interface_header[i].altsetting[a];
        inum = interface_descriptor->bInterfaceNumber;
        anum = interface_descriptor->bAlternateSetting;
        if( !iset ) {
          descriptor[inum] = Php::Array();
          iset = true;
        }
        descriptor[inum][anum] = Php::Array();

        descriptor[inum][anum]["class"] = usb_class_string.get_class_text(
           interface_descriptor->bInterfaceClass
          ,interface_descriptor->bInterfaceSubClass
          ,interface_descriptor->bInterfaceProtocol
          ,true )
        ;

        descriptor[inum][anum]["name"] = get_descriptor_string(dev_handle, interface_descriptor->iInterface);

        descriptor[inum][anum]["bNumEndpoints"] = interface_descriptor->bNumEndpoints;

        descriptor[inum][anum]["bInterfaceNumber"] = interface_descriptor->bInterfaceNumber;
        descriptor[inum][anum]["bAlternateSetting"] = interface_descriptor->bAlternateSetting;
        descriptor[inum][anum]["bNumEndpoints"] = interface_descriptor->bNumEndpoints;

        descriptor[inum][anum]["endpoint"] = Php::Array();

      	for (int e = 0 ; e < interface_descriptor->bNumEndpoints ; e++){

      		const libusb_endpoint_descriptor * endpoint =
            &interface_descriptor->endpoint[e];
          int ep = endpoint->bEndpointAddress;
          descriptor[inum][anum]["endpoint"][ep] = Php::Array();
          descriptor[inum][anum]["endpoint"][ep]["bEndpointAddress"] =
            endpoint->bEndpointAddress;

          descriptor[inum][anum]["endpoint"][ep]["direction"] =
            (endpoint->bEndpointAddress & 0x80) ? "IN" : "OUT";

          descriptor[inum][anum]["endpoint"][ep]["type"] =
            epType[endpoint->bmAttributes & 3];

          descriptor[inum][anum]["endpoint"][ep]["sync"] =
            epSync[(endpoint->bmAttributes >> 2) & 3];

          descriptor[inum][anum]["endpoint"][ep]["usages"] =
            usage[(endpoint->bmAttributes >> 4) & 3];

          descriptor[inum][anum]["endpoint"][ep]["wMaxPacketSize"] =
            endpoint->wMaxPacketSize;

          descriptor[inum][anum]["endpoint"][ep]["bRefresh"] =
            endpoint->bRefresh;

          descriptor[inum][anum]["endpoint"][ep]["bSynchAddress"] =
            endpoint->bSynchAddress;

          // configuration - interface - alternate setting - endpoint

          short_descriptor.push_back({
              (int) -1
            , (int) interface_descriptor->bInterfaceNumber
            , (int) interface_descriptor->bAlternateSetting
            , (int) endpoint->bEndpointAddress
            , (int) endpoint->wMaxPacketSize
            , string((endpoint->bEndpointAddress & 0x80) ? "in" : "out")
            , string(epType[endpoint->bmAttributes & 3])
            , string(epSync[(endpoint->bmAttributes >> 2) & 3])
          });
        }
      }
    }
  }
};



/*============================================================================*\
  Device configuration

   A USB device can have several different configurations although the majority of devices are simple and only have one. The configuration descriptor specifies how the device is powered, what the maximum power consumption is, the number of interfaces it has. Therefore it is possible to have two configurations, one for when the device is bus powered and another when it is mains powered. As this is a "header" to the Interface descriptors, its also feasible to have one configuration using a different transfer mode to that of another configuration.

  Once all the configurations have been examined by the host, the host will send a SetConfiguration command with a non zero value which matches the bConfigurationValue of one of the configurations. This is used to select the desired configuration.
\*============================================================================*/
class Device_configuration {
  public:
  Php::Value descriptor;
  vector<Short_descriptor> short_descriptor;

  Device_configuration(
     libusb_device * device
    ,libusb_device_handle * dev_handle
    ,int number_of_configurations
    ,int bcdUSB
  ){

    struct libusb_config_descriptor * config_descriptor;
    int rc;

    for( int c = 0; c < number_of_configurations ; ++c ){

      rc = libusb_get_config_descriptor(device, c, &config_descriptor);
  		if(rc)
        descriptor[c] = "Failed to retrieve configuration";

      else {
        auto i = config_descriptor->bConfigurationValue;
        descriptor[i] = Php::Array();
        descriptor[i]["configuration"] = get_descriptor_string(dev_handle,config_descriptor->iConfiguration);
        descriptor[i]["powerSource"] =
          ( config_descriptor->bmAttributes & 0x40 ?
            "Self powered" : ( config_descriptor->bmAttributes & 0x10 ?
            "Battery Powered" : "Bus powered" ));

        descriptor[i]["remoteWakeup"] =
          ( config_descriptor->bmAttributes & 0x20 ? "yes" : "no" );
        descriptor[i]["MaxPower"] = to_string(config_descriptor->MaxPower * 2) + "mA";

        auto interface = new Interface(
           dev_handle
          ,config_descriptor->bNumInterfaces
          ,config_descriptor->interface
        );

        descriptor[i]["interface"] = interface->descriptor;

        short_descriptor = interface->short_descriptor;

        for ( auto &s : short_descriptor )
          s.configuration = i;
      }

  		libusb_free_config_descriptor(config_descriptor);


    }
  }
};


/*============================================================================*\
  Device classes

  Describe a device (root class)

  The device descriptor of a USB device represents the entire device. As a result a USB device can only have one device descriptor. It specifies some basic, yet important information about the device such as the supported USB version, maximum packet size, vendor and product IDs and the number of possible configurations the device can have.

\*============================================================================*/
class Device {
  private:
  	libusb_device_descriptor descriptor_pointer;

  string bcd_to_version(int bcd){
    stringstream stream;
    string str("");

    if(bcd > 0){
      stream << hex << bcd;
      str = stream.str();

      if( str.length() < 3 )
        str.insert(0, 3 - str.length(), '0');

      str.insert(str.end()-2, 1, '.');
    }

    return str;
  }

  public:
  Php::Value descriptor;

  Device(libusb_device * device){
    uint8_t path[8];
    stringstream stream;
    string str;
    libusb_device_handle * dev_handle;
    int rc;
    string error("");
    USBClassNames usb_class_string;

  	rc = libusb_get_device_descriptor(device, &descriptor_pointer);
  	if (rc < 0)
      throw Php::Exception("io_usb unable to read device descriptor");

    uint8_t bus_number = libusb_get_bus_number(device);
	  uint8_t device_address = libusb_get_device_address(device);
    uint8_t port_number = libusb_get_port_number(device);


    rc = libusb_open(device,&dev_handle);
  	if ( rc )
      throw Php::Exception(
          string("Libusb error: ")
        + string (libusb_strerror( static_cast<libusb_error>(rc) ) )
        + ". code (" + to_string( rc ) + string(")")
      );

    descriptor["vendorID"]          = descriptor_pointer.idVendor;
    descriptor["productID"]         = descriptor_pointer.idProduct;

    descriptor["serial"] = get_descriptor_string(dev_handle, descriptor_pointer.iSerialNumber);


    descriptor["bus"]             = bus_number;
    descriptor["device"]          = device_address;
    descriptor["port"]            = port_number;

    descriptor["manufacturer"] = get_descriptor_string(
       dev_handle
      ,descriptor_pointer.iManufacturer
    );

    descriptor["product"] = get_descriptor_string(
       dev_handle
      ,descriptor_pointer.iProduct
    );

    descriptor["USBVersion"] = bcd_to_version(descriptor_pointer.bcdUSB);

    descriptor["deviceVersion"] = bcd_to_version(descriptor_pointer.bcdDevice);

    descriptor["class"] = usb_class_string.get_class_text(
       descriptor_pointer.bDeviceClass
      ,descriptor_pointer.bDeviceSubClass
      ,descriptor_pointer.bDeviceProtocol
      ,false )
    ;

    //descriptor["maxPacketSize"]   = descriptor_pointer.bMaxPacketSize0;

    auto r = libusb_get_port_numbers(device, path, sizeof(path));
		if (r > 0) {
      string path_str = "";
			for (auto j = 0; j < r; j++)
        path_str += "." + to_string(path[j]);
      descriptor["path"] = path_str.substr(1);

    }

    auto configuration = new Device_configuration(
       device
      ,dev_handle
      ,descriptor_pointer.bNumConfigurations
      ,descriptor_pointer.bcdUSB
    );

    descriptor["configuration"] = configuration->descriptor;
    descriptor["endpoint"] = Php::Array();

    for ( auto &s : configuration->short_descriptor ){
      string ep =
          to_string(s.configuration)
        + "-" + to_string(s.interface)
        + "-" + to_string(s.alternate)
        + "-" + to_string(s.endpoint)
      ;
      descriptor["endpoint"][ep] = Php::Array();
      descriptor["endpoint"][ep]["direction"] = s.direction;
      descriptor["endpoint"][ep]["type"] = s.type;
      descriptor["endpoint"][ep]["sync"] = s.sync;
      descriptor["endpoint"][ep]["maxPacketSize"] = s.maxPacketSize;
    }

    libusb_close(dev_handle);
  }
};

/*============================================================================*\
  Device list

  Construct an array of attached devices and there properties.
  the index of the array is ideally an unique identifier for the device, in
  the form:

    <vendor ID>, <Product ID>, <Serial number>[,<Physical bus address>]

  That way each attached device can be identified, even it if it is connected
  to another port.
  Unfortunately many many vendors choose to save the few cents, and skip the
  serial number.
  Therefore the only way to distinguish two identical devices, is to use the
  physical port address.
  The consequence is that if identical devices without serial number are moved,
  the identified changes.

  The index key is either
	  <vendor ID>, <Product ID>, <Serial number>
  Or
	  <vendor ID>, <Product ID>,,<Physical bus address>

\*============================================================================*/

class Device_list {
  private:
   	libusb_device **devices;
    libusb_context *ctx = NULL; //a libusb session
    ssize_t length;

  public:
    Php::Value list;

    Device_list(libusb_context *ctx) {
	    length = libusb_get_device_list(ctx, &devices);
	    if(length < 0)
        throw Php::Exception("io_usb unable to get a device list.");

	    for(auto i = 0; i < length; i++) {
        auto device = new Device(devices[i]);
        int vendor = device->descriptor["vendor"];
        int product = device->descriptor["product"];
        string serial = device->descriptor["serial"];
        int bus = device->descriptor["bus"];
        int device_address = device->descriptor["device"];
        string device_class = device->descriptor["deviceClass"];

        string address =
                  to_string(vendor)
          + "," + to_string(product)
          + "," + serial
          + "," + to_string(bus)
          + "-" + to_string(device_address);

        if(device_class != string("Hub") )
          list[address] = device->descriptor;
		  }
    }

    ~Device_list(void) {
      libusb_free_device_list(devices, 1);
    }
};


/*============================================================================*\
  USB List devices

  array $usb_device_list = io_usb_list([Vendor ID [, Product ID]])

  Return: An array of attached devices and their properties

  array io_usb_list(string composit_key ,[bool show_all = false])
  array io_usb_list([int vendor ID [, int product ID [, string serial [, string bud_address [, bool show_all = false]]]]] )

  optional values can be omitted is set to NULL.

\*============================================================================*/
Php::Value io_usb_list(Php::Parameters &params){
  //Php::Value ret = -1;

	libusb_context *ctx = NULL; //a libusb session

  if( params.size() > 0 ){
    string vendor_id = params[0];
    if( vendor_id.length() > 0
        && vendor_id.find_first_not_of( "0123456789ABCDEFabcdef" ) != std::string::npos){

      // Here goes split the string stuff
      // vendor ID , product ID ,
      // serial can be anything!
      // last ; bus address (bus-port)
    }
    else if( params.size() > 1 ){
      string product_id = params[1];
      if( params.size() > 2 ){
        string serial = params[2];
        if( params.size() > 3 ){
          string bus_address = params[3];
        }
      }
    }
  }

  //if( params.size() > 0 ){

	int rc = libusb_init(&ctx); //initialize a library session
	if(rc < 0)
    throw Php::Exception("io_usb unable to initialize libusb context. Return code(" + to_string( rc ) + ")"
    );


	libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation

  auto usb_device = new Device_list(ctx);
  Php::Value ret = usb_device->list;

	libusb_exit(ctx); //close the session

  return ret;
}

/*============================================================================*\
  Parameter test
\*============================================================================*/
vector<string> split_str(string str, char delimiter){
  vector<string> words;
  for( size_t begin = 0, end = 0; end != string::npos; ){
    end = str.find_first_of(delimiter,  begin);
    words.push_back(str.substr(begin, end-begin));
    begin = end + 1;
  }
  return words;
}

DeviceID interpret_device_str(string str){
  vector<string> words;
  vector<string> adr;
  DeviceID target = { -1, -1, "", -1, -1 };
  int i = 0;

  words = split_str( str, ',' );
  if( words.size() > 3 && words[3].length() > 0 ){
    adr = split_str( words[3],'-');
    if( adr.size() > 0 ) words.push_back( adr[0] );
    if( adr.size() > 1 ) words.push_back( adr[1] );
    words.erase( words.begin() + 3 );
  }

  for(auto it = words.begin(); it != words.end(); it++, i++)
    try {
      switch( i ){
        case 0: target.vendorID  = stoi(words[i]); break;
        case 1: target.productID = stoi(words[i]); break;
        case 2: target.serial    = words[i]; break;
        case 3: target.bus       = stoi(words[i]); break;
        case 4: target.address   = stoi(words[i]); break;
      }
    } catch(...) {}

  return target;
}

DeviceID interpret_endpoint_str(string str){
  vector<string> words;
  EndpointID target = { -1, -1, -1, -1 };
  int i = 0;

  words = split_str( str, '-' );
  for(auto it = words.begin(); it != words.end(); it++, i++)
    try {
      switch( i ){
        case 0: target.configuration = stoi(words[i]) & 0xff; break;
        case 1: target.altSetting    = stoi(words[i]) & 0xff; break;
        case 2: target.interface     = stoi(words[i]) & 0xff; break;
        case 3: target.endpoint      = stoi(words[i]) & 0xff; break;
      }
    } catch(...) {}

  return target;
}


Php::Value io_usb_parameter_test(Php::Parameters &params){
  DeviceID target;
  ostringstream sstream;

  if( params.size() < 1 )
    throw Php::Exception("io_usb_open Please specify parameter to test");

  string str = params[0];
  target = interpret_device_str( str );

  sstream << target.vendorID;
  sstream << ',' << target.productID;
  sstream << ',' << target.serial;
  sstream << ',' << target.bus;
  sstream << ',' << target.address << endl;

  return sstream.str();
}


/*============================================================================*\
  USB open devices

  parameters:

    device: the device is identified by a comma separated string of:
      <vendor ID >[, <product ID>[, <serial number>[, <buss address> - <device addess> ]]]

    lock: boolean

  return: usb_device_index on success or <1 if device was not pressent.

  Throwing error on all other failures.

    0 on success
    LIBUSB_ERROR_NOT_FOUND if the requested interface does not exist
    LIBUSB_ERROR_BUSY if another program or driver has claimed the interface
    LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
    a LIBUSB_ERROR code on other failure

\*============================================================================*/
Php::Value io_usb_open(Php::Parameters &params){
  Php::Value ret = -1;
  libusb_device **device_list;
  libusb_device_handle * dev_handle;
  libusb_device_descriptor descriptor;
  int rc;
  bool exclusive = true;
  DeviceID target;
  Handle handle = {NULL, false};

  if( params.size() < 1 )
    throw Php::Exception("io_usb_open Please specify wich device to open");

  string str = params[0];
  target = interpret_device_str( str );

  if( params.size() > 1 )
    exclusive = params[1];

  cout<< "Vendor: "<< target.vendorID << endl;
  cout<< "product:" << target.productID << endl;
  cout<< "serial: "<< target.serial << endl;
  cout<< "Bus: "<<  target.bus << endl;
  cout<< "Address: " << target.address << endl;

  cout << "lock:" << (exclusive ? "true" : "false") << endl;

  auto length = libusb_get_device_list(usb->ctx, &device_list);
  for(auto i = 0; i < length; i++) {
  	rc = libusb_get_device_descriptor(device_list[i], &descriptor);
    if (rc < 0)
      continue;

    if( target.vendorID >= 0 && target.vendorID != descriptor.idVendor )
      continue;

    if( target.productID >= 0 && target.productID != descriptor.idProduct )
      continue;

    if( target.serial.length() <= 0 ){
      if( target.bus >= 0 && target.bus != libusb_get_bus_number(device_list[i]) )
        continue;

      if( target.address >= 0 && target.address !=  libusb_get_device_address(device_list[i]) )
        continue;
    }

    rc = libusb_open(device_list[i],&dev_handle);
  	if ( rc )
      continue;

    if( target.serial.length() > 0 && target.serial != get_descriptor_string(dev_handle, descriptor.iSerialNumber) ){

      libusb_close(dev_handle);
      continue;
    }

    handle.libusb_handle = dev_handle;
    handle.detach = exclusive;
    ret = usb->add_handle(handle);

  }

  libusb_free_device_list(device_list, 1);
  return ret;
}


// http://libusb.sourceforge.net/api-1.0/group__libusb__asyncio.html
// https://codereview.stackexchange.com/questions/156701/wrapping-libusb-library-in-c
// HID example: https://github.com/signal11/hidapi/blob/master/hidtest/hidtest.cpp

/*============================================================================*\
  USB close devices

  parameters:

    device: the device is identified by a comma separated string of:
      <vendor ID >[, <product ID>[, <serial number>[, <buss address> - <device addess> ]]]

    lock: boolean

  return: usb_device_index on success or <1 if device was not pressent.

  Throwing error on all other failures.


  Release an interface previously claimed with libusb_claim_interface().

You should release all claimed interfaces before closing a device handle.

\*============================================================================*/
Php::Value io_usb_close(Php::Parameters &params){
  if( params.size() > 0 ){
    int handle_index = params[0];
    libusb_close(usb->handle_list[handle_index].libusb_handle);
    usb->remove_handle(handle_index);
  }

  return 0;
}


/*============================================================================*\
  USB write

  length = io_usb_write(devive_handle_index, EndpointID, data [,release = true [, detach_from kernel = false]])

  parameters:
    endpoint ID: a string of numbers 0-255 separated with -
      <configuration> - <alternative> - <interface> - <endpoint>

    data: string to write

    release: When writing, the interface is claimed. If release is true, the interfaces is released after use.

    detach: detach driver from kerne.

  return:
    number of bytes written

https://www.dreamincode.net/forums/topic/148707-introduction-to-using-libusb-10/

\*============================================================================*/
Php::Value io_usb_write(Php::Parameters &params){
  int rc;
  int bytes_written;
  bool release = true;
  bool detach = false;
  EndpointID = ep;

  if( params.size() < 1 )
    throw Php::Exception("io_usb_write Please specify device handle");
  if( params.size() < 2 )
    throw Php::Exception("io_usb_write Please specify endpoint ID");
  if( params.size() < 3 )
    throw Php::Exception("io_usb_write: Please specify data to write");

  int device_handle_index = param[0];

  string endpointID = param[1];
  string data = param[2];

  if( params.size() > 3 )
    release = param[3];

  if( params.size() > 4 )
    detach = param[4];

  endpoint = interpret_endpoint_str(endpointID);

  if( ep.endpoint == 0 )
    throw Php::Exception("io_usb_write: Please use io_usb_controle to write to controle");

  auto handle = usb->handle[device_handle_index].libusb_handle;

  if(  detach
    && !usb->handle[device_handle_index].detached
    && libusb_kernel_driver_active(handle, 0) == 1){

    usb->handle[device_handle_index].detached =
      libusb_detach_kernel_driver(dev_handle, 0) == 0;
    cout<<"Kernel Driver Active"<<endl;

    if( usb->handle[device_handle_index].detached)
      cout<<"Kernel Driver Detached!"<<endl;
  }

  void handle_return_code_error( int rc ){
    switch( rc ){
      case: 0;
        break;
      case: LIBUSB_ERROR_NOT_FOUND;
        throw Php::Exception("io_usb_write: no interface by that number: " + endpointID + " returned code (" + to_string( rc ) + ")");
      case: LIBUSB_ERROR_BUSY;
        throw Php::Exception("io_usb_write: interface is busy: " + endpointID + " returned code (" + to_string( rc ) + ")");
      case: LIBUSB_ERROR_NO_DEVICE;
        throw Php::Exception("io_usb_write: device is unavailable: " + endpointID + "returned code (" + to_string( rc ) + ")");
      default:
        throw Php::Exception("io_usb_write: Failed to write: " + endpointID + "returned code (" + to_string( rc ) + ")");
    }
  }

  if( usb->handle[device_handle_index].configuration != ep.configuration )
    handle_return_code_error(
      libusb_set_configuration(handle, ep.configuration)
    );

  // if(  find out if interface is not yet clamed
    handle_return_code_error(
      libusb_claim_interface(handle, ep.interface)
    );

  handle_return_code_error(
    libusb_set_interface_alt_setting(handle, ep.interface, ep.altsetting)
  );

  cout<<"Data->"<<data<<"<-"<<endl;
  cout<<"Writing Data..."<<endl;

  handle_return_code_error(
    libusb_bulk_transfer(
       handle
      ,ep.endpoint
      ,data.c_str()
      ,data.length()
      ,&bytes_written
      ,0
    )
  );

  if( release )
	  libusb_release_interface(handle, ep.interface);
    // set usb... interface released

  return bytes_written;
}


/*============================================================================*\
  USB write controle

  length = io_usb_write(devive_handle_index, data [,release = true [, detach_from kernel = false]])

  parameters:
    endpoint ID: a string of numbers 0-255 separated with -
      <configuration> - <alternative> - <interface> - <endpoint>

    data: string to write

    release: When writing, the interface is claimed. If release is true, the interfaces is released after use.

    detach: detach driver from kerne.

https://www.dreamincode.net/forums/topic/148707-introduction-to-using-libusb-10/

\*============================================================================*/
Php::Value io_usb_write(Php::Parameters &params){

  // set fields for the setup packet as needed
  uint8_t bmReqType   = 0;   // the request type (direction of transfer)
  uint8_t bReq        = 0;   // the request field for this packet
  uint16_t wVal       = 0;   // the value field for this packet
  uint16_t wIndex     = 0;   // the index field for this packet
  unsigned char* data = ‘ ‘; // the data buffer for the in/output data
  uint16_t wLen       = 0;   // length of this setup packet
  unsigned int to     = 0;   // timeout duration (if transfer fails)

  // transfer the setup packet to the USB device
  int config = libusb_control_transfer(dh,bmReqType,bReq,wVal,wIndex,data,wLen,to);

    if (config < 0) {
      //  errx(1,”\n\nERROR: No data transmitted to device %d\n\n”,DEV_ID);
    }


}
