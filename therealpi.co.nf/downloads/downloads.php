<?php

session_start();

if (isset($_SESSION['visited'])){
        $_SESSION['visited'] = true;
}
else{
        include '../counting.php'; 
}

function boxPrint($title, $paragraph){
        echo 
                                '<article>
                                        <header>
                                                <h1>'.$title.'</h1>
                                        </header>
                                        <br/><p class="paragraph">'.$paragraph.'</p>
                                        <footer>
                                                <p>developed by Matthew Niemiec</p>
                                        </footer>
                                </article>';
}

?>


<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html lang="en">
<head>
	<meta charset="utf-8"/>
	<title>Downloads</title>
	<link rel="stylesheet" href="../main.css">
        <link rel="stylesheet" href="downloads.css">
</head>
<body>
	<div id="mainWrapper">
		<?php include '../header.php';?>
		
		<div id="bodyDiv">
		
			<section id="mainSection">
                                <a href="/downloads/programs/WinAverager.7z" download="Averager.7z">
                                        <?php boxPrint('Averager (Windows)', '<span class=\'firstLetter\'>T</span>his is a small application used to store and calculate grades based on weights that the
                                                user inputs. It also has other functionalities, such as extra credit, opening syllabi, 
                                                and storing separate classes. To use on any Windows machine, simply download, extract
                                                using 7-zip, place in a suitable location, and run. Updated 8/23/16'); ?>
                                </a>
                                
                                <a href="/downloads/programs/AveragerLinux.7z" download="Averager.7z">
                                        <?php boxPrint('Averager (Linux)', '<span class=\'firstLetter\'>T</span>his is a small application used to store and calculate grades based on weights that the
                                                user inputs. It also has other functionalities, such as extra credit, opening syllabi, 
                                                and storing separate classes. To use, extract using 7-zip, save in a preferred location,
                                                and run executable. Updated 8/23/16'); ?>
                                </a>
                                
                                <a href="/downloads/programs/Averager.7z" download="Averager.7z">
                                        <?php boxPrint('Averager (Source Release)', '<span class=\'firstLetter\'>T</span>his is a small application used to store and calculate grades based on weights that the
                                                user inputs. It also has other functionalities, such as extra credit, opening syllabi, 
                                                and storing separate classes. To use, extract using 7-zip, save in a preferred location,
                                                and run using Python. Updated 8/23/16'); ?>
                                </a>
                                
                                <?php
                                if(isset($_SESSION['permissions']) && $_SESSION['permissions'] >= 3){
                                echo '<a href="/downloads/programs/therealpiSource.7z" download="TheRealPiSource.7z">';
                                boxPrint('The Real Pi Source Code', '<span class=\'firstLetter\'>T</span>his is the source code to this very web site, including all 
                                        php text and database connections. If you are reading this, you are considered to be a trusted user
                                        and, as always, I appreciate feedback, particularly when it comes to security. This file was 
                                        updated on 7/31/16');
                                echo '</a>';
                                }
                                ?>
                                
                        </section>
			
			<?php isset($_SESSION['username']) ? include '../logout.php' : include '../login.php'; ?>
		
		</div>
		
		<footer id="footer">
			@Copyright therealpi 2016
		</footer>
	</div>
</body>
</html>