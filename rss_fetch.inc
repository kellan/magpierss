<?php

require_once( 'rss_parse.inc' );
require_once( 'rss_cache.inc' );

$CACHE_BASE = './cache';
$CACHE_AGE	= 60*60; // one hour
#$CACHE_AGE = 50;

/**
* A simple, functional interface to fetching, and parsing RSS files
*
* First attempts to retrieve a locally cached version.
* RSS is returned parsed, wrapped in an RSS object (see rss_parse.inc)
*
* @param	string	url of rss file
* @param	mixed	hash of options
* @return mixed	status code, data object, and message or error
*/
function fetch_rss ($url, $options=array() ) {
	global $CACHE_BASE, $CACHE_AGE;
	
	# force no cache look up
	#
	if ( $options['cache.no'] ) {
		list($remote_status, $rss, $msg) = fetch_remote_rss( $url );
		if ( $remote_status and $rss ) {
			return array($rss, 1, 'success.remote');
		}
		else {
			return array($msg, 0, 'fail.no_cache');
		}
	}
	else {
		# setup cache object
		#
		$base = ( $options['cache.base'] ) ? $options['cache.base'] : 
											 $CACHE_BASE;
		$cache_age = ( $options['cache.age'] ) ? $options['cache.age'] :
										   	 	 $CACHE_AGE;
		$cache = new RSSCache( $base );
		
		# is item in cache, what is the cache file, when was it written?
		#
		list( $cache_status, $filename, $mtime) = 
											$cache->check_cache( $url );
		$age = time() - $mtime;
		
		# if the rss object for this url is cached
		# and the cache is fresh, retrieve rss from cache
		#
		if ( $cache_status and $cache_age > $age ) {
			list($status, $rss, $msg) = $cache->get( $url );
			if ( $status and $rss ) {
				return array($rss, 1, 'success.cache');
			}
		}
		
		# else, try retrieving it remotely
		list($remote_status, $rss, $remote_msg) = fetch_remote_rss( $url );
		if ( $remote_status and $rss ) {
			$cache->set( $url, $rss );
			return array($rss, 1, 'success.remote');
		}
		
		# else, we've failed to retrieve it remotely
		# fail back to stale cache object
		if ( $cache_status ) {
			$rss = $cache->get( $url );
			if ( $rss ) {
				return array($rss, 1, 'success.cache.stale' );
			}
		}
		
		# else, we've totally failed
		return array($remote_msg, 0, 'fail');	
	}
}

/*
* Takes a url, and returns parsed RSS object
* Internal function.
*
*/
function fetch_remote_rss ($url) {
	list($status, $data) = fetch_remote_file( $url );
	if ( $status ) {
		$rss = new MagpieRSS( $data );
		if ( $rss ) {
			return array(1, $rss, 'success');
		}
		else {
			return array(0, '', "Failed to parse RSS file: $url");
		}
	}
	else {
		return array(0, '', $data);
	}
}

/*
* Handles retrieving of arbitrary remote files
*
*/
function fetch_remote_file ($url) {
	$fp = @fopen($url, 'r');
	
	if ( ! $fp ) {
		return array(0, "Unable to open remote file: $url");
	}
	
	while ( ! feof($fp) ) {
		$data .= fread( $fp, 1024 );
	}
	fclose($fp);
	
	return array(1, $data);
}

?>
