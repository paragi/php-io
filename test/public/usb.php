<?php
/*============================================================================*\
  test usb device access and driver controle

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
            eval("return $test ;");
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
    //var_dump($ret);

    print_r($ret);
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

// USB list
validate("io_usb_list()",'is_array($ret)');


//====================================================================
// Results
printf("{$nl}Tests: %d: <span style=\"color:green\">ok: %d </span>",$test_result['ok'] + $test_result['failed'], $test_result['ok'] );
if($test_result['failed'] > 0 )
    printf("<span style=\"color:red\">failed: %d</span>\n",$test_result['failed']);
echo "\n";
exit;
?>
