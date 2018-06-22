<?php
/*============================================================================*\
  test basic device access and driver controle*termios_p);
// serial class using exec stty call:
// https://www.phpclasses.org/browse/file/17926.html

// stackoverflow links:
// https://stackoverflow.com/questions/627965/serial-comm-with-php-on-windows
// https://stackoverflow.com/questions/13114275/php-serial-port-data-return-from-arduino

// list : sudo cat /proc/tty/driver/serial

\*============================================================================*/

$test_result['ok'] = 0;
$test_result['failed'] = 0;

function present($name,$result,$fd=0){
    global $test_result;

    echo "$name: ";
    var_dump($result);
    if($fd && (!$result || $result <0)){
        echo "<span style=\"color:red\">Failed:" . io_error($fd) ."</span>\n";
        $test_result['failed']++;
    }else
        $test_result['ok']++;
    echo "\n";
}

function strToHex($string){
    $hex = '';
    for ($i=0; $i<strlen($string); $i++){
        $ord = ord($string[$i]);
        $hexCode = dechex($ord);
        $hex .= " ".substr('0'.$hexCode, -2);
    }
    return strToUpper($hex);
}

Echo "Test flag string to bin convertion:\n";
$flags = ["w","w+","r","r+","a","a+","c","c+","x","x+","w+a","a","wn","w+s","d"];
foreach($flags as $flag)
  present("Flag: $flag = ", dechex(intval(io_test($flag))));

echo "Test basic open, write, read and close";
$test_file = "test.txt";
$content = "This is Some random text string";
present("io_open(\"$test_file\",\"w+s\")", $fd = io_open($test_file,"w+s"), $fd);
if($fd < 1) exit;
present("io_write($fd,\"$content\")",io_write($fd, $content),$fd);
present("io_close($fd)",io_close($fd) == 0,$fd);

$flags = "r+"; present("io_open(\"$test_file\",\"$flags\")", $fd = io_open($test_file,$flags),$fd);
present("io_read($fd, ". strlen($content) .")",$res = io_read($fd, strlen($content)),$fd);
io_close($fd);
unlink($test_file);

// Serial
$test_file ="/dev/tty1";
$flags = "w+";
present("io_open(\"$test_file\",\"$flags\")", $fd = io_open($test_file,$flags),$fd);

present("io_tcgetattr($fd)",$res = io_tcgetattr($fd) ,$fd);
echo "C_CC string: ";
for ($i = 0; $i < 32; $i++)
  printf(" %02X",ord($res['c_cc'][$i]));
echo "\n";

$res['c_line'] = "D";
$res['c_ispeed'] =17;

present("io_tcsetattr($fd)",io_tcsetattr($fd,$res) ,$fd);
present("Result reread:",$res = io_tcgetattr($fd) ,$fd);
echo "C_CC string: ";
for ($i = 0; $i < 32; $i++)
  printf(" %02X",ord($res['c_cc'][$i]));
echo "\n";
/*

echo "test ioctl";
// #define TIOCGSERIAL	0x541E
ypedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

#define NCCS 32
struct termios
  {
    tcflag_t c_iflag;		// input mode flags
    tcflag_t c_oflag;		// output mode flags
    tcflag_t c_cflag;		// control mode flags
    tcflag_t c_lflag;		// local mode flags
    cc_t c_line;			  // line discipline
    cc_t c_cc[NCCS];		// control characters
    speed_t c_ispeed;		// input speed
    speed_t c_ospeed;		// output speed
#define _HAVE_STRUCT_TERMIOS_C_ISPEED 1
#define _HAVE_STRUCT_TERMIOS_C_OSPEED 1
  };
*/
/*
present("io_ioctl($fd, 0x5425,50)", $res = io_ioctl($fd, 0x541E,50) == 0);
echo "Length=",strlen($res),"\n";
for ($i = 0; $i < 50; $i++){
 echo " " . dechex(ord($res[$i]));
}
echo "\n";

io_close($fd);

$path = "/dev/tty1";
$settings = "115200 8,1-xon;hnd loop";
present("io_set_serial($path,$settings)",io_set_serial($path,$settings));

present("io_tcgetattr($fd)",$res = io_tcgetattr($fd) ,$fd);
for ($i = 0; $i < 32; $i++)
  printf(" %02X",ord($res['c_cc'][$i]));
echo "\n";

*/


echo str_repeat("=", 80);
printf("\nTests: %d: <span style=\"color:green\">ok: %d </span>",$test_result['ok'] + $test_result['failed'], $test_result['ok'] );
if($test_result['failed'] > 0 )
    printf("<span style=\"color:red\">failed: %d</span>\n",$test_result['failed']);
