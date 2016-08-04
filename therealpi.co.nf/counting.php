<?php

function hit_count() {

        
        $con = mysqli_connect('fdb3.biz.nf', '2084457_myflypi', 'my1mana!', '2084457_myflypi') or die('Could not connect to mySQL server!');
        
        $ip_address = $_SERVER['REMOTE_ADDR'];
        
        $query = "INSERT INTO `ip_addresses` VALUES ('".$ip_address."')";
        
        @mysqli_query($con, $query);
	
	
}

hit_count();

?>