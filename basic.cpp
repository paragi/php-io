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

#include <phpcpp.h>

#include <sys/types.h>
#include <sys/stat.h>
//#include <fcntl.h>
#include <errno.h>
//#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

// Include definitions for ioclt
#include <asm-generic/socket.h>
#include <termios.h>
#include <linux/ax25.h>
//#include <linux/cdk.h>
#include <linux/cdrom.h>
//#include <linux/cm206.h>
#include <linux/cyclades.h>
#include <linux/fd.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <linux/if_eql.h>
#include <linux/if_plip.h>
#include <linux/if_ppp.h>
#include <linux/ipx.h>
#include <linux/kd.h>
#include <linux/lp.h>
//#include <linux/mroute.h>
#include <linux/msdos_fs.h>
#include <linux/netrom.h>
#include <linux/mtio.h>
//#include <linux/wireless.h>
//#include <linux/sbpcd.h>
#include <linux/scc.h>
#include <scsi/scsi.h>
//#include <linux/smb_fs.h>
#include <linux/soundcard.h>
//#include <linux/timerfd.h> replace with <fcntl.h>
#include <fcntl.h>
//#include <linux/umsdos_fs.h>
#include <linux/vt.h>


#include "basic.h"
#include "common.h"

#ifndef TIOCM_LOOP
    #define TIOCM_LOOP 0x8000
#endif

using namespace std;

#define MAX_OPEN_FILES 256

map<int, string> io_error_string;

int str2flags(string strflags){
    unsigned int i, length;
    unsigned int flags = O_RDONLY;

    length =  strflags.length();

    for(i=0; i < length; i++){
        switch(strflags[i]){
            case 'r': // Read only
                flags &= ~(O_RDONLY | O_WRONLY | O_RDWR | O_APPEND | O_CREAT | O_TRUNC);
                flags |= O_RDONLY;
                break;
            case 'w': // Write only
                flags &= ~(O_RDONLY | O_WRONLY | O_RDWR | O_APPEND | O_CREAT | O_TRUNC);
                flags |= O_WRONLY | O_CREAT | O_TRUNC;
                break;
            case 'a': // Append
                flags &= ~(O_RDONLY | O_WRONLY | O_RDWR | O_APPEND | O_CREAT | O_TRUNC);
                flags |= O_APPEND | O_CREAT;
                break;
            case 'x': // Create and open for writing only. Fail if exists
                flags &= ~(O_RDONLY | O_WRONLY | O_RDWR | O_APPEND | O_CREAT | O_TRUNC);
                flags |= O_WRONLY | O_EXCL | O_CREAT;
                break;
            case 'c': // Writing only. If not exist, it is created but not truncated.
                flags &= ~(O_RDONLY | O_WRONLY | O_RDWR | O_APPEND | O_CREAT | O_TRUNC);
                flags |=  O_WRONLY | O_CREAT;
                break;
            case 'd': // read, write synchronized. fail if not exists
                flags &= ~(O_RDONLY | O_WRONLY | O_RDWR | O_APPEND | O_CREAT | O_TRUNC);
                flags |= O_RDWR | O_DSYNC;
                break;

            // Modifiers
            case '+': // read and write
                if( i == 0) break;
                flags &= ~(O_RDONLY | O_WRONLY | O_APPEND );
                flags |= O_RDWR;
                break;
/*
            case 't': // text
                if( i == 0) break;
                flags &= !O_BINARY
                flags |= _O_TEXT;
                break;
*/
            case 'e':
                if( i == 0) break;
                flags |= O_CLOEXEC;
                break;
            case 'n':
                if( i == 0) break;
                flags |= O_NONBLOCK;
                break;
            case 's':
                if( i == 0) break;
                flags |= O_DSYNC;
                break;

        }
    }
    return flags;
}


Php::Value io_test(Php::Parameters &params){
    Php::Value ret;
    ret = str2flags(params[0]);
    return ret;
}


const map<long int, int> dataSpeed = {
     {0,B0}
    ,{50,B50}
    ,{75,B75}
    ,{110,B110}
    ,{134,B134}
    ,{150,B150}
    ,{200,B200}
    ,{300,B300}
    ,{600,B600}
    ,{1200,B1200}
    ,{1800,B1800}
    ,{2400,B2400}
    ,{4800,B4800}
    ,{9600,B9600}
    ,{19200,B19200}
    ,{38400,B38400}
    ,{57600,B57600}
    ,{115200,B115200}
    ,{230400,B230400}
    ,{460800,B460800}
    ,{500000,B500000}
    ,{576000,B576000}
    ,{921600,B921600}
    ,{1000000,B1000000}
    ,{1152000,B1152000}
    ,{1500000,B1500000}
    ,{2000000,B2000000}
    ,{2500000,B2500000}
    ,{3000000,B3000000}
    ,{3500000,B3500000}
    ,{4000000,B4000000}
};


const map<long int, int> dataBit = {
     {5,CS5}
    ,{6,CS6}
    ,{7,CS7}
    ,{8,CS8}
};

const map<long int, int> stopBit = {
     {1,0}
    ,{2,CSTOPB}
};

/*============================================================================*\
  Open file or devices

  int $file_descriptor = io_open(string $file_name, string $flags [, int mode])
  return: File descriptor or false.

  flags: r|w|a|x|c [+] [e][n][s]

  flags has the same meaning af for fopen, with the addition of:
  'a' = O_ASYNC
  'n' = O_NONBLOCK
  "d" = O_DSYNC

  mode is a standart file mode value eg: octdec('77')

  Return Value:
    File descriptor or false.

\*============================================================================*/
Php::Value io_open(Php::Parameters &params){
    Php::Value ret = -1;
    int fd = -1;
    int flags = 0;
    mode_t mode = 0;
    string error;
    //string path_str;
    //string flags_str;

    if( params.size() < 2 ){
        throw Php::Exception("io_open takes at least 2 parameters. "
            + std::to_string( params.size() )
            + " was provided"
        );
    }

    string path_str = params[0];
    string flags_str = params[1];

    if( params.size() > 2 )
        mode = std::stoi(params[2]);

    flags = str2flags(flags_str);
    if( flags <= 0 ){
        throw Php::Exception(
            "io_open('" + path_str + "','" + flags_str + "') Failed "
            + ": parameter 2 must contain valid flags. '"
            + flags_str
            + "' was provided"
        );
    }

    if( mode > 04777 )
        mode = 0;
    if( mode <= 0 && (flags & O_CREAT))
        mode = 0640;

    if( mode > 0){
        fd = open(path_str.c_str(), flags, mode);
    }else{
        fd = open(path_str.c_str(), flags);
    }

    if(fd < 0){
        throw Php::Exception(
            "io_open('" + path_str + "','" + flags_str + "') Failed "
            + "("
            + std::to_string( fd )
            + ") : "
            + string(strerror( errno ))
        );
    }

    ret = fd;
    return ret;
}

/*============================================================================*\
  Close
\*============================================================================*/
Php::Value io_close(Php::Parameters &params){
    Php::Value ret = 0;
    int rc,fd;

    if( params.size() < 1 ){
        throw Php::Exception( ": io_close takes 1 parameter. None was provided"
        );
    }

    fd = std::stoi(params[0]);

    rc = close(fd);
    if(rc != 0){
        throw Php::Exception(
            "io_close("
            + std::to_string( fd )
            + ") Failed: "
            + string(strerror( errno ))
        );
    }

    ret = rc;
    return ret;
}

/*============================================================================*\
  Write

  io_write(int $fd, string &binary_data);

  return: bytes written
\*============================================================================*/
Php::Value io_write(Php::Parameters &params){
    Php::Value ret = 0;
    int fd, length;

    if( params.size() < 2 ){
        throw Php::Exception("io_write takes 2 parameters. "
            + std::to_string( params.size() )
            + " was provided"
        );
    }

    fd = std::stoi( params[0] );
    string buffer = params[1];

    length = write(fd, buffer.c_str(),  buffer.length() );
    if( length < 0){
        throw Php::Exception(
            "io_write("
            + std::to_string( fd )
            +","
            + buffer
            + ") Failed: "
            + string(strerror( errno ))
        );
    }

    ret = length;
    return ret;
}

/*============================================================================*\
  Read from device

  string read(int $file_descriptor, number $length [, char $end_char])

  parameters:

  file_descriptor:  returned by io_open.

  length: number of charakters to read (maximum)

 ##end_char: End of tmessage charakter

  return:
    a sting with the charakters read of 0 if empty (non-blockting) or if the read operation failed.

  Get specified number of charakters from device, or until an end_char is recieved.
  If $end_char is specified anf $length is greater than 0, which ever is true first, determins the length of the returned string.

  Non-blocking: If timeout is set to zero (with io_set_serial) io_read will return what ever is in the buffer, but not exceeding the above.

  blocking: If timeout is set to -1 (with io_set_serial) io_read wait until $length is reached.


NB: end_char not implementet yet

FIONREAD  int *argp
              Get the number of bytes in the input buffer.

\*============================================================================*/
Php::Value io_read(Php::Parameters &params){
    Php::Value ret = "";
    int fd, length;
    int end_char = -1;

    if( params.size() < 2 ){
        throw Php::Exception("io_read takes at least 2 parameters. "
            + std::to_string( params.size() )
            + " was provided"
        );
    }

    // File descriptor
    fd = std::stoi( params[0] );

    // Length
    length = std::stoi( params[1] );
    char * buffer = new char[length +1];
    if(!buffer){
        throw Php::Exception(
              "io_read was unable to allocate "
            + std::to_string(length)
            + "bytes of memory"
        );
    }

    // End of message char
    if(params.size() > 2){
        string str = params[2];
        end_char = str[0];
    }

    // read from device
    length = read(fd, buffer, length);
    if( length < 0){
        throw Php::Exception(
            "io_read("
            + std::to_string( fd )
            +","
            + std::to_string( length )
            + ") Failed: "
            + string(strerror( errno ))
        );
    }
    buffer[length] = 0;

    if(length > 0){
        ret = std::string(buffer, length);
    }

    return ret;
}

