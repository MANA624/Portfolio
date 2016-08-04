<?php

session_start();

if (isset($_SESSION['visited'])){
        $_SESSION['visited'] = true;
}
else{
        include '../counting.php'; 
}

$response = '';
$success = false;
ini_set('post_max_size', '64M');
ini_set('upload_max_filesize', '64M');

if(isset($_FILES['fileUpload'])){
	$name = $_FILES['fileUpload']['name'];
	$size = $_FILES['fileUpload']['size'];
	$max_size = 2097152;
	$type = $_FILES['fileUpload']['type'];

	$tmp_name = $_FILES['fileUpload']['tmp_name'];

	$extension = strtolower(substr($name, strpos($name, '.')+1));

}

if(isset($name)){
	if(!empty($name)){
		if(($extension=='jpg'||$extension=='jpeg') && ($type='image/jpeg')){
			if($size <= $max_size){
				$location = '../uploads/';
				
				if(move_uploaded_file($tmp_name, $location.$name)){
                                        $success = true;
					$response = 'File upload successful';
				}
				else{
					$response = 'File upload failed';
				}
			}
			else{
				$response = 'File must be less than 2MB';
			}
		}
		else{
			$response = 'Not a valid extension';
		}
	}
	else{
		$response = 'You must submit a file';
	}
}

?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html lang="en">
<head>
	<meta charset="utf-8"/>
	<title>Contact Us</title>
	<link rel="stylesheet" href="../main.css" />
        <link rel="stylesheet" href="contact.css" />
</head>
<body>
	<div id="mainWrapper">
		<?php include '../header.php';?>
		
		<div id="bodyDiv">
		
			<section id="mainSection">
				<article>
                                        <?php echo $success ? '<p class="success">' : '<p class="error">';
                                        echo $response.'</p><br/>'; ?>
                                        
                                        <p class="info">We welcome your comments and suggestions! If you have had 
                                        an error, please specify your problem and what program (or web page) you were using.
                                        If you are experiencing a graphical error, please submit a screenshot and specify
                                        your browser and version number. Thank you for your input!</p>
					<form action="contact.php" method="POST" enctype="multipart/form-data">
                                                <br/>Your Name:<br/>
                                                <input type="text" class="nameInfo" placeholder="eg. John Doe" />
                                                <br/>Your email:<br/>
                                                <input type="text" class="nameInfo" placeholder="example@example.com" />
                                                <br/>Subject:<br/>
                                                <input type="text" class="nameInfo" placeholder="Subject" />
                                                <br/>Please list any comments or suggestions below:<br/>
                                                <textarea id="suggest" placeholder="Comments and suggestions" /></textarea>
                                                <br/>
                                                <input type="file" name="fileUpload"><br/><br/>
                                                <br/>
                                                <input type="submit" value="Send" id="sendButton">
                                                
                                        </form>
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