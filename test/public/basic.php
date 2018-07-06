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
function validate($command,$test =""){
    global $test_result;

    echo "$command: ";

    try{
        $ret = eval("return " . $command . ";");
    } catch (Exception $e){
        $ret = $e->getMessage();
    }

    if( !empty($test)){
        echo " Test: ";
        if(is_scalar($ret))
            eval("return \"$test \";");
        else
            echo $test;

        if(eval("return $test;")){
            echo "<span style=\"color:green\">OK: </span>\n";
            $test_result['ok']++;
        }else{
            echo "<span style=\"color:red\">Failed: ($test) </span>\n";
            $test_result['failed']++;
        }
    }
    var_dump($ret);
    //echo "\n";

    return $ret;
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

// Error handler, passes flow over the exception logger with new ErrorException.

function error_handler( $num, $str, $file, $line, $context = null ){
    exception_handler( new ErrorException( $str, 0, $num, $file, $line ) );
}

// Uncaught exception handler.
function exception_handler( $exception )
{
    print "<div style='color:red;'>";
    print "Program failed with '" . get_class( $exception ) . "'</br>";
    print "Message: {$exception->getMessage()}<br>";
    print "File: {$exception->getFile()}<br>";
    print "Line: {$exception->getLine()}<br>";
    print "</div>";
    exit();
}

// Checks for a fatal error, work around for set_error_handler not working on fatal errors.
function shutdown_check_for_fatal(){
    $error = error_get_last();
    if ( $error["type"] == E_ERROR )
        log_error( $error["type"], $error["message"], $error["file"], $error["line"] );
}

register_shutdown_function( "shutdown_check_for_fatal" );
set_error_handler( "error_handler" );
set_exception_handler( "exception_handler" );
ini_set( "display_errors", "on" );
error_reporting( E_ALL );

$nl = str_repeat("=",80) . "\n";
Echo "{$nl}Test flag string to bin convertion:\n";
$flags = ["w","w+","r","r+","a","a+","c","c+","x","x+","w+a","a","wn","w+s","d"];
foreach($flags as $flag)
  validate("dechex(intval(io_test(\"$flag\")))",'$ret >= 0');

echo "{$nl}Test basic open, write, read and close\n";
$test_file = "test.txt";
$content = "This is Some random text string";
$fd = validate("io_open(\"$test_file\",\"w+s\")",'$ret >= 0');
if($fd < 1) exit;
validate("io_write($fd,\"$content\")",'$ret == ' .strlen($content));
validate("io_close($fd)",'$ret == 0');

echo "{$nl}Test reading:\n";
$fd = io_open($test_file,"r");
validate("io_read($fd, ". strlen($content) .")",'strlen($ret) == '. strlen($content));
io_close($fd);

echo "{$nl}Test read to end char:\n";
$fd = io_open($test_file,"r+");
validate("io_read($fd, ". strlen($content) .",'x')",'strlen($ret) == ' . (strpos($content,"x")+1) );
io_close($fd);
unlink($test_file);

// Serial
echo "{$nl}Test Serial settings:\n";

// Select a real serial devide
$device_list = glob("/dev/serial/by-path/*");
$serial_device = @reset($device_list);
if( empty($serial_device) )
    echo "NO REAL DEVICE FOUND! tests skiped\n";

if( !empty($serial_device) ){
    $flags = "d";
    $settings="38400,8,1,Even";
    $fd = validate("io_open('$serial_device','$flags')", '$ret >= 0');

    validate("io_set_serial($fd,'$settings')",'is_array($ret)');
    $settings="5";
    validate("io_set_serial($fd,'$settings')",'is_array($ret)');
    $settings="6";
    validate("io_set_serial($fd,'$settings')",'is_array($ret)');
    $settings="7";
    validate("io_set_serial($fd,'$settings')",'is_array($ret)');
    $settings="Even";
    validate("io_set_serial($fd,'$settings')",'is_array($ret)');
    $settings="Odd";
    validate("io_set_serial($fd,'$settings')",'is_array($ret)');
    $settings="mark";
    validate("io_set_serial($fd,'$settings')",'is_array($ret)');
    $settings="space";
    validate("io_set_serial($fd,'$settings')",'is_array($ret)');
    $settings="2";
    validate("io_set_serial($fd,'$settings')",'is_array($ret)');
    $settings="4000000";
    validate("io_set_serial($fd,'$settings')",'is_array($ret)');
    $settings="50";
    validate("io_set_serial($fd,'$settings')",'is_array($ret)');

    io_close($fd);

}

echo "{$nl}Test bad Serial settings:\n";

$test_file ="/dev/tty1";
$flags = "d";
$settings="38400,8,1,Even";
$fd = validate("io_open(\"$test_file\",\"$flags\")",'$ret >= 0');

$settings="38400,8,1,o";
validate("io_set_serial($fd,'$settings')", '!is_array($ret)');
io_close($fd);

if( !empty($serial_device) ){
    $flags = "d";
    $fd = io_open($serial_device,$flags);

    $settings="38400,8,1,plot";
    validate("io_set_serial($fd,'$settings')", '!is_array($ret)');

    $settings="40";
    validate("io_set_serial($fd,'$settings')", '!is_array($ret)');

    $settings="3";
//    validate("io_set_serial($fd,'$settings')", '!is_array($ret)');

    echo "{$nl}Test other serial tty functions\n";
    $res = validate("io_tcgetattr($fd)",'is_array($ret)');
    echo "C_CC string: ";
    for ($i = 0; $i < 32; $i++)
      printf(" %02X",ord($res['c_cc'][$i]));
    echo "\n";

    //validate("io_tcsetattr($fd,[ 'c_line' => 'D', 'c_ispeed' => 17 ])",'$ret == 0');

    io_close($fd);
}



echo "{$nl}Ioctl:\n";

/*
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
  };
*/

if( !empty($serial_device) ){
    $flags = "d";
    $fd = io_open($serial_device,$flags);

    echo "{$nl}Test ioctl TIOCMGET:\n";
    // TIOCMGET	0x5415
    $rc = validate("io_ioctl($fd, 0x5415,0)");
    echo "Returned: ", dechex($rc),"h\n";

    io_close($fd);
    echo "\n";
}


//====================================================================
// Results
printf("{$nl}Tests: %d: <span style=\"color:green\">ok: %d </span>",$test_result['ok'] + $test_result['failed'], $test_result['ok'] );
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
