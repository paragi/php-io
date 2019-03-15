/*============================================================================*\
  Basic IO functions

  (c) By Simon Rigét @ Paragi 2018
  License: Apache




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

int do_list();
void printdev(libusb_device *dev); //prototype of the function

using namespace std;

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

#define HASH1  0x10
#define HASH2  0x02
#define HASHSZ 512
#define SYSFS_DEV_ATTR_PATH "/sys/bus/usb/devices/%d-%d/%s"
int read_sysfs_prop(char *buf, size_t size, uint8_t bnum, uint8_t pnum, char *propname)
{
	int n, fd;
	char path[PATH_MAX];

	buf[0] = '\0';
	snprintf(path, sizeof(path), SYSFS_DEV_ATTR_PATH, bnum, pnum, propname);
	fd = open(path, O_RDONLY);

	if (fd == -1)
		return 0;

	n = read(fd, buf, size);

	if (n > 0)
		buf[n-1] = '\0';  // Turn newline into null terminator

	close(fd);
	return n;
}


/*============================================================================*\
  Device list classes


  	libusb_config_descriptor *config;
  	libusb_get_config_descriptor(dev, 0, &config);
  	cout<<"Interfaces: "<<(int)config->bNumInterfaces<<" ||| ";
  	const libusb_interface *inter;
  	const libusb_interface_descriptor *interdesc;
  	const libusb_endpoint_descriptor *epdesc;
  	for(int i=0; i<(int)config->bNumInterfaces; i++) {
  		inter = &config->interface[i];
  		cout<<"Number of alternate settings: "<<inter->num_altsetting<<" | ";
  		for(int j=0; j<inter->num_altsetting; j++) {
  			interdesc = &inter->altsetting[j];
  			cout<<"Interface Number: "<<(int)interdesc->bInterfaceNumber<<" | ";
  			cout<<"Number of endpoints: "<<(int)interdesc->bNumEndpoints<<" | ";
  			for(int k=0; k<(int)interdesc->bNumEndpoints; k++) {
  				epdesc = &interdesc->endpoint[k];
  				cout<<"Descriptor Type: "<<(int)epdesc->bDescriptorType<<" | ";
  				cout<<"EP Address: "<<(int)epdesc->bEndpointAddress<<" | ";
  			}
  		}
  	}
  	cout<<endl<<endl<<endl;
  	libusb_free_config_descriptor(config);

\*============================================================================*/
class Device {
  private:
  	libusb_device_descriptor descriptor_pointer;

  string get_descriptor_string(
     libusb_device_handle * dev_handle
    ,uint8_t string_index){

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
      USBClassNames usb_class_string;

    	rc = libusb_get_device_descriptor(device, &descriptor_pointer);
    	if (rc < 0)
        throw Php::Exception("io_usb unable to read device descriptor");

      uint8_t bus_number = libusb_get_bus_number(device);
		  uint8_t device_address = libusb_get_device_address(device);
      uint8_t port_number = libusb_get_port_number(device);


      rc = libusb_open(device,&dev_handle);
    	if (rc != 0)
        throw Php::Exception(string ("io_usb unable to open and read device descriptor. code (") + to_string(rc) + string(")"));

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

      auto class_string = usb_class_string.get_class_text(
         descriptor_pointer.bDeviceClass
        ,descriptor_pointer.bDeviceSubClass
        ,descriptor_pointer.bDeviceProtocol
        ,false );

      descriptor["deviceClass"] = class_string.class_name;
      descriptor["subclass"]    = class_string.sub_class;
      descriptor["protocol"]    = class_string.protocol;

      descriptor["maxPacketSize"]   = descriptor_pointer.bMaxPacketSize0;
      descriptor["descriptorType"]  = descriptor_pointer.bDescriptorType;


      descriptor["configurations"]  = descriptor_pointer.bNumConfigurations;

      auto r = libusb_get_port_numbers(device, path, sizeof(path));
  		if (r > 0) {
        string path_str = "";
  			for (auto j = 0; j < r; j++)
          path_str += "." + to_string(path[j]);
        descriptor["path"] = path_str.substr(1);

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
          + ";" + to_string(bus)
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

\*============================================================================*/
Php::Value io_usb_list(Php::Parameters &params){
  //Php::Value ret = -1;

	libusb_context *ctx = NULL; //a libusb session

  if( params.size() > 0 ){
    string vendor_id = params[0];
    if( params.size() > 1 ){
      string product_id = params[1];
    }
  }

  //do_list();

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




int do_list() {
	libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
	libusb_context *ctx = NULL; //a libusb session
	int r; //for return values
	ssize_t cnt; //holding number of devices in list
	r = libusb_init(&ctx); //initialize a library session
	if(r < 0) {
		cout<<"Init Error "<<r<<endl; //there was an error
				return 1;
	}
	libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation
	cnt = libusb_get_device_list(ctx, &devs); //get the list of devices
	if(cnt < 0) {
		cout<<"Get Device Error"<<endl; //there was an error
	}
	cout<<cnt<<" Devices in list."<<endl; //print total number of usb devices
		ssize_t i; //for iterating through the list
	for(i = 0; i < cnt; i++) {
				printdev(devs[i]); //print specs of this device
		}
		libusb_free_device_list(devs, 1); //free the list, unref the devices in it
		libusb_exit(ctx); //close the session
		return 0;
}

void printdev(libusb_device *dev) {
	libusb_device_descriptor desc;
	int r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
		cout<<"failed to get device descriptor"<<endl;
		return;
	}
	cout<<"Number of possible configurations: "<<(int)desc.bNumConfigurations<<"  ";
	cout<<"Device Class: "<<(int)desc.bDeviceClass<<"  ";
	cout<<"VendorID: "<<desc.idVendor<<"  ";
	cout<<"ProductID: "<<desc.idProduct<<endl;
	libusb_config_descriptor *config;
	libusb_get_config_descriptor(dev, 0, &config);
	cout<<"Interfaces: "<<(int)config->bNumInterfaces<<" ||| ";
	const libusb_interface *inter;
	const libusb_interface_descriptor *interdesc;
	const libusb_endpoint_descriptor *epdesc;
	for(int i=0; i<(int)config->bNumInterfaces; i++) {
		inter = &config->interface[i];
		cout<<"Number of alternate settings: "<<inter->num_altsetting<<" | ";
		for(int j=0; j<inter->num_altsetting; j++) {
			interdesc = &inter->altsetting[j];
			cout<<"Interface Number: "<<(int)interdesc->bInterfaceNumber<<" | ";
			cout<<"Number of endpoints: "<<(int)interdesc->bNumEndpoints<<" | ";
			for(int k=0; k<(int)interdesc->bNumEndpoints; k++) {
				epdesc = &interdesc->endpoint[k];
				cout<<"Descriptor Type: "<<(int)epdesc->bDescriptorType<<" | ";
				cout<<"EP Address: "<<(int)epdesc->bEndpointAddress<<" | ";
			}
		}
	}
	cout<<endl<<endl<<endl;
	libusb_free_config_descriptor(config);
}
