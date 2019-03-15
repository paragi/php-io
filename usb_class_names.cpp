/*============================================================================*\
  USB class names

  Convert USB repported class names to human readable text strings

  (c) By Simon Rigét @ Paragi 2019
  License: Apache

\*============================================================================*/
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <locale>
#include <map>

#include "usb_class_names.h"

using namespace std;

USBClassNames::Class_descriptor USBClassNames::get_class_text(
     int classID
    ,int sub_classID
    ,int protocolID
    ,bool is_interface ) {


    Class_descriptor ret {"undefined","undefined","undefined"};

    do {
      auto it_usb_class = usb_class.find(classID);
      if( it_usb_class == usb_class.end()) break;

      if( ( is_interface && !it_usb_class->second.valid_for_interface)
          ||
          ( !is_interface && !it_usb_class->second.valid_for_device) )
        break;

      ret.class_name = it_usb_class->second.description;

      auto it_sub_class = it_usb_class->second.sub_class.find(sub_classID);
      if( it_sub_class == it_usb_class->second.sub_class.end()) break;

      ret.sub_class = it_sub_class->second.description;

      auto it_protocol = it_sub_class->second.protocol.find(protocolID);
      if( it_protocol == it_sub_class->second.protocol.end()) break;

      ret.protocol = it_protocol->second.description;

    } while(false);

cout<< "class ID: "<< classID << ret.class_name<<" sub: "<<sub_classID<<ret.sub_class<<" prot: "<<protocolID<<ret.protocol<<endl;


    return ret;
  };
