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
* @author	kellan@protest.net
* @version	.01-alpha
* @license	GPL
*/

class RSSCache {
	var $BASE_CACHE = './cache';
	var $MAX_AGE	= 3600;  // one hour
	var $ERROR 		= "";
	
	function RSSCache ($base, $age) {
		if ( $base ) {
			$this->BASE_CACHE = $base;
		}
		if ( $age ) {
			$this->MAX_AGE = $age;
		}
		
		// attempt to make the cache directory
		if ( ! file_exists( $this->BASE_CACHE ) ) {
			$status = @mkdir( $this->BASE_CACHE, 0755 );
			
			// if make failed 
			if ( ! $status ) {
				$this->error(
					"Cache couldn't make dir '" . $this->BASE_CACHE . "'."
				);
			}
		}
	}
	
	function set ($url, $rss) {
		$this->ERROR = "";
		$cache_file = $this->file_name( $url );
		$fp = @fopen( $cache_file, 'w' );
		
		if ( ! $fp ) {
			$this->error(
				"Cache unable to open file for writing: $cache_file"
			);
			return 0;
		}
		
		
		$data = $this->serialize( $rss );
		fwrite( $fp, $data );
		fclose( $fp );
		
		return $cache_file;
	}
	
	function get ($url) {
		$this->ERROR = "";
		$cache_file = $this->file_name( $url );
		
		if ( ! file_exists( $cache_file ) ) {
			$this->error( 
				"Cache doesn't contain: $url (cache file: $cache_file)"
			);
			return 0;
		}
		
		$fp = @fopen($cache_file, 'r');
		if ( ! $fp ) {
			$this->error(
				"Failed to open cache file for reading: $cache_file"
			);
			return 0;
		}
		
		$data = fread( $fp, filesize($cache_file) );
		$rss = $this->unserialize( $data );
		
		return $rss;
	}
	
	function check_cache ( $url ) {
		$this->ERROR = "";
		$filename = $this->file_name( $url );
		
		if ( file_exists( $filename ) ) {
			$mtime = filemtime( $filename );
			$age = time() - $mtime;
			if ( $this->MAX_AGE > $age ) {
				return 'HIT';
			}
			else {
				return 'STALE';
			}
		}
		else {
			return 'MISS';
		}
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
	
	function error ($errormsg, $lvl=E_USER_WARNING) {
		// append PHP's error message if track_errors enabled
		if ( $php_errormsg ) { 
			$errormsg .= " ($php_errormsg)";
		}
		$this->ERROR = $errormsg;
		trigger_error( $errormsg, $lvl);		
	}
}

?>
