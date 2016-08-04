<?php 

if(isset($_POST['submit'])){

        session_start();

        unset($_SESSION['username']);
        unset($_SESSION['permissions']);
        unset($_SESSION['message']);

        $redirect_page = 'http://therealpi.co.nf';
                                
        header('Location: '.$redirect_page);
}

?>


<div id="side">
<aside id="news">
        <h5><?php echo 'Welcome, '.$_SESSION['username'].'!'?></h5>
        
        <form action="<?php echo $_SERVER['HOST_NAME']; ?>/logout.php" method="POST">
                <input type='submit' value='Logout' id="loginSubmit" name="submit" />
        </form>
        
</aside>
</div>