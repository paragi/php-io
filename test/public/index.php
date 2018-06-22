<!DOCTYPE HTML>
<html>
<head>
<meta charset="utf-8" />

<link rel="shortcut icon" href="/favicon.ico" >
<link rel="icon" href="/favicon_big.ico" type="image/ico" >
<link rel="stylesheet" type="text/css" href="/theme.css" />

<style>
table{
	border:1px solid #f0d988;
	padding:3px;
	margin:6px;
}
td:hover{
	background-color:rgba(240,217,136,0.2);
	cursor:pointer;
}
*{font-size: 16pt;}
</style>
<?php
  $page = @$_REQUEST['page'];
  $title = substr($page,strrpos($page,"/")+1) ? : "test scripts:";
  echo "<title>{$title}</title>" ;
?>
</head>

<body>
<?php echo "<h1>$title</h1>"; ?>
<?php
  if(empty($page)){
    echo "<form name=\"form\" method=\"GET\">";
    echo "<input type=\"hidden\" name=\"page\" >";
    echo "</form><table>";
    foreach(array_merge(glob(__DIR__."/*.php"),glob(__DIR__."/*.html")) as $path)
      if(!strrpos($path,"index.php"))
        echo "<tr><td onclick=\"document.forms['form'].elements['page'].value='$path';document.forms['form'].submit();console.log(document.form);\">"
         .substr($path,strrpos($path,"/")+1)."</td></tr>";
    echo "</table>";

  }else{
    echo "<pre>\n";
    require $page;
  }
?>
</body>