/*============================================================================*\
  Ioctl

  string ioctl(int $file_descriptor, int $command , &$buffer)

  Controle device sriver parameters.

  Use of this function makes your code very hardware and operating system
  dependent.

\*============================================================================*/
Php::Value io_ioctl(Php::Parameters &params){
    Php::Value ret = false;
    int fd = std::stoi( params[0] );
    int command = std::stoi( params[1] );
    int rc = -1;
    int int_buffer;

    if( params.size() < 2 ){
            throw Php::Exception("io_ioctl takes at least 2 parameters. "
            + std::to_string( params.size() )
            + " was provided"
        );
    }

    switch (command){
        // int *
        case FIOSETOWN:
        case SIOCSPGRP:
        case FIOGETOWN:
        //case SIOCGPGR:
        case SIOCATMARK:
        case TIOCOUTQ:
        case TIOCMGET:
        case TIOCGSOFTCAR:
        case FIONREAD:
        //case TIOCINQ:
        case TIOCGETD:
        case VT_OPENQRY:
        case SOUND_MIXER_READ_VOLUME:
        case SOUND_MIXER_READ_BASS   :
        case SOUND_MIXER_READ_TREBLE :
        case SOUND_MIXER_READ_SYNTH  :
        case SOUND_MIXER_READ_PCM    :
        case SOUND_MIXER_READ_SPEAKER:
        case SOUND_MIXER_READ_LINE   :
        case SOUND_MIXER_READ_MIC    :
        case SOUND_MIXER_READ_CD     :
        case SOUND_MIXER_READ_IMIX   :
        case SOUND_MIXER_READ_ALTPCM :
        case SOUND_MIXER_READ_RECLEV :
        case SOUND_MIXER_READ_IGAIN  :
        case SOUND_MIXER_READ_OGAIN  :
        case SOUND_MIXER_READ_LINE1  :
        case SOUND_MIXER_READ_LINE2  :
        case SOUND_MIXER_READ_LINE3  :
        case SOUND_MIXER_READ_MUTE   :
        //case SOUND_MIXER_READ_ENHANCE:
        //case SOUND_MIXER_READ_LOUD   :
        case SOUND_MIXER_READ_RECSRC :
        case SOUND_MIXER_READ_DEVMASK:
        case SOUND_MIXER_READ_RECMASK:
        case SOUND_MIXER_READ_STEREODEVS:
        case SOUND_MIXER_READ_CAPS      :
        case SOUND_MIXER_WRITE_VOLUME :
        case SOUND_MIXER_WRITE_BASS   :
        case SOUND_MIXER_WRITE_TREBLE :
        case SOUND_MIXER_WRITE_SYNTH  :
        case SOUND_MIXER_WRITE_PCM    :
        case SOUND_MIXER_WRITE_SPEAKER:
        case SOUND_MIXER_WRITE_LINE   :
        case SOUND_MIXER_WRITE_MIC    :
        case SOUND_MIXER_WRITE_CD     :
        case SOUND_MIXER_WRITE_IMIX   :
        case SOUND_MIXER_WRITE_ALTPCM :
        case SOUND_MIXER_WRITE_RECLEV :
        case SOUND_MIXER_WRITE_IGAIN  :
        case SOUND_MIXER_WRITE_OGAIN  :
        case SOUND_MIXER_WRITE_LINE1  :
        case SOUND_MIXER_WRITE_LINE2  :
        case SOUND_MIXER_WRITE_LINE3  :
        case SOUND_MIXER_WRITE_MUTE   :
        //case SOUND_MIXER_WRITE_ENHANCE:
        //case SOUND_MIXER_WRITE_LOUD   :
        case SOUND_MIXER_WRITE_RECSRC :
        case SOUND_PCM_READ_RATE      :
        case SOUND_PCM_READ_CHANNELS  :
        case SOUND_PCM_READ_BITS      :
        case SOUND_PCM_READ_FILTER    :
        case SNDCTL_DSP_SUBDIVIDE     :
        case SNDCTL_DSP_SETFRAGMENT   :
        case SNDCTL_DSP_GETFMTS       :
        case SNDCTL_DSP_SETFMT        :
        case SNDCTL_DSP_SPEED         :
        case SNDCTL_DSP_STEREO        :
        case SNDCTL_DSP_GETBLKSIZE    :
        case SOUND_PCM_WRITE_CHANNELS :
        case SOUND_PCM_WRITE_FILTER   :
        case SNDCTL_MIDI_PRETIME    :
        case SNDCTL_TMR_TIMEBASE    :
        case SNDCTL_TMR_TEMPO       :
        case SNDCTL_TMR_SOURCE      :
        case SNDCTL_TMR_SELECT      :
        case SNDCTL_SEQ_CTRLRATE    :
        case SNDCTL_SEQ_GETOUTCOUNT :
        case SNDCTL_SEQ_GETINCOUNT  :
        case LPGETIRQ     :
        case LPGETSTATUS  :
        case KDGKBMODE    :
        case KDGKBLED     :
        case KDGKBMETA    :
        case KDGETMODE    :
        case PPPIOCGFLAGS :
        case PPPIOCGASYNCMAP    :
        case PPPIOCGUNIT        :
        case PPPIOCGDEBUG       :
        case HDIO_GET_UNMASKINTR   :
        case HDIO_GET_MULTCOUNT    :
        case HDIO_GET_KEEPSETTINGS :
        // case HDIO_GET_CHIPSET      :
        case HDIO_GET_NOWERR       :
        case HDIO_GET_DMA          :
        case HDIO_DRIVE_CMD        :
        case BLKROGET              :
        case FIBMAP                :
        case FIGETBSZ              :
        case FS_IOC_GETFLAGS       :
        case FS_IOC_SETFLAGS       :
        case FS_IOC_GETVERSION     :
        case FS_IOC_SETVERSION     :
        //case FS_IOC32_SETFLAGS     :
        //case FS_IOC32_GETVERSION   :
        //case FS_IOC32_SETVERSION   :
        case CYGETTHRESH :
        case CYGETDEFTHRESH:
        case CYGETTIMEOUT:
        case CYGETDEFTIMEOUT:
        case SNDCTL_SEQ_NRSYNTHS:
        case SNDCTL_SEQ_NRMIDIS:
        case SNDCTL_SYNTH_MEMAVL:
        case SIOCGIFENCAP:
            int_buffer = std::stoi( params[2] );
            rc = ioctl(fd, command, &int_buffer);
            params[2] = int_buffer;
            break;

/*

        // Const int *
        case TIOCMBIS:
        case TIOCMBIC:
        case TIOCMSET:
        case TIOCSSOFTCAR:
        case TIOCPKT:
        case FIONBIO:
        case TIOCSETD:
        case TIOCSERGWILD:
        case TIOCSERGETLSR:
        case FIOASYNC        :
        case TIOCSERSWILD    :
        case SIOCAX25NOUID   :
        case SIOCAX25DIGCTL  :
        case SNDCTL_TMR_METRONOME :
        case SNDCTL_MIDI_MPUMODE  :
        case PPPIOCSFLAGS       :
        case PPPIOCSASYNCMAP    :
        case PPPIOCSINPSIG      :
        case PPPIOCSDEBUG       :
        case PPPIOCSMRU         :
        case PPPIOCRASYNCMAP    :
        case PPPIOCSMAXCID      :
        case BLKROSET           :
        case SIOCNRRTCTL      :
        case DDIOCSDBG          :
        case SCSI_IOCTL_PROBE_HOST   :
        case SNDCTL_SEQ_TESTMIDI     :
        case SNDCTL_SEQ_RESETSAMPLES :
        case SNDCTL_SEQ_THRESHOLD    :
        case SNDCTL_FM_4OP_ENABLE    :
        case SIOCSIFENCAP            :
            int buffer = std::stoi( params[2] );
            rc = ioctl(fd, command, &buffer);
            break;

        // int
        case TCSBRK:
        case TCXONC:
        case TCFLSH:
        case TIOCSCTTY:
        case TCSBRKP:
        case CDROMEJECT_SW :
        case VT_RELDISP    :
        case VT_ACTIVATE   :
        case VT_WAITACTIVE :
        case VT_DISALLOCATE:
        case LPCHAR        :
        case LPTIME        :
        case LPABORT       :
        case LPSETIRQ      :
        case LPWAIT        :
        case LPCAREFUL     :
        case LPABORTOPEN   :
        case KDSKBMODE     :
        case KDSKBMETA     :
        case KDSKBLED      :
        case KDSIGACCEPT   :
        case KIOCSOUND     :
        case KDMKTONE      :
        case KDSETLED      :
        case KDADDIO       :
        case KDDELIO       :
        case KDSETMODE     :
        case HDIO_SET_MULTCOUNT    :
        case HDIO_SET_UNMASKINTR   :
        case HDIO_SET_KEEPSETTINGS :
        case HDIO_SET_CHIPSET      :
        case HDIO_SET_NOWERR       :
        case HDIO_SET_DMA          :
        case FDRESET          :
        case FDSETEMSGTRESH   :
        case CM206CTL_GET_STAT     :
        case CM206CTL_GET_LAST_STAT:
        case CYSETTHRESH       :
        case CYSETDEFTHRESH    :
        case CYSETTIMEOUT      :
        case CYSETDEFTIMEOUT   :
        case CDROMAUDIOBUFSIZ:
            string str = params[2];
            char buffer = str[0];

            int buffer = std::stoi( params[2] );
            rc = ioctl(fd, command, buffer);
            break;

        // Void
        case FIONCLEX:
        case FIOCLEX:
        case TIOCSERCONFIG:
        case TIOCEXCL:
        case TIOCNXCL:
        case TIOCCONS:
        case TIOCNOTTY:
        case STL_BINTR   :
        case STL_BSTART  :
        case STL_BSTOP   :
        case STL_BRESET  :
        case CDROMPAUSE  :
        case CDROMRESUME :
        case CDROMSTOP   :
        case CDROMSTART  :
        case CDROMEJECT  :
        case CDROMRESET  :
        case VT_SENDSIG  :
        case UMSDOS_INIT_EMD     :
        case SNDCTL_SEQ_PERCMODE :
        case SNDCTL_SEQ_PANIC    :
        case SNDCTL_DSP_RESET    :
        case SNDCTL_DSP_SYNC     :
        case SNDCTL_DSP_POST     :
        case SNDCTL_DSP_NONBLOCK :
        case SNDCTL_COPR_RESET   :
        case SNDCTL_TMR_START    :
        case SNDCTL_TMR_STOP     :
        case SNDCTL_TMR_CONTINUE :
        case SNDCTL_SEQ_RESET :
        case SNDCTL_SEQ_SYNC  :
        case SIOCGIFSLAVE     :
        case SIOCSIFSLAVE     :
        case SIOCADDRTOLD     :
        case SIOCDELRTOLD     :
        case LPRESET      :
        case KDENABIO     :
        case KDDISABIO    :
        case KDMAPDISP    :
        case KDUNMAPDISP  :
        case FDCLRPRM     :
        case FDMSGON      :
        case FDMSGOFF     :
        case FDFMTBEG     :
        case FDFMTEND     :
        case FDFLUSH      :
        case FDWERRORCLR  :
        case FDTWADDLE   :
        case BLKRRPART    :
        case BLKFLSBUF    :
        case SIOCNRDECOBS :
        case TIOCSCCINI   :
        case SCSI_IOCTL_TAGGED_ENABLE  :
        case SCSI_IOCTL_TAGGED_DISABLE :
        case SIOCSIFLINK                :
            rc = ioctl(fd, command, NULL);
            break;

        // Char *
        case KDGETLED   :
        case KDGKBTYPE  :
        case SIOCGIFNAME:
            string str = params[2];
            if(str.length()<1){
                io_error_string[fd] = "device ioctl expects parameter 3 to be of type string with the lenght of 1";
                return false;
            }
            char buffer = str[0];
            rc = ioctl(fd, command, &buffer);
            params[2] = buffer;
            break;

        // const char *
        case TIOCSTI:
        case TIOCLINUX:
        case SIOCAIPXITFCRT:
        case SIOCAIPXPRISLT:
            string str = params[2];
            if(str.length()<1){
                io_error_string[fd] = "device ioctl expects parameter 3 to be of type string with the lenght of 1";
                return false;
            }
            char buffer = str[0];
            rc = ioctl(fd, command, &buffer);
            break;

        // unsigned long *
        case BLKGETSIZE :
        case BLKRAGET   :
        case BLKRASET:
            unsigned long buffer = std::stoi( params[2] );
            rc = ioctl(fd, command, &buffer);
            params[2] = buffer;
            break;

        case TFD_IOC_SET_TICKS:
            uint64_t buffer = std::stoi( params[2] );
            rc = ioctl(fd, command, &buffer);
            params[2] = buffer;
            break;

        // __u32 *
        case FAT_IOCTL_GET_ATTRIBUTES:
        case FAT_IOCTL_GET_VOLUME_ID :
          __u32 buffer = std::stoi( params[2] );
          rc = ioctl(fd, command, &buffer);
          params[2] = buffer;
          break;

        // const __u32 *
        case FAT_IOCTL_SET_ATTRIBUTES:
            const __u32 buffer = std::stoi( params[2] );
            rc = ioctl(fd, command, &buffer);
            break;

        //  struct * timeval {
        //       long tv_sec;    // seconds
        //       long tv_usec;   // microseconds
        //    };
        case SIOCGSTAMP:
            struct timeval buffer;
            double decimal_timestamp, seconds, useconds;

            // Convert PHP real number to timeval
            decimal_timestamp = params[2];
            useconds = std::modf(decimal_timestamp, &seconds) * 1000000;
            buffer.tv_sec = (long)seconds;
            buffer.tv_usec = (long)useconds;

            rc = ioctl(fd, command, &buffer);

            // Convert timeval to PHP number.
            decimal_timestamp = buffer.tv_sec + buffer.tv_usec / 1000000;
            params[2] = decimal_timestamp;
            break;

        case TIOCGPGRP        pid_t *


        case TIOCSPGRP        const pid_t *

        case SIOCSIWCOMMIT      struct iwreq *
        case SIOCGIWNAME        struct iwreq *
        case SIOCSIWNWID        struct iwreq *
        case SIOCGIWNWID        struct iwreq *
        case SIOCSIWFREQ        struct iwreq *
        case SIOCGIWFREQ        struct iwreq *
        case SIOCSIWMODE        struct iwreq *
        case SIOCGIWMODE        struct iwreq *
        case SIOCSIWSENS        struct iwreq *
        case SIOCGIWSENS        struct iwreq *
        case SIOCSIWRANGE       struct iwreq *
        case SIOCGIWRANGE       struct iwreq *
        case SIOCSIWPRIV        struct iwreq *
        case SIOCGIWPRIV        struct iwreq *
        case SIOCSIWSTATS       struct iwreq *
        case SIOCGIWSTATS       struct iwreq *
        case SIOCSIWSPY         struct iwreq *
        case SIOCGIWSPY         struct iwreq *
        case SIOCSIWTHRSPY      struct iwreq *
        case SIOCGIWTHRSPY      struct iwreq *
        case SIOCSIWAP          struct iwreq *
        case SIOCGIWAP          struct iwreq *
        case SIOCGIWAPLIST      struct iwreq *
        case SIOCSIWSCAN        struct iwreq *
        case SIOCGIWSCAN        struct iwreq *
        case SIOCSIWESSID       struct iwreq *
        case SIOCGIWESSID       struct iwreq *
        case SIOCSIWNICKN       struct iwreq *
        case SIOCGIWNICKN       struct iwreq *
        case SIOCSIWRATE        struct iwreq *
        case SIOCGIWRATE        struct iwreq *
        case SIOCSIWRTS         struct iwreq *
        case SIOCGIWRTS         struct iwreq *
        case SIOCSIWFRAG        struct iwreq *
        case SIOCGIWFRAG        struct iwreq *
        case SIOCSIWTXPOW       struct iwreq *
        case SIOCGIWTXPOW       struct iwreq *
        case SIOCSIWRETRY       struct iwreq *
        case SIOCGIWRETRY       struct iwreq *
        case SIOCSIWENCODE      struct iwreq *
        case SIOCGIWENCODE      struct iwreq *
        case SIOCSIWPOWER       struct iwreq *
        case SIOCGIWPOWER       struct iwreq *
        case SIOCSIWGENIE       struct iwreq *
        case SIOCGIWGENIE       struct iwreq *
        case SIOCSIWMLME        struct iwreq *
        case SIOCSIWAUTH        struct iwreq *
        case SIOCGIWAUTH        struct iwreq *
        case SIOCSIWENCODEEXT   struct iwreq *
        case SIOCGIWENCODEEXT   struct iwreq *
        case SIOCSIWPMKSA       struct iwreq *


        case EQL_ENSLAVE       struct ifreq *   // MORE // I-O
        case EQL_EMANCIPATE    struct ifreq *   // MORE // I-O
        case EQL_GETSLAVECFG   struct ifreq *   // MORE // I-O
        case EQL_SETSLAVECFG   struct ifreq *   // MORE // I-O
        case EQL_GETMASTRCFG   struct ifreq *   // MORE // I-O
        case EQL_SETMASTRCFG   struct ifreq *   // MORE // I-O
        case SIOCDEVPLIP   struct ifreq *   // I-O
        case SIOCGIFFLAGS     struct ifreq *           // I-O
        case SIOCGIFADDR      struct ifreq *           // I-O
        case SIOCGIFBRDADDR   struct ifreq *           // I-O
        case SIOCGIFNETMASK   struct ifreq *           // I-O
        case SIOCGIFMEM       struct ifreq *           // I-O
        case SIOCGIFMTU       struct ifreq *           // I-O
        case SIOCGIFMETRIC    struct ifreq *           // I-O
        case OLD_SIOCGIFHWADDR   struct ifreq *          // I-O
        case SIOCGIFHWADDR       struct ifreq *          // I-O
        case SIOCGIFMAP          struct ifreq *          // I-O


        case SIOCSIFHWADDR       const struct ifreq *    // MORE
        case SIOCADDMULTI        const struct ifreq *
        case SIOCDELMULTI        const struct ifreq *
        case SIOCSIFMAP          const struct ifreq *
        case SIOCSIFFLAGS     const struct ifreq *
        case SIOCSIFADDR      const struct ifreq *
        case SIOCSIFDSTADDR   const struct ifreq *
        case SIOCSIFBRDADDR   const struct ifreq *
        case SIOCSIFNETMASK   const struct ifreq *
        case SIOCSIFMETRIC    const struct ifreq *
        case SIOCSIFMEM       const struct ifreq *
        case SIOCSIFMTU       const struct ifreq *

        case UMSDOS_READDIR_DOS   struct umsdos_ioctl *         // I-O
        case UMSDOS_STAT_DOS      struct umsdos_ioctl *         // I-O
        case UMSDOS_READDIR_EMD   struct umsdos_ioctl *
        case UMSDOS_GETVERSION    struct umsdos_ioctl *


        case UMSDOS_UNLINK_DOS    const struct umsdos_ioctl *
        case UMSDOS_RMDIR_DOS     const struct umsdos_ioctl *
        case UMSDOS_CREAT_EMD     const struct umsdos_ioctl *
        case UMSDOS_UNLINK_EMD    const struct umsdos_ioctl *
        case UMSDOS_DOS_SETUP     const struct umsdos_ioctl *
        case UMSDOS_RENAME_DOS    const struct umsdos_ioctl *


        case SIOCGARP            struct arpreq *         // I-O
        case SIOCGRARP           struct arpreq *         // I-O

        case SIOCDARP            const struct arpreq *
        case SIOCSARP            const struct arpreq *
        case SIOCDRARP           const struct arpreq *
        case SIOCSRARP           const struct arpreq *



        case TCGETA           struct termio *

        case TCSETA           const struct termio *
        case TCSETAW          const struct termio *
        case TCSETAF          const struct termio *

        case TCGETS           struct termios *
        case TIOCGLCKTRMIOS   struct termios *

        case TIOCSLCKTRMIOS   const struct termios *
        case TCSETS           const struct termios *
        case TCSETSW          const struct termios *
        case TCSETSF          const struct termios *

        case TIOCGSERIAL      struct serial_struct *

        case TIOCSSERIAL      const struct serial_struct *

        case TIOCTTYGSTRUCT   struct tty_struct *

        case TIOCSERGSTRUCT   struct async_struct *

        case TIOCSERGETMULTI   struct serial_multiport_struct *

        case TIOCSERSETMULTI   const struct serial_multiport_struct *

        case TIOCGWINSZ       struct winsize *

        case TIOCSWINSZ       const struct winsize *

        case SIOCAX25GETPARMS   struct ax25_parms_struct *     // I-O

        case SIOCAX25GETUID     const struct sockaddr_ax25 *
        case SIOCAX25ADDUID     const struct sockaddr_ax25 *
        case SIOCAX25DELUID     const struct sockaddr_ax25 *

        case SIOCAX25SETPARMS   const struct ax25_parms_struct *

        case CDROMPLAYMSF      const struct cdrom_msf *
        case CDROMREADMODE2   const struct cdrom_msf *          // MORE
        case CDROMREADMODE1   const struct cdrom_msf *          // MORE
        case CDROMREADRAW      const struct cdrom_msf *   // MORE
        case CDROMREADCOOKED   const struct cdrom_msf *   // MORE
        case CDROMSEEK         const struct cdrom_msf *

        case CDROMPLAYTRKIND   const struct cdrom_ti *

        case CDROMREADTOCHDR   struct cdrom_tochdr *

        case CDROMREADTOCENTRY   struct cdrom_tocentry *   // I-O

        case CDROMVOLREAD      struct cdrom_volctrl *

        case CDROMVOLCTRL     const struct cdrom_volctrl *

        case CDROMSUBCHNL     struct cdrom_subchnl *            // I-O

        case CDROMREADAUDIO   const struct cdrom_read_audio *   // MORE

        case CDROMMULTISESSION   struct cdrom_multisession *   // I-O

        case CDROM_GET_UPC     struct { char [8]; } *

        case FDGETPRM         struct floppy_struct *

        case FDSETPRM         const struct floppy_struct *
        case FDDEFPRM         const struct floppy_struct *

        case FDFMTTRK         const struct format_descr *

        case FDGETMAXERRS     struct floppy_max_errors *

        case FDSETMAXERRS     const struct floppy_max_errors *

        case FDGETDRVTYP      struct { char [16]; } *

        case FDGETDRVPRM      struct floppy_drive_params *

        case FDSETDRVPRM      const struct floppy_drive_params *

        case FDGETDRVSTAT     struct floppy_drive_struct *
        case FDPOLLDRVSTAT    struct floppy_drive_struct *

        case FDGETFDCSTAT     struct floppy_fdc_state *

        case FDWERRORGET      struct floppy_write_errors *

        case FDRAWCMD    struct floppy_raw_cmd *   // MORE // I-O

        case SIOCGIFCONF      struct ifconf *          //

        case CYGETMON          struct cyclades_monitor *

        case FS_IOC_FIEMAP         struct fiemap *

        case HDIO_GETGEO             struct hd_geometry *

        case HDIO_GET_IDENTITY       struct hd_driveid *

        case PPPIOCGSTAT        struct ppp_stats *

        case PPPIOCGTIME        struct ppp_ddinfo *

        case PPPIOCGXASYNCMAP   struct { int [8]; } *

        case PPPIOCSXASYNCMAP   const struct { int [8]; } *

        case SIOCIPXCFGDATA   struct ipx_config_data *

        case GIO_FONT   struct { char [8192]; } *

        case PIO_FONT   const struct { char [8192]; } *

        case GIO_FONTX  struct console_font_desc *        // MORE // I-O

        case PIO_FONTX  const struct console_font_desc *  //MORE

        case GIO_CMAP   struct { char [48]; } *

        case PIO_CMAP   const struct { char [48]; }

        case GIO_SCRNMAP   struct { char [E_TABSZ]; } *

        case PIO_SCRNMAP      const struct { char [E_TABSZ]; } *

        case PIO_UNISCRNMAP   const struct { short [E_TABSZ]; } *

        case GIO_UNISCRNMAP   struct { short [E_TABSZ]; } *

        case GIO_UNIMAP      struct unimapdesc *         // MORE // I-O

        case PIO_UNIMAP      const struct unimapdesc *   // MORE

        case PIO_UNIMAPCLR   const struct unimapinit *

        case KDGKBENT        struct kbentry *            // I-O

        case KDSKBENT        const struct kbentry *

        case KDGKBSENT       struct kbsentry *           // I-O

        case KDSKBSENT       const struct kbsentry *

        case KDGKBDIACR      struct kbdiacrs *

        case KDSKBDIACR      const struct kbdiacrs *

        case KDGETKEYCODE    struct kbkeycode *          // I-O

        case KDSETKEYCODE    const struct kbkeycode *

        case LPGETSTATS    struct lp_stats *

        case SIOCGETVIFCNT   struct sioc_vif_req *   // I-O

        case SIOCGETSGCNT    struct sioc_sg_req *    // I-O

        case VFAT_IOCTL_READDIR_BOTH    struct dirent [2]
        case VFAT_IOCTL_READDIR_SHORT   struct dirent [2]

        case MTIOCGET         struct mtget *

        case MTIOCPOS         struct mtpos *

        case MTIOCTOP         const struct mtop *

        case MTIOCGETCONFIG   struct mtconfiginfo *

        case MTIOCSETCONFIG   const struct mtconfiginfo *

        case SIOCNRGETPARMS   struct nr_parms_struct *         // I-O

        case SIOCNRSETPARMS   const struct nr_parms_struct *

        case TIOCCHANINI   const struct scc_modem *

        case TIOCGKISS     struct ioctl_command *         // I-O

        case TIOCSKISS     const struct ioctl_command *

        case TIOCSCCSTAT   struct scc_stat *

        case SCSI_IOCTL_GET_IDLUN       struct { int [2]; } *

        case SMB_IOC_GETMOUNTUID   uid_t *

        case SIOCADDRT        const struct rtentry *   // MORE
        case SIOCDELRT        const struct rtentry *   // MORE

        case SNDCTL_SYNTH_INFO        struct synth_info *   // I-O

        case SNDCTL_FM_LOAD_INSTR   const struct sbi_instrument *

        case SNDCTL_MIDI_INFO          struct midi_info *     // I-O

        case SNDCTL_PMGR_ACCESS        struct patmgr_info *   // I-O

        case SNDCTL_SEQ_OUTOFBAND   const struct seq_event_rec *

        case SNDCTL_PMGR_IFACE      struct patmgr_info *   // I-O

        case SNDCTL_MIDI_MPUCMD   struct mpu_command_rec *   // I-O

        case SNDCTL_DSP_GETOSPACE      struct audio_buf_info *
        case SNDCTL_DSP_GETISPACE      struct audio_buf_info *

        case SNDCTL_COPR_LOAD          const struct copr_buffer *

        case SNDCTL_COPR_RDATA   struct copr_debug_buf *   // I-O
        case SNDCTL_COPR_RCODE   struct copr_debug_buf *   // I-O

        case SNDCTL_COPR_WDATA   const struct copr_debug_buf *
        case SNDCTL_COPR_WCODE   const struct copr_debug_buf *

        case SNDCTL_COPR_RUN    struct copr_debug_buf *   // I-O
        case SNDCTL_COPR_HALT   struct copr_debug_buf *   // I-O

        case SNDCTL_COPR_SENDMSG           const struct copr_msg *

        case SNDCTL_COPR_RCVMSG            struct copr_msg *

        case VT_GETMODE       struct vt_mode *

        case VT_SETMODE       const struct vt_mode *

        case VT_GETSTATE      struct vt_stat *

        case VT_RESIZE        const struct vt_sizes *

        case VT_RESIZEX       const struct vt_consize *


        */

        default:
            break;

    }

    // Process return code
    if(rc == -1){
          io_error_string[fd] = "io_ioctl operation failed. "
              + string(strerror(errno))
              + " (" + std::to_string( fd ) + ")"
          ;
    }

    //  ret = std::string(buffer.data(), &buffer[minimum_buffer_size]);

    // a few drivers return values in rc!
    return rc;
}




