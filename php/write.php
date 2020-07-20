<?php
    //$formInput=$_GET['key'];
    $formInput= file_get_contents('php://input');
    $fn=$_GET['fn'];
    $fn="../".$fn;
    $myfile = fopen($fn, "w") or die("Unable to open file!");
    $txt = $formInput;
    fwrite($myfile, $txt);
    fclose($myfile);
?>