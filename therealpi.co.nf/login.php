<?php 

if(isset($_POST['username']) && isset($_POST['password'])){
        session_start();
        
        $entered_name = $_POST['username'];
        $entered_password = $_POST['password'];
        
        $con = mysqli_connect('fdb3.biz.nf', '2084457_myflypi', 'my1mana!', '2084457_myflypi') or die('Could not connect to mySQL server!');
        
        $query = "SELECT `password` FROM `users` WHERE `username`='".$entered_name."'";
        
        if($results = mysqli_query($con, $query)){
                while($row = mysqli_fetch_array($results)){
                        $database_password = $row[0];
                        if(md5($entered_password) == $database_password){
                                
                                $_SESSION['username'] = $entered_name;
                                
                                
                                
                                $_SESSION['message'] = 'You have logged in!';
                                
                                $permissions_query = "SELECT `clearance` FROM `users` WHERE `username`='".$entered_name."'";

                                if($results = mysqli_query($con, $permissions_query)){
                                        $_SESSION['permissions'] = mysqli_fetch_array($results)[0];
                                }
                                
                                
                        }
                        else{
                                $_SESSION['message'] = "Incorrect Password";
                        }
                        
                }
                
        }
        else{
                echo mysqli_error($con);
        }
        $redirect_page = 'http://therealpi.co.nf';
                                
        header('Location: '.$redirect_page);
}

?>


<div id="side">
<aside id="news">
        <h4>Login</h4>
        <p>Please login for more features!</p><br/>
        
        <form action="<?php echo $_SERVER['HOST_NAME']; ?>/login.php" method="POST">
                Username: <input type="text" class="loginBox" name="username" />
                Password: <input type="password" class="loginBox" name="password" />
                <br/>
                <input type='submit' value='Login' id="loginSubmit" />
        </form>
        
</aside>
</div>