echo "\n";

exit;

/*
#define TCGETS		0x5401
#define TCSETS		0x5402
#define TCSETSW		0x5403
#define TCSETSF		0x5404
#define TCGETA		0x5405
#define TCSETA		0x5406
#define TCSETAW		0x5407
#define TCSETAF		0x5408
#define TCSBRK		0x5409
#define TCXONC		0x540A
#define TCFLSH		0x540B
#define TIOCEXCL	0x540C
#define TIOCNXCL	0x540D
#define TIOCSCTTY	0x540E
#define TIOCGPGRP	0x540F
#define TIOCSPGRP	0x5410
#define TIOCOUTQ	0x5411
#define TIOCSTI		0x5412
#define TIOCGWINSZ	0x5413
#define TIOCSWINSZ	0x5414
#define TIOCMGET	0x5415
#define TIOCMBIS	0x5416
#define TIOCMBIC	0x5417
#define TIOCMSET	0x5418
#define TIOCGSOFTCAR	0x5419
#define TIOCSSOFTCAR	0x541A
#define FIONREAD	0x541B
#define TIOCINQ		FIONREAD
#define TIOCLINUX	0x541C
#define TIOCCONS	0x541D
#define TIOCGSERIAL	0x541E
#define TIOCSSERIAL	0x541F
#define TIOCPKT		0x5420
#define FIONBIO		0x5421
#define TIOCNOTTY	0x5422
#define TIOCSETD	0x5423
#define TIOCGETD	0x5424
#define TCSBRKP		0x5425	// Needed for POSIX tcsendbreak()
#define TIOCSBRK	0x5427  // BSD compatibility
#define TIOCCBRK	0x5428  // BSD compatibility
#define TIOCGSID	0x5429  // Return the session ID of FD
#define TCGETS2		_IOR('T', 0x2A, struct termios2)
#define TCSETS2		_IOW('T', 0x2B, struct termios2)
#define TCSETSW2	_IOW('T', 0x2C, struct termios2)
#define TCSETSF2	_IOW('T', 0x2D, struct termios2)
#define TIOCGRS485	0x542E
#ifndef TIOCSRS485
#define TIOCSRS485	0x542F
#endif
#define TIOCGPTN	_IOR('T', 0x30, unsigned int) // Get Pty Number (of pty-mux device)
#define TIOCSPTLCK	_IOW('T', 0x31, int)  // Lock/unlock Pty
#define TIOCGDEV	_IOR('T', 0x32, unsigned int) // Get primary device node of /dev/console
#define TCGETX		0x5432 // SYS5 TCGETX compatibility
#define TCSETX		0x5433
#define TCSETXF		0x5434
#define TCSETXW		0x5435
#define TIOCSIG		_IOW('T', 0x36, int)  // pty: generate signal
#define TIOCVHANGUP	0x5437
#define TIOCGPKT	_IOR('T', 0x38, int) // Get packet mode state
#define TIOCGPTLCK	_IOR('T', 0x39, int) // Get Pty lock state
#define TIOCGEXCL	_IOR('T', 0x40, int) // Get exclusive mode state
#define TIOCGPTPEER	_IO('T', 0x41) // Safely open the slave

#define FIONCLEX	0x5450
#define FIOCLEX		0x5451
#define FIOASYNC	0x5452
#define TIOCSERCONFIG	0x5453
#define TIOCSERGWILD	0x5454
#define TIOCSERSWILD	0x5455
#define TIOCGLCKTRMIOS	0x5456
#define TIOCSLCKTRMIOS	0x5457
#define TIOCSERGSTRUCT	0x5458 // For debugging only
#define TIOCSERGETLSR   0x5459 // Get line status register
#define TIOCSERGETMULTI 0x545A // Get multiport config
#define TIOCSERSETMULTI 0x545B // Set multiport config

#define TIOCMIWAIT	0x545C	// wait for a change on serial input line(s)
#define TIOCGICOUNT	0x545D	// read serial port __inline__ interrupt counts

//
// Some arches already define FIOQSIZE due to a historical
// conflict with a Hayes modem-specific ioctl value.

#ifndef FIOQSIZE
# define FIOQSIZE	0x5460
#endif

// Used for packet mode */
#define TIOCPKT_DATA		 0
#define TIOCPKT_FLUSHREAD	 1
#define TIOCPKT_FLUSHWRITE	 2
#define TIOCPKT_STOP		 4
#define TIOCPKT_START		 8
#define TIOCPKT_NOSTOP		16
#define TIOCPKT_DOSTOP		32
#define TIOCPKT_IOCTL		64

#define TIOCSER_TEMT	0x01	// Transmitter physically empty


?>
