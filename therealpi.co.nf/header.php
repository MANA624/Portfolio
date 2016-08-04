        <?
        $home = $_SERVER['HTML_HOST'].'/';
        ?>
<header id="topHeader">
                <h1 class="title">Welcome to The Real Pi!</h1>
        </header>
        
        <nav id="navigationBar">
                <a href="<?php echo $home?>"><div id="home">Home</div></a>
                <a href="<?php echo $home; ?>downloads/downloads.php"><div id="tutorials">Downloads</div></a>
                <a href="<?php echo $home; ?>whereAmI/tracking.php"><div id="doing">Where Am I?</div></a>
                <a href="<?php echo $home; ?>about/about.php"><div id="local">About</div></a>
                <a href="<?php echo $home; ?>contact/contact.php"><div id="local">Contact Us</div></a>
        </nav>