/*============================================================================*\
  Ioctl

  string ioctl(int $file_descriptor, int $command , int $minimum_buffer_size [, string argument] )

  Controle device sriver parameters.

  This is a dangerous function, as there is no well defined behaviour, and the
  controled device driver might do anything with priviliges.
  Futher more, the buffer used to return parameters from the device driver, has
  no upper limit. If the buffer is overflowing, it will crash the program and
  open your code for security risks.

  When ever posible, use one of the well defined functions for driver controle.

  The returned value is driver specific, OS specific. The byte order and integer
  size is processor specific.

  Use of this function makes your code very hardware and operating system
  dependent.

  If you truely need this function, make shure you know exactly what to expect
  from the driver, and make shure there is no posibility for injection.
  See the man page for ioctl, for a full description of ioctl commands and
  arguments.

  io_ioctl will send the command to the device driver of the open file
  descriptor, and retur a string of $minimum_buffer_size length, containing the
  raw binary reply. Or false on failure.


// Block non-root users from using this port
ioctl(serialFileDescriptor, TIOCEXCL);

// Clear the O_NONBLOCK flag, so that read() will
//   block and wait for data.
fcntl(serialFileDescriptor, F_SETFL, 0);

// Grab the options for the serial port
tcgetattr(serialFileDescriptor, &options);

// Setting raw-mode allows the use of tcsetattr() and ioctl()
cfmakeraw(&options);
\*============================================================================*/

