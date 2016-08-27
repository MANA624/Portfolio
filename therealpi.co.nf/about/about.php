<?php

session_start();

$_SESSION['current_page'] = 'http://' . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'];

if (isset($_SESSION['visited'])){
        $_SESSION['visited'] = true;
}
else{
        include '../counting.php'; 
}

?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html lang="en">
<head>
	<meta charset="utf-8"/>
	<title>About</title>
	<link rel="stylesheet" href="../main.css">
</head>
<body>
	<div id="mainWrapper">
		<?php include '../header.php';?>
		
		<div id="bodyDiv">
		
			<section id="mainSection">
				<article>
					<header>
						<hgroup>
							<h1>Who Made This Site?</h1>
						</hgroup>
					</header>
                                        
                                        <img src="favpic.JPG" id="mypic"/>
                                        
					<p class='paragraph'>
                                        <span class='firstLetter'>H</span>ello! My name is Matthew Niemiec, and I am the sole developer of this web site. In 
                                        addition to this site being about what I can do with a keyboard, I would like to tell you
                                        a little bit about what I do with my life!
                                        </p>
                                        
                                        <p class='paragraph'>
                                        After graduating first in my class from Cimarron High School, I decided to attend the
                                        University of Colorado. I am currently studying - you guessed it
                                         - Computer Science. I began as a Mechanical Engineer with a CS minor, but quickly decided to
                                         shift the focus. I began programming in October 2015 in Python, which is still my favorite
                                         language. In class I also began to code in Arduino, Matlab, C++, and Mathematica. I quickly
                                         realized I had an aptitude for it, and second semester I kept spending all my free
                                         time coding. Soon I decided to change majors altogether, and have had no regrets since! I simply
                                         love what I do!
                                        </p>
                                        
                                        <p class='paragraph'>
                                        While I love coding, I also spend free time pursuing other hobbies. I enjoy downhill mountain
                                        biking, snowboarding, running, Photshopping, playing chess, and hanging out with my friends from the Christian
                                        campus ministry group. I love living in Boulder, where I can enjoy a friendly atmosphere, be 
                                        unique, and do anything from trying a new local restaurant to tossing a football on the lawn.
                                        The sky is the limit on what I can do. Or maybe the cloud is :)
                                        </p>
                                        
					<footer>
						<p>written by Matthew Niemiec</p>
					</footer>
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