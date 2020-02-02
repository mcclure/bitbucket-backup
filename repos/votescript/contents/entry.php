<?php require "header.php";?>

<?php 

password_check(); // THIS IS AN ADMIN PAGE-- REQUIRES THE PASSWORD

if ($_POST[clear]) {
	mysql_query ("delete from ftgame" ) or crash();
}

foreach(preg_split("/[\r\n]+/", $_POST[games]) as $game) {
	if ($game)
        mysql_query ("insert into ftgame(name) values(\"$game\")" ) or crash();
}

$result = mysql_query("SELECT * FROM ftgame ORDER BY name");

while ($row = mysql_fetch_array($result)) {
	echo("<li>$row[name]");
}

mysql_free_result($result);

?>
<p>
<form name="input" action="entry.php?<?php echo $_SERVER['QUERY_STRING']; ?>" method="post">
Games:<br>
<textarea rows="15" cols="35" WRAP="VIRTUAL" name="games"></textarea><br>
<INPUT TYPE="checkbox" NAME="clear">Clear first
<input type="submit" value="Submit">
</form>
</p>

<?php require "footer.php"; ?>