Php::Value io_ioctl_raw(Php::Parameters &params){
    Php::Value ret = false;
    int fd = std::stoi( params[0] );
    int command = std::stoi( params[1] );
    int minimum_buffer_size = std::stoi( params[2] );
    // Be on the somewath safer side: add 1MB to buffer size
    std::vector<char> buffer(minimum_buffer_size + 0x100000);
    int rc;

    if(fd < 1 || fd > MAX_OPEN_FILES -1 ) return ret;

    // Test secure mode

    buffer[0] = 0;
    if(params.size() > 3){
        string argument = params[3];
        strncpy(buffer.data(),argument.c_str(),params[3].length());
        rc = ioctl(fd, command, buffer.data());
    }else
        rc = ioctl(fd, command);

    if(rc  != 0){
        io_error_string[fd] = "io_ioctl_raw operation failed. "
            + string(strerror(errno))
            + " (" + std::to_string( fd ) + ")"
        ;

        // a few drivers return values in rc!
        if(rc != -1)
            ret = rc;
        return ret;
    }

    ret = std::string(buffer.data(), &buffer[minimum_buffer_size]);

    return ret;
}



/*============================================================================*\
  int tcgetattr(int fd, struct termios *termios_p);

  TY stuff

  string read(int $file_descriptor,
\*============================================================================*/
Php::Value io_tcgetattr(Php::Parameters &params){
    Php::Value ret = false;
    int fd = std::stoi( params[0] );
    struct termios serial_settings;

    if(tcgetattr(fd, &serial_settings) != 0){
        io_error_string[fd] = "io_tcgetattr operation failed. "
            + string(strerror(errno))
            + " (" + std::to_string( fd ) + ")"
        ;
        return ret;
    }

    ret = Php::Array();
    ret["c_iflag"]  = (int)serial_settings.c_iflag;
    ret["c_oflag"]  = (int)serial_settings.c_oflag;
    ret["c_cflag"]  = (int)serial_settings.c_cflag;
    ret["c_lflag"]  = (int)serial_settings.c_lflag;
    ret["c_line"]   = serial_settings.c_line;
    ret["c_cc"]     = string(serial_settings.c_cc,serial_settings.c_cc + sizeof(serial_settings.c_cc));
    ret["c_ispeed"] = (int)serial_settings.c_ispeed;
    ret["c_ospeed"] = (int)serial_settings.c_ospeed;
    return ret;
}

/*============================================================================*\
  int tcsetattr(int fd, struct termios *termios_p);


https://gist.github.com/jerome-labidurie/5dde0ec105fe88aaf5fa8f3c54f4b07d

\*============================================================================*/
Php::Value io_tcsetattr(Php::Parameters &params){
    Php::Value ret = false;
    int fd = std::stoi( params[0] );
    struct termios serial_settings;
    const char * str;

    serial_settings.c_iflag  = std::stoi( params[1]["c_iflag"] );
    serial_settings.c_oflag  = std::stoi( params[1]["c_oflag"] );
    serial_settings.c_cflag  = std::stoi( params[1]["c_cflag"] );
    serial_settings.c_lflag  = std::stoi( params[1]["c_lflag"] );
    str = params[1]["c_line"];
    serial_settings.c_line   = str[0];
    str = params[1]["c_cc"];
    strncpy((char *)serial_settings.c_cc,str,sizeof(serial_settings.c_cc));

    serial_settings.c_ispeed = std::stoi( params[1]["c_ispeed"] );
    serial_settings.c_ospeed = std::stoi( params[1]["c_ospeed"] );

serial_settings.c_ospeed = B50;

//    if(tcsetattr(fd, TCSADRAIN, (const termios*) &serial_settings)){
    if(ioctl(fd, TCSETS, (const termios*) &serial_settings)){
        io_error_string[fd] = "io_tcgetattr operation failed. "
            + string(strerror(errno))
            + " (" + std::to_string( fd ) + ")"
        ;
        return ret;
    }

    ret = true;
    return ret;
}

