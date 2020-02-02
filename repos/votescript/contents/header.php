<?php require "config.php"; ?>

<html>
<head>
<style type="text/css">
<!--
h1 { background-color:#332222; color:white; }
-->
</style>
<?php 
echo "<title>$PAGE_TITLE</title>";

$db = mysql_connect(DB_HOST, DB_USER, DB_PASSWORD);
if (!$db) { echo "Could not connect to database!"; }
mysql_select_db(DB_NAME);

function crash() {
	die( "Problem with SQL database: " . mysql_error() );
}

function password_check() {
	if (!$_GET[ADMIN_PASSWORD]) crash();
}

$boxes = 0;

class vote {
	var $gid;
	var $gname;
	var $v;

	function vote($nid,$nname) {
		$this->gid = $nid; $this->gname = $nname;
		$this->v = $_POST["vote_{$this->gid}"];
	}

	function checkbox() {
		global $boxes; $color = ($boxes++ % 2 ? "#EEDDDD" : "#DDCCCC");
		echo <<<EOF
<tr><td bgcolor="$color"><INPUT TYPE="checkbox" NAME="vote_{$this->gid}" value="1"> {$this->gname}</td></tr>
EOF;
	}

	function field() {
		global $boxes; $color = ($boxes++ % 2 ? "#EEDDDD" : "#DDCCCC");
		echo <<<EOF
<tr><td bgcolor="$color"><INPUT TYPE="textfield" NAME="vote_{$this->gid}" value="{$this->v}" id="e{$this->gid}" onChange="doupdate({$this->gid})"> {$this->gname}</td></tr>
EOF;
	}
}

$votes = array();
$tempabstain = array();
$tempvote = array();
$result = mysql_query("SELECT * FROM ftgame ORDER BY name");

while ($row = mysql_fetch_array($result)) {
	$some = new vote($row[id],$row[name]); # pull one out of the db
	if ($some->v) {			# if we've voted on it
		$tempvote[$some->v][] = $some; # note that
	} else $tempabstain[] = $some;		#otherwise store it
}
mysql_free_result($result);

$topmost = 1;					#counter
ksort($tempvote);
foreach($tempvote as $key => $value) {		# look at our votecounts
	shuffle($value);			# for fairness shuffle each
	foreach($value as $elem) {		# and pop out the values
		$elem->v = $topmost++;	# associated
		$votes[] = $elem;
	}
}

foreach ($tempabstain as $x) $votes[] = $x; # CONCAT

unset($tempabstain);
unset($tempvote);
unset($topmost);
?>

<script language="Javascript" type="text/javascript">
<!--

var postoobj = [];
var idtopos = [];

function doload() {
<?php
foreach ($votes as $n) {
	if ($n->v) {
		echo "\tpostoobj[{$n->v}] = document.getElementById(\"e{$n->gid}\");\n";
		echo "\tpostoobj[{$n->v}].realid={$n->gid};";
		echo "\tidtopos[{$n->gid}] = $n->v;\n";
	}
}
?>
}

function doupdate(num) {
	var oldpos = idtopos[num];
	var target = (oldpos ? postoobj[oldpos] : document.getElementById("e" + num));
	var newpos = target.value;
	if (oldpos == newpos) return;

	if (!newpos && !newpos) {
	} if (!newpos) {
		idtopos[num] = 0;
		var i = oldpos-1;
		do {
			i++;
			postoobj[i] = postoobj[i+1];
			if (postoobj[i]) postoobj[i].value = i;
			if (postoobj[i]) idtopos[postoobj[i].realid] = i;
		} while(postoobj[i]);
	} else if (!oldpos) {
		idtopos[num] = newpos;
		var i = newpos;
		var insert = target;
		do {
			var newinsert = postoobj[i];
			postoobj[i] = insert;
			if (postoobj[i]) postoobj[i].value = i;
			insert = newinsert;		
			if (postoobj[i]) idtopos[postoobj[i].realid] = i;       
			i++;
		} while (insert);
	} else if (oldpos < newpos) {
		for(var i = oldpos; i < newpos; i++) {
			if(postoobj[i+1]) {
				postoobj[i] = postoobj[i+1];
				postoobj[i].value = i;
				idtopos[postoobj[i].realid] = i;
			}
		}
	} else if (oldpos > newpos) {
		for(var i = oldpos; i > newpos; i--) {
                        if(postoobj[i-1]) { 
                                postoobj[i] = postoobj[i-1];
                                postoobj[i].value = i;
                                idtopos[postoobj[i].realid] = i;
                        }
                }
	}

	if (newpos) {
		idtopos[target.realid] = newpos;
		postoobj[newpos] = target;
	}
}

//-->
</script>
</head>
<body onLoad="doload()">
<h1><?php echo $PAGE_TITLE ?></h1>
