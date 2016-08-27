<?php 
if(isset($_POST['username']) && isset($_POST['password'])){
        session_start();
        
        $entered_name = $_POST['username'];
        $entered_password = $_POST['password'];
        
        $con = mysqli_connect('fdb3.biz.nf', '2084457_myflypi', 'my1mana!', '2084457_myflypi') or die('Could not connect to mySQL server!');
        
        $query = "SELECT `password` FROM `users` WHERE `username`='".$entered_name."'";
        
        if($results = mysqli_query($con, $query)){
                if($row = mysqli_fetch_array($results)){
                        $database_password = $row[0];
                        if(md5($entered_password) == $database_password){
                                
                                $_SESSION['username'] = $entered_name;
                                
                                
                                $permissions_query = "SELECT `clearance` FROM `users` WHERE `username`='".$entered_name."'";

                                if($results = mysqli_query($con, $permissions_query)){
                                        $_SESSION['permissions'] = mysqli_fetch_array($results)[0];
                                }
                                
                                
                        }
                        else{
                                $_SESSION['message'] = "Incorrect Password";
                                $_SESSION['showed'] = 2;
                        }
                        
                }
                else{
                        $_SESSION['message'] = 'Username not valid';
                        $_SESSION['showed'] = 2;
                }
                
        }
        else{
                $_SESSION['message'] = 'Unexpected error!';
                $_SESSION['showed'] = 2;
        }
        $redirect_page = $_SESSION['current_page'];
                                
        header('Location: '.$redirect_page);
}
else{
        if(!isset($_SESSION['message'])){
                $_SESSION['message'] = "Please log in for more features!";
         }
}

?>


<div id="side">
<aside id="news">
        <h4>Login</h4>
        <p><?php 
                if($_SESSION['showed'] > 0){
                        echo $_SESSION['message']; 
                        $_SESSION['showed']--;
                }else{
                        echo 'Please log in for more features!';
                }
        ?></p><br/>
        <form action="<?php echo $_SERVER['HOST_NAME']; ?>/login.php" method="POST">
                Username: <input type="text" class="loginBox" name="username" />
                Password: <input type="password" class="loginBox" name="password" />
                <br/>
                <input type='submit' value='Login' id="loginSubmit" />
        </form>
        
</aside>
</div>