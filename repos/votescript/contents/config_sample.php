<?php

// MySQL settings:

define('DB_HOST', 'localhost'); // The address of MySQL-- probably localhost
define('DB_USER', 'SET-THIS'); // The user to connect to MySQL with
define('DB_PASSWORD', 'SET-THIS'); // The password to connect to MySQL with
define('DB_NAME', 'SET-THIS'); // The name of the database within MySQL

// Poll settings:

define('ADMIN_PASSWORD', 'SET-THIS'); // MUST SET-- secret code to access admin scripts

// Display:

$PAGE_TITLE = 'This is a poll'; // Displayed in <title> and at top of page

// This is displayed on first load.
$PAGE_WELCOME = <<<END
<p>This is a "game of the year" poll. It is just for fun..</p>
<p>This page will ask you to select your favorite video games from this last year from a list. I tried to make the list as complete as possible without including ports or imports. I'll run this poll for a week, then post which games did best. Remember, some of your favorite games may have been from early in the year, so try to remember what they might have been!</p>
<p>Want to give it a try?</p>
END;

// This is displayed on a successful vote.
$PAGE_YOUVOTED = <<<END
<p>Check back <a href=".">here</a> in a few days and I'll have some results up. I plan to end this the evening of Jan 12 and post the results shortly afterward.</p>"
END;

?>
