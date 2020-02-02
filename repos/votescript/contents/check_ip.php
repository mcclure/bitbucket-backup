<?php require "header.php"; ?>

<?php 

password_check(); // THIS IS AN ADMIN PAGE-- REQUIRES THE PASSWORD

$concordet = array();
$rates = array();
$colors = array();

$result = mysql_query("SELECT * FROM ftuser");

while ($row = mysql_fetch_array($result)) { 
	$concordet[$row['hostname']]++; 
}

mysql_free_result($result);

arsort($concordet);

foreach ($concordet as $a => $b) { 
	echo "<li>$a: $b";
} 

?>

<?php require "footer.php"; ?>
