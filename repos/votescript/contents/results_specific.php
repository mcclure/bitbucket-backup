<?php require "header.php"; ?>

<?php 

password_check(); // THIS IS AN ADMIN PAGE-- REQUIRES THE PASSWORD

$concordet = array();
$rates = array();
$colors = array(); 
$voters = array();

# Show only results from Twitter
$result = mysql_query("SELECT ftvote.* FROM ftvote, ftuser where ftvote.user=ftuser.id and (ftuser.initial like '%t.co%' or ftuser.initial like '%twitter%' or ftuser.initial like '%tweet')");

# Show only results from a site with "penny" in the name, like, penny-arcade.com
#$result = mysql_query("SELECT ftvote.* FROM ftvote, ftuser where ftvote.user=ftuser.id and (ftuser.initial like '%penny%')"); 

while ($row = mysql_fetch_array($result)) {
	$concordet[$row[game]] += (21-$row[rate]);
	$rates[$row[game]][$row[rate]]++;
	$voters[$row[user]] = 1;
}

mysql_free_result($result);

$result = mysql_query("SELECT * FROM ftgame");

while ($row = mysql_fetch_array($result)) {
	$names[$row[id]] = $row[name];
}

mysql_free_result($result);

arsort($concordet);

foreach($rates as $a => $b) {
	foreach ($b as $c => $d) {
		$colors[$d] = 1;
	}
}

krsort($colors);


$mycolors=array(
"#880000","#BB0000","#FF0000","#000088","#0000BB","#0000FF","#004400","#007700","#00AA00",

"#040404","#080808","#0C0C0C","#111111","#151515","#191919","#1D1D1D","#222222","#262626","#2A2A2A","#2E2E2E","#333333","#373737","#3B3B3B","#404040","#444444","#484848","#4C4C4C","#515151","#555555","#595959","#5D5D5D","#626262","#666666","#6A6A6A","#6E6E6E","#737373","#777777","#7B7B7B","#808080","#848484","#888888","#8C8C8C","#919191","#959595","#999999","#9D9D9D","#A2A2A2","#A6A6A6","#AAAAAA","#AEAEAE","#B3B3B3","#B7B7B7","#BBBBBB","#C0C0C0","#C4C4C4","#C8C8C8","#CCCCCC","#D1D1D1","#D5D5D5","#D9D9D9","#DDDDDD","#E2E2E2","#E6E6E6","#EAEAEA","#EEEEEE","#F3F3F3","#F7F7F7","#FBFBFB"
);


$count = 0; 
foreach ($colors as $a => $b) {
	$colors[$a] = $mycolors[$count++];
}

$votercount = count($voters);
echo "<p align=center>Based on $votercount voters:</p>";

echo "<table><tr><td><font color=gray>&nbsp;&nbsp;&nbsp;FOR THIS GAME, THE NUMBER OF PEOPLE WHO RATED:</font></td>";
for($count = 1; $count < 21; $count++) {
	echo "<td><font color=gray>" . ($count < 10 ? "&nbsp;" : "") . $count . "</font></td>";
}
echo "</tr>";

$count = 1;
foreach ($concordet as $a => $b) {
	echo "<tr><td>$count. $names[$a]</td>";
	for ($c = 1; $c < 21; $c++) {
		$l = $rates[$a][$c];
		echo "<td bgcolor=\"$colors[$l]\"><font color=white>$l</font></td>";
	}
	echo "</tr>";
	$count++;
}
echo "</table>";

?>

<?php require "footer.php"; ?>