/*============================================================================*\
  set_serial

  array io_set_serial(string $path, string $settings [,number $read_timeout)

  Parameters:
  path: full path to device

  settings: A string containing one or more of the following values and words, separated by space og commas.

    number <speed> Baurate bits pr. second. Only standard values are accepted.
        int he range of 50 - 4000000 See man pages for tty.

    number <bit> number of bits in a charakter. values are 5 - 8

    number <stop bits> values 1 - 2

    all other number values are discarted.

    "even" | "odd" paraty bit.

    "stick" use mark-stick parity.

    "xon" use xon/off software flow controle

    "hwflow"  use hardware flowcontrole with the RTS/CTS lines

    "loop" Let the device perform an internal loop back - if supported.

  read timeout: Number of seconds io_read will wait for a charakter. value range from 0.1 - 25.5 if set to 0 io_read will return immediately, with the available charakters read. If set to -1 io_read will block, until the specified number of charakters are revieved.

  return: false or an associative array containing the values set.

  Values that are not specified are set to defaults:
    speed: 9600
    bits:  8
    stopbits: 1
    parity: off
    sw flow controle: off
    hw flow controle: off
    loop

  example:
    io_set_serial("/dev/tty3","115200 7 2 odd hwflow",0,5)

  If some of the more exotic attributes is needed, please use io_ioclt to set the attributes.
\*============================================================================*/
Php::Value io_set_serial(Php::Parameters &params){
    Php::Value ret = false;
    //string path = params[0];
    int fd = std::stoi( params[0] );
    // struct termios serial_settings;
    string line = params[1];
    unsigned char readTimeout = 0.0;

    vector<string> parameter;
    double number;
    string str;
    map<string,string> serial;
    struct termios tty;
    int modem_control_bits;
    bool no_loopback = false;

    if(params.size() >2)
        readTimeout = (int)(std::stof(params[2]) * 10 ) & 0xff;

    // Read the device settings structure
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0){
        io_error_string[fd] =
             "io_set_serial operation failed to retrieve settings: "
            + string(strerror( errno ))
        ;
        return ret;
    }

    // Preset to usefull defaults

    // RAW sets the following:
    // c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    // c_oflag &= ~OPOST;
    // c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    // c_cflag &= ~(CSIZE | PARENB);
    // c_cflag |= CS8;
    cfmakeraw(&tty);

    // tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // tty.c_iflag &= ~(IGNBRK | ICRNL);  // disable break processing
                                    // no CR NL convertions
    // tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    // tty.c_oflag = 0;                // no remapping, no delays

    // tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// Ignore modem control lines,
                                    // enable reading
    // tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag &= ~CSTOPB;         // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;        // Hw flow controle off

    // Set read to block
    if(readTimeout == 0){
        tty.c_cc[VMIN]  = 1;        // read blocks

    // Set read to timeout (0.1 - 25,5 seconds)
    }else{
        tty.c_cc[VMIN]  = 0;        // read doesn't block
        tty.c_cc[VTIME] = readTimeout;
    }

    // Prepare loop back
    if( ioctl(fd, TIOCMGET, &modem_control_bits) )
        no_loopback = true;
    else
        modem_control_bits &= ~TIOCM_LOOP; // disable loop back mode

    // Split parameters into entities and set device
    Tokenize(line,parameter," ,:;-/+");
    for (vector<string>::iterator it = parameter.begin(); it != parameter.end(); ++it){

        // Process numeric parameters
        number = atoi(it->c_str());
        if(number > 0){

            // Set speed
            if( number > 8 ){
                if( dataSpeed.count( number ) != 1 ){
                    io_error_string[fd] =
                        "io_set_serial operation failed because Baud rate of '"
                        + std::to_string(number)
                        + "'Bit/s is non-standard."
                    ;
                    return ret;
                }

                if ( cfsetispeed(&tty, dataSpeed.at(number)) != 0 ){
                    io_error_string[fd] =
                        + "io_set_serial operation failed to set input baudrate";
                    return ret;

                }
                if ( cfsetospeed(&tty, dataSpeed.at(number)) !=  0){
                    io_error_string[fd] =
                        + "io_set_serial operation failed to set input  baudrate";
                    return ret;

                }

                serial["speed"] = std::to_string((long int)number);

            // Set number of bits pr word (5-8)
            }else if(number > 4){
                tty.c_cflag = (tty.c_cflag & ~CSIZE) | dataBit.at(number);
                serial["bits"] = std::to_string((long int)number);

            // Stop bits
            }else if(number < 3){
                tty.c_cflag |= stopBit.at(number);
                serial["stopBits"] = std::to_string((long int)number);
            }

        // Process alphanumeric parameters
        }else{
            str = *it;
            transform(str.begin(), str.end(), str.begin(), (int (*)(int))tolower);

            // Parity
            if(str[0] == 'e'){
                tty.c_cflag |= PARENB;
                tty.c_cflag &= ~PARODD;
                serial["parity"] = str;

            }else if(str[0] == 'o'){
                tty.c_cflag |= PARENB;
                tty.c_cflag |= PARODD;
                serial["parity"] = str;

            // (not in POSIX) Use "stick" (mark/space) parity (supported on certain serial devices): if PARODD is set, the parity bit is always 1; if PARODD is not set, then the parity bit is always 0).
            }else if(str[0] == 's'){
                tty.c_cflag |= CMSPAR;
                serial["parity"] = str;

            // Soft flow controle Enable XON/XOFF flow control.
            }if(str[0] == 'x'){
                tty.c_iflag |= IXON | IXOFF;
                serial["xon"] = "Xon/off-flow-controle";

            // Hardware flow controle (not in POSIX) Enable RTS/CTS (hardware) flow control.
            }else if(str[0] == 'h'){
                tty.c_cflag |= CRTSCTS;
                serial["hw"] = "Flow-controle";

            // Loop back mode (Ignore error)
            }else if(str[0] == 'l'){
                if( no_loopback )
                    serial["loop"] = "Not supported (TIOCMGET)";
                else
                    modem_control_bits |= TIOCM_LOOP;
            }
        }
        cout << *it << endl;
    }

    // Do the combined setting of attributes
    if (tcsetattr (fd, TCSANOW, &tty) != 0){
        io_error_string[fd] =
             "io_set_serial operation failed to set parameters with TCSANOW "
            + string(strerror( errno ))
        ;
        return ret;
    }

    // Set loop back mode
    if( !no_loopback )
        if( ioctl(fd, TIOCMSET, &modem_control_bits) )
            if( modem_control_bits & TIOCM_LOOP )
                serial["loop"] = "Not supported (TIOCMSET)";

    ret = serial;

    return ret;
}


/*============================================================================*\
Linux uses a dirty method for non-standard baud rates, called "baud rate aliasing". Basically, you tell the serial driver to interpret the value B38400 differently. This is controlled with the ASYNC_SPD_CUST flag in serial_struct member flags.

You need to manually calculate the divisor for the custom speed as follows:

// configure port to use custom speed instead of 38400
ioctl(port, TIOCGSERIAL, &ss);
ss.flags = (ss.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
ss.custom_divisor = (ss.baud_base + (speed / 2)) / speed;
closestSpeed = ss.baud_base / ss.custom_divisor;

if (closestSpeed < speed * 98 / 100 || closestSpeed > speed * 102 / 100) {
    sprintf(stderr, "Cannot set serial port speed to %d. Closest possible is %d\n", speed, closestSpeed));
}

ioctl(port, TIOCSSERIAL, &ss);

cfsetispeed(&tios, B38400);
cfsetospeed(&tios, B38400);














typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

#define NCCS 32
struct termios
  {
    tcflag_t c_iflag;		// input mode flags
    tcflag_t c_oflag;		// output mode flags
    tcflag_t c_cflag;		// control mode flags
    tcflag_t c_lflag;		// local mode flags
    cc_t c_line;			// line discipline
    cc_t c_cc[NCCS];		// control characters
    speed_t c_ispeed;		// input speed
    speed_t c_ospeed;		// output speed
#define _HAVE_STRUCT_TERMIOS_C_ISPEED 1
#define _HAVE_STRUCT_TERMIOS_C_OSPEED 1
  };

int tcgetattr(int fd, struct termios *termios_p);

int tcsetattr(int fd, int optional_actions,
              const struct termios *termios_p);

int tcsendbreak(int fd, int duration);

int tcdrain(int fd);

int tcflush(int fd, int queue_selector);

int tcflow(int fd, int action);

void cfmakeraw(struct termios *termios_p);

speed_t cfgetispeed(const struct termios *termios_p);

speed_t cfgetospeed(const struct termios *termios_p);

int cfsetispeed(struct termios *termios_p, speed_t speed);

int cfsetospeed(struct termios *termios_p, speed_t speed);

int cfsetspeed(struct termios *termios_p, speed_t speed);
\*============================================================================*/

