<?php

$con = mysqli_connect('fdb3.biz.nf', '2084457_myflypi', 'my1mana!', '2084457_myflypi') or die('Could not connect to mySQL server!');
        
$query = "SELECT `welcome` FROM `users` WHERE `username`='MANA624'";

if($results = mysqli_query($con, $query)){
$row = mysqli_fetch_array($results)[0];
}

echo $row;

?>