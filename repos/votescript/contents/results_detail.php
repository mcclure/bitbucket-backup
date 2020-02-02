<?php require "header.php"; ?>

<?php
 
password_check(); // THIS IS AN ADMIN PAGE-- REQUIRES THE PASSWORD

set_time_limit(240);

// "concordet" is used throughout this file to refer to Borda ranking

$concordet = array();
$post = array(); 
$approval = array();
$names = array(); 

$result = mysql_query("SELECT * FROM ftvote");

while ($row = mysql_fetch_array($result)) {
	$concordet[$row[game]] += (21-$row[rate]);
	$approval[$row[game]]++;
	if ($row[rate]==1) $post[$row[game]]++;
}

mysql_free_result($result);

$result = mysql_query("SELECT * FROM ftgame");

while ($row = mysql_fetch_array($result)) {
	$names[$row[id]] = $row[name];
}

mysql_free_result($result);

arsort($concordet); arsort($post); arsort($approval);

echo "<p>First Past the Post</p><ol>";
$count=0;
foreach($post as $k=>$v) { echo "<li>$names[$k] (<font color=darkred>$v</font>)"; if($count++>18) break; }
echo "</ol><p>Approval</p><ol>";
$count=0;
foreach($approval as $k=>$v) { echo "<li>$names[$k] (<font color=darkred>$v</font>)"; if($count++>18) break; }
echo "</ol><p>Borda count</p><ol>";
$count=0;
foreach($concordet as $k=>$v) { echo "<li>$names[$k] (<font color=darkred>$v</font>)"; if($count++>18) break; }
echo "</ol><p>IRV</p>";
if (1) {
function remove($table,$game) {
	$votes = array();
	$result = mysql_query("SELECT * FROM $table WHERE game=$game");
	while ($avote = mysql_fetch_array($result)) { $votes[] = $avote; }
	mysql_free_result($result);

	foreach ($votes as $vo) {
		mysql_query("update $table set rate=rate-1 WHERE user=$vo[user] and rate>$vo[rate]") or crash();
	}

	mysql_query ("delete from $table where game=$game" ) or crash();
}

        mysql_query ("create table temp1 select * from ftvote" ) or crash();
	echo "<ol>";
	for($i=0;$i<20;$i++) {
		mysql_query ("create table temp2 select * from temp1" ) or crash();
		while(1) {
			$post = array();
			$result = mysql_query("SELECT * FROM temp2 where rate=1");
			while ($row = mysql_fetch_array($result)) {
				$post[$row[game]]++;
			}
			mysql_free_result($result); 

			asort($post);

			$postk = array_flip($post);
			$pickedvalue = array_shift($postk);
			$pickedvote = array_shift($post);
                        if (!count($post)) break;              # if no competitors left
			remove("temp2",$pickedvalue);
		}
		echo "<li>$names[$pickedvalue] (<font color=darkred>$pickedvote</font>)";
		remove("temp1",$pickedvalue);
		mysql_query ("drop table temp2" ) or crash();
	}
        mysql_query ("drop table temp1" ) or crash();
}
?>

<?php require "footer.php"; ?>
