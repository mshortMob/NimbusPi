<?php
    //$formInput=$_GET['key'];
    $formInput=file_get_contents('php://input');
    $formInput = str_replace('data:image/png;base64,', '', $formInput);
    $formInput = str_replace(' ', '+', $formInput);
	$formInput = base64_decode($formInput);
    $fn=$_GET['fn'];
    $fn="../".$fn;
    $myfile = fopen($fn, "w") or die("Unable to open file!");
    $txt = $formInput;
    fwrite($myfile, $txt);
    fclose($myfile);
?>