<?php
    $fileName=$_GET['fn'];
    $fileName="../".$fileName;
    $myfile = fopen($fileName, "r") or die("Unable to open file!");
    echo fread($myfile,filesize($fileName));
    fclose($myfile);
?>