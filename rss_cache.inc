<?php

/**
* A simple cache designed to cache RSS objects (rss_parse.inc)
*
* Designed to be used as a part of MagpieRSS, RSSCache accepts a
* url, and an RSS object, and serializes the RSS object to the disk
* keyed on the md5 of the url
*
* NOTE: This is rolling cache, therefore there is no garbage collection
*
* Usage example:
*
* require_once('rss_cache.inc');
* $cacedir = "./cache";
* $maxage = 60*60    // one hour
* $cache = new RSSCache($cachedir); 
*
* list ($status, $filename, $mtime) = $cache->check_cache( $rss_url );
* $age = time() - $mtime;
* if ( $status and $maxage > $age) {
* 	return $cache->get( $rss_url )
* }
* else 
*	$rss = ....get rss object..
*	$cache->set( $rss_url, $rss );
* }
*
* @author	kellan@protest.net
* @version	.01-alpha
* @license	GPL
*/

class RSSCache {
	var $BASE_CACHE = './cache';
	
	function RSSCache ($base) {
		if ( $base ) {
			$this->BASE_CACHE = $base;
		}
		
		if ( ! file_exists( $this->BASE_CACHE ) ) {
			mkdir( $this->BASE_CACHE, 0755 );
		}
	}
	
	function set ($url, $rss) {
		$cache_file = $this->file_name( $url );
		$fp = @fopen( $cache_file, 'w' );
		
		if ( ! $fp ) {
			return 
				array(0, "Unable to open cache file for writing: $cache_file");
		}
		
		$data = $this->serialize( $rss );
		fwrite( $fp, $data );
		fclose( $fp );
		
		return array(1, $cache_file);
	}
	
	function get ($url) {
		$cache_file = $this->file_name( $url );
		if ( ! file_exists( $cache_file ) ) {
			return array(0, '', "Cache file does not exists: $cache_file");
		}
		
		$fp = @fopen($cache_file, 'r');
		if ( ! $fp ) {
			return array(0, '', 
						 "Failed to open cache file for reading: $cache_file");
		}
		
		$data = fread( $fp, filesize($cache_file) );
		$rss = $this->unserialize( $data );
		
		return array(1, $rss, 'cache.success');
	}
	
	function check_cache ( $url ) {
		$filename = $this->file_name( $url );
		
		if ( file_exists( $filename ) ) {
			$mtime = filemtime( $filename );
			return array(1, $filename, $mtime);
		}
		# else 		
		
		return array(0);
	}

	function serialize ( $rss ) {
		return serialize( $rss );
	}
	
	function unserialize ( $data ) {
		return unserialize( $data );
	}
	
	# this probably doesn't work under windows
	# does php have something perl's File::Spec module?
	#
	function file_name ($url) {
		$filename = md5( $url );
		return join( '/', array( $this->BASE_CACHE, $filename ) );
	}
}

?>
