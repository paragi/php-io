/*============================================================================*\
  USB class names header

  Convert USB repported class names to human readable text strings

  (c) By Simon Rigét @ Paragi 2019
  License: Apache

\*============================================================================*/
#ifndef USBClassNames_defined
#define USBClassNames_defined

using namespace std;


class USBClassNames{
  private:

  typedef struct {
    string description;
  } Protocol;

  typedef struct {
      string description;
      map<int, Protocol> protocol;
  } Sub_class;

  typedef struct  {
    string description;
    bool valid_for_device;
    bool valid_for_interface;
    map<int, Sub_class> sub_class;
  } Usb_class;

  map<int, Usb_class> usb_class {
    { 0x01, {"Audio", false, true
      ,{{ 0x01, {"Control Device"
      }}}
    }}
    ,{ 0x02, {"Communications and CDC Control", true, true
    }}
    ,{ 0x03, {"Human Interface Device (HID)", false, true
    }}
    ,{ 0x05, {"Physical", false, true
    }}
    ,{ 0x06, {"Still Imaging", false, true
      ,{{ 0x01, {"" // ?
        ,{{ 0x01, { "" }}} //?
      }}}
    }}
    ,{ 0x07, {"Printer", false, true
    }}
    ,{ 0x08, {"Mass Storage (MSD)", false, true
    }}
    ,{ 0x09, {"Hub", true, false
      ,{{ 0x00, {""
        ,{
           { 0x00, { "Full Speed Hub" }}
          ,{ 0x01, { "High Speed Hub with single TT" }}
          ,{ 0x02, { "High Speed Hub with single TTs" }}
        }
      }}}
    }}
    ,{ 0x0A, {"CDC-Data", false, true
    }}
    ,{ 0x0B, {"Smart Card", false, true
    }}
    ,{ 0x0D, {"Content Security", false, true
      ,{{ 0x00, {""
        ,{{ 0x00, { "" }}}
      }}}
    }}
    ,{ 0x0E, {"Video", false, true
    }}
    ,{ 0x0F, {"Personal Healthcare", false, true
    }}
    ,{ 0x10, {"Audio/Video Devices", false, true
      ,{
         { 0x01, {"AVControl Interface" ,{ { 0x00, { "" }} }}}
        ,{ 0x02, {"AVData Video Streaming Interface" ,{ { 0x00, { "" }} }}}
        ,{ 0x03, {"" ,{ { 0x00, { "" }} }}} //?
      }
    }}
    ,{ 0x11, {"Billboard Device Class", true, false
      ,{{ 0x00, {""
        ,{{ 0x00, { "" }}}
      }}}
    }}
    ,{ 0xDC, {"Diagnostic Device", true, true
      ,{{ 0x01, {"Control Device"
        ,{{ 0x01, { "noget protocol" }}}
      }}}
    }}
    ,{ 0xE0, {"Wireless Controller", false, true
      ,{{ 0x01, {"Control Device"
        ,{{ 0x01, { "noget protocol" }}}
      }}}
    }}
    ,{ 0xEF, {"Miscellaneous", true, true
      ,{{ 0x01, {"Control Device"
        ,{{ 0x01, { "noget protocol" }}}
      }}}
    }}
    ,{ 0xFE, {"Application Specific", false, true
      ,{{ 0x01, {"Control Device"
        ,{{ 0x01, { "noget protocol" }}}
      }}}
    }}
    ,{ 0xFF, {"Vendor Specific", true, true
    }}
  };

  public:

  string get_class_text(
     int classID
    ,int sub_classID
    ,int protocolID
    ,bool is_interface );

};

#endif