/*


       // <include/asm-i386/socket.h>

       0x00008901   FIOSETOWN    const int *
       0x00008902   SIOCSPGRP    const int *
       0x00008903   FIOGETOWN    int *
       0x00008904   SIOCGPGRP    int *
       0x00008905   SIOCATMAR    int *
       0x00008906   SIOCGSTAMP   timeval *

       // <include/asm-i386/termios.h>

       0x00005401   TCGETS           struct termios *
       0x00005402   TCSETS           const struct termios *
       0x00005403   TCSETSW          const struct termios *
       0x00005404   TCSETSF          const struct termios *
       0x00005405   TCGETA           struct termio *
       0x00005406   TCSETA           const struct termio *
       0x00005407   TCSETAW          const struct termio *
       0x00005408   TCSETAF          const struct termio *
       0x00005409   TCSBRK           int
       0x0000540A   TCXONC           int
       0x0000540B   TCFLSH           int
       0x0000540C   TIOCEXCL         void
       0x0000540D   TIOCNXCL         void
       0x0000540E   TIOCSCTTY        int
       0x0000540F   TIOCGPGRP        pid_t *
       0x00005410   TIOCSPGRP        const pid_t *
       0x00005411   TIOCOUTQ         int *
       0x00005412   TIOCSTI          const char *
       0x00005413   TIOCGWINSZ       struct winsize *
       0x00005414   TIOCSWINSZ       const struct winsize *
       0x00005415   TIOCMGET         int *
       0x00005416   TIOCMBIS         const int *
       0x00005417   TIOCMBIC         const int *
       0x00005418   TIOCMSET         const int *
       0x00005419   TIOCGSOFTCAR     int *
       0x0000541A   TIOCSSOFTCAR     const int *
       0x0000541B   FIONREAD         int *
       0x0000541B   TIOCINQ          int *
       0x0000541C   TIOCLINUX        const char *                   // MORE
       0x0000541D   TIOCCONS         void
       0x0000541E   TIOCGSERIAL      struct serial_struct *
       0x0000541F   TIOCSSERIAL      const struct serial_struct *
       0x00005420   TIOCPKT          const int *
       0x00005421   FIONBIO          const int *
       0x00005422   TIOCNOTTY        void
       0x00005423   TIOCSETD         const int *
       0x00005424   TIOCGETD         int *
       0x00005425   TCSBRKP          int
       0x00005426   TIOCTTYGSTRUCT   struct tty_struct *
       0x00005450   FIONCLEX         void
       0x00005451   FIOCLEX          void
       0x00005452   FIOASYNC         const int *
       0x00005453   TIOCSERCONFIG    void
       0x00005454   TIOCSERGWILD     int *
       0x00005455   TIOCSERSWILD     const int *
       0x00005456   TIOCGLCKTRMIOS   struct termios *
       0x00005457   TIOCSLCKTRMIOS   const struct termios *
       0x00005458   TIOCSERGSTRUCT   struct async_struct *
       0x00005459   TIOCSERGETLSR    int *

       0x0000545A   TIOCSERGETMULTI   struct serial_multiport_struct *
       0x0000545B   TIOCSERSETMULTI   const struct serial_multiport_struct *

       // <include/linux/ax25.h>

       0x000089E0   SIOCAX25GETUID     const struct sockaddr_ax25 *

       0x000089E1   SIOCAX25ADDUID     const struct sockaddr_ax25 *
       0x000089E2   SIOCAX25DELUID     const struct sockaddr_ax25 *
       0x000089E3   SIOCAX25NOUID      const int *
       0x000089E4   SIOCAX25DIGCTL     const int *
       0x000089E5   SIOCAX25GETPARMS   struct ax25_parms_struct *     // I-O

       0x000089E6   SIOCAX25SETPARMS   const struct ax25_parms_struct *

       // <include/linux/cdk.h>

       0x00007314   STL_BINTR    void
       0x00007315   STL_BSTART   void
       0x00007316   STL_BSTOP    void
       0x00007317   STL_BRESET   void

       // <include/linux/cdrom.h>

       0x00005301   CDROMPAUSE        void
       0x00005302   CDROMRESUME       void
       0x00005303   CDROMPLAYMSF      const struct cdrom_msf *
       0x00005304   CDROMPLAYTRKIND   const struct cdrom_ti *
       0x00005305   CDROMREADTOCHDR   struct cdrom_tochdr *

       0x00005306   CDROMREADTOCENTRY   struct cdrom_tocentry *   // I-O

       0x00005307   CDROMSTOP        void
       0x00005308   CDROMSTART       void
       0x00005309   CDROMEJECT       void
       0x0000530A   CDROMVOLCTRL     const struct cdrom_volctrl *
       0x0000530B   CDROMSUBCHNL     struct cdrom_subchnl *            // I-O
       0x0000530C   CDROMREADMODE2   const struct cdrom_msf *          // MORE
       0x0000530D   CDROMREADMODE1   const struct cdrom_msf *          // MORE
       0x0000530E   CDROMREADAUDIO   const struct cdrom_read_audio *   // MORE
       0x0000530F   CDROMEJECT_SW    int

       0x00005310   CDROMMULTISESSION   struct cdrom_multisession *   // I-O

       0x00005311   CDROM_GET_UPC     struct { char [8]; } *
       0x00005312   CDROMRESET        void
       0x00005313   CDROMVOLREAD      struct cdrom_volctrl *
       0x00005314   CDROMREADRAW      const struct cdrom_msf *   // MORE
       0x00005315   CDROMREADCOOKED   const struct cdrom_msf *   // MORE
       0x00005316   CDROMSEEK         const struct cdrom_msf *

       // <include/linux/cm206.h>

       0x00002000   CM206CTL_GET_STAT        int
       0x00002001   CM206CTL_GET_LAST_STAT   int

       // <include/linux/cyclades.h>

       0x00435901   CYGETMON          struct cyclades_monitor *
       0x00435902   CYGETTHRESH       int *
       0x00435903   CYSETTHRESH       int
       0x00435904   CYGETDEFTHRESH    int *
       0x00435905   CYSETDEFTHRESH    int
       0x00435906   CYGETTIMEOUT      int *
       0x00435907   CYSETTIMEOUT      int
       0x00435908   CYGETDEFTIMEOUT   int *
       0x00435909   CYSETDEFTIMEOUT   int

       // <include/linux/fd.h>

       0x00000000   FDCLRPRM         void
       0x00000001   FDSETPRM         const struct floppy_struct *

       0x00000002   FDDEFPRM         const struct floppy_struct *
       0x00000003   FDGETPRM         struct floppy_struct *
       0x00000004   FDMSGON          void
       0x00000005   FDMSGOFF         void
       0x00000006   FDFMTBEG         void
       0x00000007   FDFMTTRK         const struct format_descr *
       0x00000008   FDFMTEND         void
       0x0000000A   FDSETEMSGTRESH   int
       0x0000000B   FDFLUSH          void
       0x0000000C   FDSETMAXERRS     const struct floppy_max_errors *
       0x0000000E   FDGETMAXERRS     struct floppy_max_errors *
       0x00000010   FDGETDRVTYP      struct { char [16]; } *
       0x00000014   FDSETDRVPRM      const struct floppy_drive_params *
       0x00000015   FDGETDRVPRM      struct floppy_drive_params *
       0x00000016   FDGETDRVSTAT     struct floppy_drive_struct *
       0x00000017   FDPOLLDRVSTAT    struct floppy_drive_struct *
       0x00000018   FDRESET          int
       0x00000019   FDGETFDCSTAT     struct floppy_fdc_state *
       0x0000001B   FDWERRORCLR      void
       0x0000001C   FDWERRORGET      struct floppy_write_errors *

       0x0000001E   FDRAWCMD    struct floppy_raw_cmd *   // MORE // I-O
       0x00000028   FDTWADDLE   void

       // <include/linux/fs.h>

       0x0000125D   BLKROSET              const int *
       0x0000125E   BLKROGET              int *
       0x0000125F   BLKRRPART             void
       0x00001260   BLKGETSIZE            unsigned long *
       0x00001261   BLKFLSBUF             void
       0x00001262   BLKRASET              unsigned long
       0x00001263   BLKRAGET              unsigned long *
       0x00000001   FIBMAP                int *             // I-O
       0x00000002   FIGETBSZ              int *
       0x80086601   FS_IOC_GETFLAGS       int *
       0x40086602   FS_IOC_SETFLAGS       int *
       0x80087601   FS_IOC_GETVERSION     int *
       0x40087602   FS_IOC_SETVERSION     int *
       0xC020660B   FS_IOC_FIEMAP         struct fiemap *
       0x40086602   FS_IOC32_SETFLAGS     int *
       0x80047601   FS_IOC32_GETVERSION   int *
       0x40047602   FS_IOC32_SETVERSION   int *

       // <include/linux/hdreg.h>

       0x00000301   HDIO_GETGEO             struct hd_geometry *
       0x00000302   HDIO_GET_UNMASKINTR     int *
       0x00000304   HDIO_GET_MULTCOUNT      int *
       0x00000307   HDIO_GET_IDENTITY       struct hd_driveid *
       0x00000308   HDIO_GET_KEEPSETTINGS   int *
       0x00000309   HDIO_GET_CHIPSET        int *
       0x0000030A   HDIO_GET_NOWERR         int *
       0x0000030B   HDIO_GET_DMA            int *
       0x0000031F   HDIO_DRIVE_CMD          int *                  // I-O
       0x00000321   HDIO_SET_MULTCOUNT      int
       0x00000322   HDIO_SET_UNMASKINTR     int
       0x00000323   HDIO_SET_KEEPSETTINGS   int
       0x00000324   HDIO_SET_CHIPSET        int
       0x00000325   HDIO_SET_NOWERR         int
       0x00000326   HDIO_SET_DMA            int

       // <include/linux/if_eql.h>

       0x000089F0   EQL_ENSLAVE       struct ifreq *   // MORE // I-O
       0x000089F1   EQL_EMANCIPATE    struct ifreq *   // MORE // I-O
       0x000089F2   EQL_GETSLAVECFG   struct ifreq *   // MORE // I-O
       0x000089F3   EQL_SETSLAVECFG   struct ifreq *   // MORE // I-O
       0x000089F4   EQL_GETMASTRCFG   struct ifreq *   // MORE // I-O
       0x000089F5   EQL_SETMASTRCFG   struct ifreq *   // MORE // I-O

       // <include/linux/if_plip.h>

       0x000089F0   SIOCDEVPLIP   struct ifreq *   // I-O

       // <include/linux/if_ppp.h>

       0x00005490   PPPIOCGFLAGS       int *
       0x00005491   PPPIOCSFLAGS       const int *
       0x00005492   PPPIOCGASYNCMAP    int *
       0x00005493   PPPIOCSASYNCMAP    const int *
       0x00005494   PPPIOCGUNIT        int *
       0x00005495   PPPIOCSINPSIG      const int *
       0x00005497   PPPIOCSDEBUG       const int *
       0x00005498   PPPIOCGDEBUG       int *
       0x00005499   PPPIOCGSTAT        struct ppp_stats *
       0x0000549A   PPPIOCGTIME        struct ppp_ddinfo *
       0x0000549B   PPPIOCGXASYNCMAP   struct { int [8]; } *
       0x0000549C   PPPIOCSXASYNCMAP   const struct { int [8]; } *
       0x0000549D   PPPIOCSMRU         const int *
       0x0000549E   PPPIOCRASYNCMAP    const int *
       0x0000549F   PPPIOCSMAXCID      const int *

       // <include/linux/ipx.h>

       0x000089E0   SIOCAIPXITFCRT   const char *
       0x000089E1   SIOCAIPXPRISLT   const char *
       0x000089E2   SIOCIPXCFGDATA   struct ipx_config_data *

       // <include/linux/kd.h>

       0x00004B60   GIO_FONT   struct { char [8192]; } *
       0x00004B61   PIO_FONT   const struct { char [8192]; } *

       0x00004B6B  GIO_FONTX  struct console_font_desc *        // MORE // I-O
       0x00004B6C  PIO_FONTX  const struct console_font_desc *  //MORE

       0x00004B70   GIO_CMAP   struct { char [48]; } *
       0x00004B71   PIO_CMAP   const struct { char [48]; }

       0x00004B2F   KIOCSOUND     int
       0x00004B30   KDMKTONE      int
       0x00004B31   KDGETLED      char *
       0x00004B32   KDSETLED      int
       0x00004B33   KDGKBTYPE     char *
       0x00004B34   KDADDIO       int                            // MORE
       0x00004B35   KDDELIO       int                            // MORE
       0x00004B36   KDENABIO      void                           // MORE
       0x00004B37   KDDISABIO     void                           // MORE
       0x00004B3A   KDSETMODE     int
       0x00004B3B   KDGETMODE     int *
       0x00004B3C   KDMAPDISP     void                           // MORE
       0x00004B3D   KDUNMAPDISP   void                           // MORE
       0x00004B40   GIO_SCRNMAP   struct { char [E_TABSZ]; } *

       0x00004B41   PIO_SCRNMAP      const struct { char [E_TABSZ]; } *
       0x00004B69   GIO_UNISCRNMAP   struct { short [E_TABSZ]; } *
       0x00004B6A   PIO_UNISCRNMAP   const struct { short [E_TABSZ]; } *

       0x00004B66   GIO_UNIMAP      struct unimapdesc *         // MORE // I-O
       0x00004B67   PIO_UNIMAP      const struct unimapdesc *   // MORE
       0x00004B68   PIO_UNIMAPCLR   const struct unimapinit *
       0x00004B44   KDGKBMODE       int *
       0x00004B45   KDSKBMODE       int
       0x00004B62   KDGKBMETA       int *
       0x00004B63   KDSKBMETA       int
       0x00004B64   KDGKBLED        int *
       0x00004B65   KDSKBLED        int
       0x00004B46   KDGKBENT        struct kbentry *            // I-O
       0x00004B47   KDSKBENT        const struct kbentry *
       0x00004B48   KDGKBSENT       struct kbsentry *           // I-O
       0x00004B49   KDSKBSENT       const struct kbsentry *
       0x00004B4A   KDGKBDIACR      struct kbdiacrs *
       0x00004B4B   KDSKBDIACR      const struct kbdiacrs *
       0x00004B4C   KDGETKEYCODE    struct kbkeycode *          // I-O
       0x00004B4D   KDSETKEYCODE    const struct kbkeycode *
       0x00004B4E   KDSIGACCEPT     int

       // <include/linux/lp.h>

       0x00000601   LPCHAR        int
       0x00000602   LPTIME        int
       0x00000604   LPABORT       int
       0x00000605   LPSETIRQ      int
       0x00000606   LPGETIRQ      int *
       0x00000608   LPWAIT        int
       0x00000609   LPCAREFUL     int
       0x0000060A   LPABORTOPEN   int
       0x0000060B   LPGETSTATUS   int *
       0x0000060C   LPRESET       void
       0x0000060D   LPGETSTATS    struct lp_stats *

       // <include/linux/mroute.h>

       0x000089E0   SIOCGETVIFCNT   struct sioc_vif_req *   // I-O
       0x000089E1   SIOCGETSGCNT    struct sioc_sg_req *    // I-O

       // <include/linux/msdos_fs.h> see ioctl_fat(2)

       0x82307201   VFAT_IOCTL_READDIR_BOTH    struct dirent [2]
       0x82307202   VFAT_IOCTL_READDIR_SHORT   struct dirent [2]
       0x80047210   FAT_IOCTL_GET_ATTRIBUTES   __u32 *
       0x40047211   FAT_IOCTL_SET_ATTRIBUTES   const __u32 *
       0x80047213   FAT_IOCTL_GET_VOLUME_ID    __u32 *

       // <include/linux/mtio.h>

       0x40086D01   MTIOCTOP         const struct mtop *
       0x801C6D02   MTIOCGET         struct mtget *
       0x80046D03   MTIOCPOS         struct mtpos *
       0x80206D04   MTIOCGETCONFIG   struct mtconfiginfo *
       0x40206D05   MTIOCSETCONFIG   const struct mtconfiginfo *

       // <include/linux/netrom.h>

       0x000089E0   SIOCNRGETPARMS   struct nr_parms_struct *         // I-O
       0x000089E1   SIOCNRSETPARMS   const struct nr_parms_struct *
       0x000089E2   SIOCNRDECOBS     void
       0x000089E3   SIOCNRRTCTL      const int *

       // <include/uapi/linux/wireless.h>
       // This API is deprecated.
       // It is being replaced by nl80211 and cfg80211.  See
       //
       https://wireless.wiki.kernel.org/en/developers/documentation/nl80211

       x00008b00   SIOCSIWCOMMIT      struct iwreq *
       x00008b01   SIOCGIWNAME        struct iwreq *
       x00008b02   SIOCSIWNWID        struct iwreq *
       x00008b03   SIOCGIWNWID        struct iwreq *
       x00008b04   SIOCSIWFREQ        struct iwreq *
       x00008b05   SIOCGIWFREQ        struct iwreq *
       x00008b06   SIOCSIWMODE        struct iwreq *
       x00008b07   SIOCGIWMODE        struct iwreq *
       x00008b08   SIOCSIWSENS        struct iwreq *
       x00008b09   SIOCGIWSENS        struct iwreq *
       x00008b0a   SIOCSIWRANGE       struct iwreq *
       x00008b0b   SIOCGIWRANGE       struct iwreq *
       x00008b0c   SIOCSIWPRIV        struct iwreq *
       x00008b0d   SIOCGIWPRIV        struct iwreq *
       x00008b0e   SIOCSIWSTATS       struct iwreq *
       x00008b0f   SIOCGIWSTATS       struct iwreq *
       x00008b10   SIOCSIWSPY         struct iwreq *
       x00008b11   SIOCGIWSPY         struct iwreq *
       x00008b12   SIOCSIWTHRSPY      struct iwreq *
       x00008b13   SIOCGIWTHRSPY      struct iwreq *
       x00008b14   SIOCSIWAP          struct iwreq *
       x00008b15   SIOCGIWAP          struct iwreq *
       x00008b17   SIOCGIWAPLIST      struct iwreq *
       x00008b18   SIOCSIWSCAN        struct iwreq *
       x00008b19   SIOCGIWSCAN        struct iwreq *
       x00008b1a   SIOCSIWESSID       struct iwreq *
       x00008b1b   SIOCGIWESSID       struct iwreq *
       x00008b1c   SIOCSIWNICKN       struct iwreq *
       x00008b1d   SIOCGIWNICKN       struct iwreq *
       x00008b20   SIOCSIWRATE        struct iwreq *
       x00008b21   SIOCGIWRATE        struct iwreq *
       x00008b22   SIOCSIWRTS         struct iwreq *
       x00008b23   SIOCGIWRTS         struct iwreq *
       x00008b24   SIOCSIWFRAG        struct iwreq *
       x00008b25   SIOCGIWFRAG        struct iwreq *
       x00008b26   SIOCSIWTXPOW       struct iwreq *
       x00008b27   SIOCGIWTXPOW       struct iwreq *
       x00008b28   SIOCSIWRETRY       struct iwreq *
       x00008b29   SIOCGIWRETRY       struct iwreq *
       x00008b2a   SIOCSIWENCODE      struct iwreq *
       x00008b2b   SIOCGIWENCODE      struct iwreq *
       x00008b2c   SIOCSIWPOWER       struct iwreq *
       x00008b2d   SIOCGIWPOWER       struct iwreq *
       x00008b30   SIOCSIWGENIE       struct iwreq *
       x00008b31   SIOCGIWGENIE       struct iwreq *
       x00008b16   SIOCSIWMLME        struct iwreq *
       x00008b32   SIOCSIWAUTH        struct iwreq *
       x00008b33   SIOCGIWAUTH        struct iwreq *
       x00008b34   SIOCSIWENCODEEXT   struct iwreq *
       x00008b35   SIOCGIWENCODEEXT   struct iwreq *
       x00008b36   SIOCSIWPMKSA       struct iwreq *

       // <include/linux/sbpcd.h>

       0x00009000   DDIOCSDBG          const int *
       0x00005382   CDROMAUDIOBUFSIZ   int

       // <include/linux/scc.h>

       0x00005470   TIOCSCCINI    void
       0x00005471   TIOCCHANINI   const struct scc_modem *
       0x00005472   TIOCGKISS     struct ioctl_command *         // I-O
       0x00005473   TIOCSKISS     const struct ioctl_command *
       0x00005474   TIOCSCCSTAT   struct scc_stat *

       // <include/linux/scsi.h>

       0x00005382   SCSI_IOCTL_GET_IDLUN       struct { int [2]; } *
       0x00005383   SCSI_IOCTL_TAGGED_ENABLE   void
       0x00005384   SCSI_IOCTL_TAGGED_DISABLE  void

       0x00005385   SCSI_IOCTL_PROBE_HOST   const int *   // MORE

       // <include/linux/smb_fs.h>

       0x80027501   SMB_IOC_GETMOUNTUID   uid_t *

       // <include/uapi/linux/sockios.h> see netdevice(7)

       0x0000890B   SIOCADDRT        const struct rtentry *   // MORE
       0x0000890C   SIOCDELRT        const struct rtentry *   // MORE
       0x00008910   SIOCGIFNAME      char []
       0x00008911   SIOCSIFLINK      void
       0x00008912   SIOCGIFCONF      struct ifconf *          // MORE // I-O
       0x00008913   SIOCGIFFLAGS     struct ifreq *           // I-O
       0x00008914   SIOCSIFFLAGS     const struct ifreq *
       0x00008915   SIOCGIFADDR      struct ifreq *           // I-O
       0x00008916   SIOCSIFADDR      const struct ifreq *
       0x00008917   SIOCGIFDSTADDR   struct ifreq *           // I-O
       0x00008918   SIOCSIFDSTADDR   const struct ifreq *
       0x00008919   SIOCGIFBRDADDR   struct ifreq *           // I-O
       0x0000891A   SIOCSIFBRDADDR   const struct ifreq *
       0x0000891B   SIOCGIFNETMASK   struct ifreq *           // I-O
       0x0000891C   SIOCSIFNETMASK   const struct ifreq *
       0x0000891D   SIOCGIFMETRIC    struct ifreq *           // I-O
       0x0000891E   SIOCSIFMETRIC    const struct ifreq *
       0x0000891F   SIOCGIFMEM       struct ifreq *           // I-O
       0x00008920   SIOCSIFMEM       const struct ifreq *
       0x00008921   SIOCGIFMTU       struct ifreq *           // I-O
       0x00008922   SIOCSIFMTU       const struct ifreq *

       0x00008923   OLD_SIOCGIFHWADDR   struct ifreq *          // I-O
       0x00008924   SIOCSIFHWADDR       const struct ifreq *    // MORE
       0x00008925   SIOCGIFENCAP        int *
       0x00008926   SIOCSIFENCAP        const int *
       0x00008927   SIOCGIFHWADDR       struct ifreq *          // I-O
       0x00008929   SIOCGIFSLAVE        void
       0x00008930   SIOCSIFSLAVE        void
       0x00008931   SIOCADDMULTI        const struct ifreq *
       0x00008932   SIOCDELMULTI        const struct ifreq *
       0x00008940   SIOCADDRTOLD        void
       0x00008941   SIOCDELRTOLD        void
       0x00008950   SIOCDARP            const struct arpreq *
       0x00008951   SIOCGARP            struct arpreq *         // I-O
       0x00008952   SIOCSARP            const struct arpreq *
       0x00008960   SIOCDRARP           const struct arpreq *
       0x00008961   SIOCGRARP           struct arpreq *         // I-O
       0x00008962   SIOCSRARP           const struct arpreq *
       0x00008970   SIOCGIFMAP          struct ifreq *          // I-O
       0x00008971   SIOCSIFMAP          const struct ifreq *

       // <include/linux/soundcard.h>

       0x00005100   SNDCTL_SEQ_RESET   void
       0x00005101   SNDCTL_SEQ_SYNC    void

       0xC08C5102   SNDCTL_SYNTH_INFO        struct synth_info *   // I-O
       0xC0045103   SNDCTL_SEQ_CTRLRATE      int *                 // I-O
       0x80045104   SNDCTL_SEQ_GETOUTCOUNT   int *
       0x80045105   SNDCTL_SEQ_GETINCOUNT    int *

       0x40045106   SNDCTL_SEQ_PERCMODE      void

       0x40285107   SNDCTL_FM_LOAD_INSTR   const struct sbi_instrument *

       0x40045108   SNDCTL_SEQ_TESTMIDI       const int *
       0x40045109   SNDCTL_SEQ_RESETSAMPLES   const int *
       0x8004510A   SNDCTL_SEQ_NRSYNTHS       int *
       0x8004510B   SNDCTL_SEQ_NRMIDIS        int *
       0xC074510C   SNDCTL_MIDI_INFO          struct midi_info *     // I-O
       0x4004510D   SNDCTL_SEQ_THRESHOLD      const int *
       0xC004510E   SNDCTL_SYNTH_MEMAVL       int *                  // I-O
       0x4004510F   SNDCTL_FM_4OP_ENABLE      const int *
       0xCFB85110   SNDCTL_PMGR_ACCESS        struct patmgr_info *   // I-O
       0x00005111   SNDCTL_SEQ_PANIC          void

       0x40085112   SNDCTL_SEQ_OUTOFBAND   const struct seq_event_rec *

       0xC0045401   SNDCTL_TMR_TIMEBASE    int *                  // I-O
       0x00005402   SNDCTL_TMR_START       void
       0x00005403   SNDCTL_TMR_STOP        void
       0x00005404   SNDCTL_TMR_CONTINUE    void
       0xC0045405   SNDCTL_TMR_TEMPO       int *                  // I-O
       0xC0045406   SNDCTL_TMR_SOURCE      int *                  // I-O
       0x40045407   SNDCTL_TMR_METRONOME   const int *
       0x40045408   SNDCTL_TMR_SELECT      int *                  // I-O
       0xCFB85001   SNDCTL_PMGR_IFACE      struct patmgr_info *   // I-O
       0xC0046D00   SNDCTL_MIDI_PRETIME    int *                  // I-O
       0xC0046D01   SNDCTL_MIDI_MPUMODE    const int *

       0xC0216D02   SNDCTL_MIDI_MPUCMD   struct mpu_command_rec *   // I-O

       0x00005000   SNDCTL_DSP_RESET           void
       0x00005001   SNDCTL_DSP_SYNC            void
       0xC0045002   SNDCTL_DSP_SPEED           int *   // I-O
       0xC0045003   SNDCTL_DSP_STEREO          int *   // I-O
       0xC0045004   SNDCTL_DSP_GETBLKSIZE      int *   // I-O
       0xC0045006   SOUND_PCM_WRITE_CHANNELS   int *   // I-O
       0xC0045007   SOUND_PCM_WRITE_FILTER     int *   // I-O
       0x00005008   SNDCTL_DSP_POST            void
       0xC0045009   SNDCTL_DSP_SUBDIVIDE       int *   // I-O
       0xC004500A   SNDCTL_DSP_SETFRAGMENT     int *   // I-O
       0x8004500B   SNDCTL_DSP_GETFMTS         int *
       0xC0045005   SNDCTL_DSP_SETFMT          int *   // I-O

       0x800C500C   SNDCTL_DSP_GETOSPACE      struct audio_buf_info *
       0x800C500D   SNDCTL_DSP_GETISPACE      struct audio_buf_info *
       0x0000500E   SNDCTL_DSP_NONBLOCK       void
       0x80045002   SOUND_PCM_READ_RATE       int *
       0x80045006   SOUND_PCM_READ_CHANNELS   int *
       0x80045005   SOUND_PCM_READ_BITS       int *
       0x80045007   SOUND_PCM_READ_FILTER     int *
       0x00004300   SNDCTL_COPR_RESET         void
       0xCFB04301   SNDCTL_COPR_LOAD          const struct copr_buffer *

       0xC0144302   SNDCTL_COPR_RDATA   struct copr_debug_buf *   // I-O
       0xC0144303   SNDCTL_COPR_RCODE   struct copr_debug_buf *   // I-O

       0x40144304   SNDCTL_COPR_WDATA   const struct copr_debug_buf *
       0x40144305   SNDCTL_COPR_WCODE   const struct copr_debug_buf *

       0xC0144306   SNDCTL_COPR_RUN    struct copr_debug_buf *   // I-O
       0xC0144307   SNDCTL_COPR_HALT   struct copr_debug_buf *   // I-O

       0x4FA44308   SNDCTL_COPR_SENDMSG           const struct copr_msg *
       0x8FA44309   SNDCTL_COPR_RCVMSG            struct copr_msg *

       0x80044D00   SOUND_MIXER_READ_VOLUME       int *
       0x80044D01   SOUND_MIXER_READ_BASS         int *
       0x80044D02   SOUND_MIXER_READ_TREBLE       int *
       0x80044D03   SOUND_MIXER_READ_SYNTH        int *
       0x80044D04   SOUND_MIXER_READ_PCM          int *
       0x80044D05   SOUND_MIXER_READ_SPEAKER      int *
       0x80044D06   SOUND_MIXER_READ_LINE         int *
       0x80044D07   SOUND_MIXER_READ_MIC          int *
       0x80044D08   SOUND_MIXER_READ_CD           int *
       0x80044D09   SOUND_MIXER_READ_IMIX         int *
       0x80044D0A   SOUND_MIXER_READ_ALTPCM       int *
       0x80044D0B   SOUND_MIXER_READ_RECLEV       int *
       0x80044D0C   SOUND_MIXER_READ_IGAIN        int *
       0x80044D0D   SOUND_MIXER_READ_OGAIN        int *
       0x80044D0E   SOUND_MIXER_READ_LINE1        int *
       0x80044D0F   SOUND_MIXER_READ_LINE2        int *
       0x80044D10   SOUND_MIXER_READ_LINE3        int *
       0x80044D1C   SOUND_MIXER_READ_MUTE         int *
       0x80044D1D   SOUND_MIXER_READ_ENHANCE      int *
       0x80044D1E   SOUND_MIXER_READ_LOUD         int *
       0x80044DFF   SOUND_MIXER_READ_RECSRC       int *
       0x80044DFE   SOUND_MIXER_READ_DEVMASK      int *
       0x80044DFD   SOUND_MIXER_READ_RECMASK      int *
       0x80044DFB   SOUND_MIXER_READ_STEREODEVS   int *
       0x80044DFC   SOUND_MIXER_READ_CAPS         int *

       0xC0044D00   SOUND_MIXER_WRITE_VOLUME    int *   // I-O
       0xC0044D01   SOUND_MIXER_WRITE_BASS      int *   // I-O
       0xC0044D02   SOUND_MIXER_WRITE_TREBLE    int *   // I-O
       0xC0044D03   SOUND_MIXER_WRITE_SYNTH     int *   // I-O
       0xC0044D04   SOUND_MIXER_WRITE_PCM       int *   // I-O
       0xC0044D05   SOUND_MIXER_WRITE_SPEAKER   int *   // I-O
       0xC0044D06   SOUND_MIXER_WRITE_LINE      int *   // I-O
       0xC0044D07   SOUND_MIXER_WRITE_MIC       int *   // I-O
       0xC0044D08   SOUND_MIXER_WRITE_CD        int *   // I-O
       0xC0044D09   SOUND_MIXER_WRITE_IMIX      int *   // I-O
       0xC0044D0A   SOUND_MIXER_WRITE_ALTPCM    int *   // I-O
       0xC0044D0B   SOUND_MIXER_WRITE_RECLEV    int *   // I-O
       0xC0044D0C   SOUND_MIXER_WRITE_IGAIN     int *   // I-O
       0xC0044D0D   SOUND_MIXER_WRITE_OGAIN     int *   // I-O
       0xC0044D0E   SOUND_MIXER_WRITE_LINE1     int *   // I-O
       0xC0044D0F   SOUND_MIXER_WRITE_LINE2     int *   // I-O
       0xC0044D10   SOUND_MIXER_WRITE_LINE3     int *   // I-O
       0xC0044D1C   SOUND_MIXER_WRITE_MUTE      int *   // I-O
       0xC0044D1D   SOUND_MIXER_WRITE_ENHANCE   int *   // I-O
       0xC0044D1E   SOUND_MIXER_WRITE_LOUD      int *   // I-O
       0xC0044DFF   SOUND_MIXER_WRITE_RECSRC    int *   // I-O

       // <include/linux/timerfd.h> see timerfd_create(2)

       0x40085400   TFD_IOC_SET_TICKS   uint64_t *

       // <include/linux/umsdos_fs.h>

       0x000004D2   UMSDOS_READDIR_DOS   struct umsdos_ioctl *         // I-O
       0x000004D3   UMSDOS_UNLINK_DOS    const struct umsdos_ioctl *
       0x000004D4   UMSDOS_RMDIR_DOS     const struct umsdos_ioctl *
       0x000004D5   UMSDOS_STAT_DOS      struct umsdos_ioctl *         // I-O
       0x000004D6   UMSDOS_CREAT_EMD     const struct umsdos_ioctl *
       0x000004D7   UMSDOS_UNLINK_EMD    const struct umsdos_ioctl *
       0x000004D8   UMSDOS_READDIR_EMD   struct umsdos_ioctl *         // I-O
       0x000004D9   UMSDOS_GETVERSION    struct umsdos_ioctl *
       0x000004DA   UMSDOS_INIT_EMD      void
       0x000004DB   UMSDOS_DOS_SETUP     const struct umsdos_ioctl *
       0x000004DC   UMSDOS_RENAME_DOS    const struct umsdos_ioctl *

       // <include/linux/vt.h>

       0x00005600   VT_OPENQRY       int *
       0x00005601   VT_GETMODE       struct vt_mode *
       0x00005602   VT_SETMODE       const struct vt_mode *
       0x00005603   VT_GETSTATE      struct vt_stat *
       0x00005604   VT_SENDSIG       void
       0x00005605   VT_RELDISP       int
       0x00005606   VT_ACTIVATE      int
       0x00005607   VT_WAITACTIVE    int
       0x00005608   VT_DISALLOCATE   int
       0x00005609   VT_RESIZE        const struct vt_sizes *
       0x0000560A   VT_RESIZEX       const struct vt_consize *

       // More arguments.  Some ioctl's take a pointer to a structure which
       contains additional pointers.  These are documented here in
       alphabetical order.
*/
