<?php
/*
 * Project:     MagpieRSS: a simple RSS integration tool
 * File:        rss_fetch.inc, a simple functional interface
 				to fetching and parsing RSS files, via the
				function fetch_rss()
 * Author:      Kellan Elliott-McCrea <kellan@protest.net>
 * Version:		0.2
 * License:		GPL
 *
 * The lastest version of MagpieRSS can be obtained from:
 * http://magpierss.sourceforge.net
 *
 * For questions, help, comments, discussion, etc., please join the
 * Mapgie mailing list:
 *
 */
 
// Setup MAGPIE_DIR for use on hosts that don't include
// the current path in include_path.
// with thanks to rajiv and smarty
define('DIR_SEP', DIRECTORY_SEPARATOR);

if (!defined('MAGPIE_DIR')) {
    define('MAGPIE_DIR', dirname(__FILE__) . DIR_SEP);
}

require_once( MAGPIE_DIR . 'rss_parse.inc' );
require_once( MAGPIE_DIR . 'rss_cache.inc' );

/* 
 * CONSTANTS - redefine these in your script to change the
 * behaviour of fetch_rss() currently, most options effect the cache
 *
 * MAGPIE_CACHE_ON - Should Magpie cache parsed RSS objects? 
 * For me a built in cache was essential to creating a "PHP-like" 
 * feel to Magpie, see rss_cache.inc for rationale
 *
 *
 * MAGPIE_CACHE_DIR - Where should Magpie cache parsed RSS objects?
 * This should be a location that the webserver can write to.   If this 
 * directory does not already exist Mapie will try to be smart and create 
 * it.  This will often fail for permissions reasons.
 *
 *
 * MAPGIE_CACHE_AGE - How long to store cached RSS objects? In seconds.
 *
 *
 * MAGPIE_CACHE_FRESH_ONLY - If remote fetch fails, throw error
 * instead of returning stale object?
 *
 * MAGPIE_DEBUG - Display debugging notices?
 *
*/


/*=======================================================================*\
	Function: fetch_rss: 
	Purpose:  return RSS object for the give url
			  maintain the cache
	Input:	  url of RSS file
	Output:	  parsed RSS object (see rss_parse.inc)

	NOTES ON CACHEING:  
	If caching is on (MAGPIE_CACHE_ON) fetch_rss will first check the cache.
	
	NOTES ON RETRIEVING REMOTE FILES:
	If conditional gets are on (MAGPIE_CONDITIONAL_GET_ON) fetch_rss will
	return a cached object, and touch the cache object upon recieving a
	304.
	
	NOTES ON FAILED REQUESTS:
	If there is an HTTP error while fetching an RSS object, the cached
	version will be return, if it exists (and if MAGPIE_CACHE_FRESH_ONLY is off)
\*=======================================================================*/

function fetch_rss ($url) {
	// initialize constants
	init();
	
	// if cache is disabled
	if ( !MAGPIE_CACHE_ON ) {
		list($remote_status, $rss, $msg) = fetch_remote_rss( $url );
		
		if ( $remote_status and $rss ) {
			return $rss;
		}
		else {
			trigger_error("MagpieRSS: failed to fetch $url. Cache is off");
		}
	}
	else {
		// 1. check cache
		// 2. if there is a hit, make sure its fresh
		// 3. if cached obj fails freshness check, fetch remote
		// 4. if remote fails, return stale object, or error
		
		$cache = new RSSCache( MAGPIE_CACHE_DIR, MAGPIE_CACHE_AGE );
		
		if (MAGPIE_DEBUG and $cache->ERROR) {
			trigger_error($cache->ERROR, E_USER_WARNING);
		}
		
		
		$cache_status;
		if (!$cache->ERROR) {
			# return cache HIT, MISS, or STALE
			$cache_status = $cache->check_cache( $url );
			if ( MAGPIE_DEBUG ) {
				trigger_error("Cache $cache_status for $url", E_USER_NOTICE);
			}
		}
		
		# if the rss object for this url is cached
		# and the cache is fresh, retrieve rss from cache
		#
		if ( $cache_status == 'HIT' ) {
			$rss = $cache->get( $url );
			if ( $rss ) {
				$rss->from_cache = 1;
				return $rss;
			}
		}
		
		# else, try retrieving it remotely
		list($remote_status, $rss, $remote_msg) = _fetch_remote_rss( $url );
		if ( $remote_status and $rss ) {
			if ( ! $rss->ERROR ) {
				$cache->set( $url, $rss );
			}
			return $rss;
		}
		
		# else, we've failed to retrieve it remotely
		# fail back to stale cache object
		if ( $cache_status == 'STALE') {
			$rss = $cache->get( $url );
			if ( $rss ) {
				$rss->from_cache = 1;
				$rss->stale = 1;
				if ( MAGPIE_DEBUG ) {
					trigger_error("Cache STALE for $url", E_USER_NOTICE);
				}
				return $rss;
			}
		}
		
		trigger_error($remote_msg, E_USER_ERROR);
		
		# else, we've totally failed
		return 0;	
	}
}

/*=======================================================================*\
	Function:	_fetch_remote_rss
	Purpose:	retrieve and parse the remote RSS file
	Input:		url of the remote file
	Output:		parsed RSS object	
\*=======================================================================*/
function _fetch_remote_rss ($url) {
	list($status, $data) = _fetch_remote_file( $url );
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

/*=======================================================================*\
	Function:	_fetch_remote_rss
	Purpose:	retrieve an arbitrary remote file
	Input:		url of the remote file
	Output:		an HTTP response object (see Snoopy.class.inc)	
\*=======================================================================*/
function _fetch_remote_file ($url) {
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

/*=======================================================================*\
	Function:	init
	Purpose:	setup constants with default values
				check for user overrides
\*=======================================================================*/
function init () {
	if ( defined('MAGPIE_INITALIZED') ) {
		return;
	}
	else {
		define('MAGPIE_INITALIZED', 1);
	}
	
	if ( !defined('MAGPIE_CACHE_ON') ) {
		define('MAGPIE_CACHE_ON', 1);
	}

	if ( !defined('MAGPIE_CACHE_DIR') ) {
		define('MAGPIE_CACHE_DIR', './cache');
	}

	if ( !defined('MAGPIE_CACHE_AGE') ) {
		define('MAGPIE_CACHE_AGE', 60*60); // one hour
	}

	if ( !defined('MAGPIE_CACHE_FRESH_ONLY') ) {
		define('MAGPIE_CACHE_FRESH_ONLY', 0);
	}

	if ( !defined('MAGPIE_DEBUG') ) {
		define('MAGPIE_DEBUG', 0);
	}
}

?>
