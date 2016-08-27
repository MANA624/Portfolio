<?php

session_start();

$_SESSION['current_page'] = 'http://' . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'];

if (isset($_SESSION['visited'])){
        $_SESSION['visited'] = true;
}
else{
        include '../counting.php'; 
}

if($_SESSION['permissions'] > 0){

        $con = mysqli_connect('fdb3.biz.nf', '2084457_myflypi', 'my1mana!', '2084457_myflypi') or die('Could not connect to mySQL server!');
        
        date_default_timezone_set('America/Denver');
        
        $hour = date('H');
        $minute = date('i');
        $final_time = ($hour*60 + $minute);
        
        $day = jddayofweek ( cal_to_jd(CAL_GREGORIAN, date("m"),date("d"), date("Y")), 0 );
        
        $query = "SELECT `".$day."` FROM `schedule` WHERE 1";
        
        $is_doing_something = false;
        
        if($results = mysqli_query($con, $query)){
                while($row = mysqli_fetch_array($results)){
                        $string = $row[0];
                        $string_position = strpos($string, ' ', 0);
                        $start_time = substr($string, 0, $string_position);
                        $string = substr($string, $string_position+1);
                        $string_position = strpos($string, ' ', 0);
                        $end_time = substr($string, 0, $string_position);
                        $string = substr($string, $string_position+1);
                        if($start_time < $final_time && $final_time < $end_time){
                                $is_doing = $string;
                                $is_doing_something = true;
                                break;
                        }
                }
                
        }
        else{
                echo mysqli_error($con);
        }
        
        if(!$is_doing_something){
                if(!isset($_COOKIE['event'])){
                        $query = 'SELECT `7` FROM `schedule` ORDER BY RAND() LIMIT 1';
                        if($results = mysqli_query($con, $query)){
                                $row = mysqli_fetch_array($results);
                                setcookie('event',$row[0],time()+300);
                                $is_doing = $row[0];
                        }
                        else{
                                echo mysqli_error($con);
                        }
                }
                else{
                        $is_doing = $_COOKIE['event'];
                }
        }

}
else{
        $is_doing = ' ... well, wouldn\'t you like to know?';
}

?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"> 

<html lang="en">
<head>
        <title>Where Am I?</title>
        <link rel="stylesheet" href="../main.css" />
        <meta charset="utf-8"/>
        <link rel="stylesheet" href="tracking.css" />
</head>

<body>
        <div id="mainWrapper">
		<?php include '../header.php';?>
		
		<div id="bodyDiv">
                        
                        <section id="mainSection">
				<article>
                                        <br/><h1>Matthew is <?php echo $is_doing;?></h1><br/>
				</article>
			</section>
			
			<?php isset($_SESSION['username']) ? include '../logout.php' : include '../login.php'; ?>
		
		</div>
		
		<footer id="footer">
			@Copyright therealpi 2016
		</footer>
	</div>



</body>
</html>
