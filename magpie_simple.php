<?php

require_once('rss_fetch.inc');

// lots of debugging info
define('MAGPIE_DEBUG', 2);
// flush cache quickly for debugging purposes, don't do this on a live site
define('MAGPIE_CACHE_AGE', 10);

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
	
$url = $_GET['url'];

?>

<html
<body LINK="#999999" VLINK="#000000">

<form>
<input type="text" name="url" size="40" value="<?php echo $url ?>"><input type="Submit">
</form>

<?php

if ( $url ) {
	echo "displaying: $url<p>";
	$rss = fetch_rss( $url );
	echo slashbox ($rss);
}

echo "<pre>";
print_r($rss);
echo "</pre>";
?>

</body>
</html>

