<?php

require_once('rss_fetch.inc');
$url = $_GET['url'];
	
if ( ! $url ) {
	$url = 'http://protest.net/dcimc/rss';
}
	
list( $rss, $status, $msg) = fetch_rss( $url );


# just some quick and ugly php to generate html
#
#
function slashbox ($rss) {
	echo "<table cellpadding=2><tr>";
	echo "<td bgcolor=#006666>";
	
	# get the channel title and link properties off of the rss object
	#
	$title = $rss->channel['title'];
	$link = $rss->channel['link'];
	
	echo "<a href=$link><font color=#FFFFFF><b>$title</b></font></a>";
	echo "</td></tr>";
	
	# foreach over each item in the array.
	# displaying simple links
	#
	# we could be doing all sorts of neat things with the dublin core
	# info, or the event info, or what not, but keeping it simple for now.
	#
	foreach ($rss->items as $item ) {
		echo "<tr><td bgcolor=#cccccc>";
		echo "<a href=$item[link]>";
		echo $item[title];
		echo "</a></td></tr>";
	}		
	
	echo "</table>";
}
	
?>

<html
<body LINK="#999999" VLINK="#000000">

displaying: <? echo $url ?>
<p>
<? slashbox ($rss); ?>
</body>
</html>
