<?php require "header.php"; 

if ($_POST[submit] === "* I am done *") {

$result = mysql_query("SELECT * FROM ftuser where id=$_POST[user] and hostname  is not NULL;");
$isrepeat = mysql_num_rows($result);
mysql_free_result($result);
if ($isrepeat) {
	echo "<p>You've already submitted once.</p>";
} else {
        mysql_query ("update ftuser set hostname=\"$_SERVER[REMOTE_ADDR]\", stamp=now() where id=$_POST[user];") or crash();
	echo "Thanks! You voted for:<ol>";
	$count=1;
	foreach ($votes as $g) {
		if (!$g->v || $count == 21) break;
		mysql_query("insert into ftvote(game,rate,user) values({$g->gid},{$g->v},$_POST[user])") or crash();
		echo "<li>" . $g->gname;
		$count++;
	}
	echo "</ol>";
}

?>

<?php echo $PAGE_YOUVOTED ?>
<?php
} else if ($_POST[submit] === "Continue") {
?>
<form name="input" action="index.php" method="post">
<p><b>Now out of the games you just picked, rate them depending on which were your favorites. You can pick at absolute most 20.  Don't worry about getting the numbers exactly right, or entering numbers too many times-- you can just be vague, and the page will fix itself. Hit "Continue" to make the page sort your choices for you, or when you are done.</b></p>
<?php
if ($_POST[hedgehog]) {
	echo "<p>When you feel like the list of your favorite games below is right, click \"I am done\".</p>";
	$done = <<<EOF
<input type="submit" name="submit" value="* I am done *">
EOF;
}
?>
<input type="hidden" name="hedgehog" value="YES">
<p><input type="submit" name="submit" value="Continue"><?php echo $done; ?></p>
<table>
<?php
$count = 1;
foreach ($votes as $g) {
	if ($count) { 
		$count++;
		if (!$g->v || $count == 22) { $count = 0; echo "<tr><td>&nbsp;</td></tr>"; }
	}
	$g->field();
}
?>
</table>
<input type="hidden" name="user" value="<?php echo $_POST[user] ?>">
<input type="submit" name="submit" value="Continue"><?php echo $done; ?>
</form>

<?php
} else if ($_POST[submit] === "Start") {
?>
<form name="input" action="index.php" method="post">
<p><b style="color:darkred">Check all the games that you played this year, and liked-- the ones you *might* put in your top 10. You can pick as few or as many as you like (up to 20 if you really want, though 5 or 10 might be more normal). Don't worry about being exact, or clicking too many. The next page will let you fix this. When you're done, hit this button.</b></p>
<p><input type="submit" name="submit" value="Continue"></p>
<table>
<?php
foreach ($votes as $g) {
$g->checkbox();
}
?>
</table>

<input type="hidden" name="user" value="<?php echo $_POST[user] ?>">
<p><input type="submit" name="submit" value="Continue"></p>
</form>

<?php
} else {
?>

<form name="input" action="index.php" method="post">
<?php echo $PAGE_WELCOME ?>
<input type="submit" name="submit" value="Start">
<?php
	mysql_query ("insert into ftuser(initial) values(\"$_SERVER[REMOTE_HOST],$_SERVER[HTTP_REFERER]\")") or crash();
	$myuser = mysql_insert_id();
	echo <<<EOF
	<input type="hidden" name="user" value="$myuser">
EOF
?>
</form>

<?php } require "footer.php"; ?>
