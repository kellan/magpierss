<?php

/**
* A simple RSS 1.0 (and earlier) parser.  Part of MagpieRSS.
*
* MagpieRSS parse all versions of RSS with a few limitation (below) into
* a simple object, with 2 fields, the hash 'channel', and the array 'items'.
*
* MagpieRSS is inclusive, if it finds properly formatted RSS.93 tags in the
* channel definition, it will stuff them in the channel hash, if it finds 
* RSS 1.0 style modules, it will happily parse thoughs.
*
* For example this item:
*
* <item rdf:about="http://protest.net/NorthEast/calendrome.cgi?span=event&#38;ID=210257">
* <title>Weekly Peace Vigil</title>
* <link>http://protest.net/NorthEast/calendrome.cgi?span=event&#38;ID=210257</link>
* <description>Wear a white ribbon</description>
* <dc:subject>Peace</dc:subject>
* <ev:startdate>2002-06-01T11:00:00</ev:startdate>
* <ev:location>Northampton, MA</ev:location>
* <ev:enddate>2002-06-01T12:00:00</ev:enddate>
* <ev:type>Protest</ev:type>
* </item>
* 
*
* Is pushed onto the $rss->items field, as:
*
* array(
*	title => 'Weekly Peace Vigil',
*	link =>
*	'http://protest.net/NorthEast/calendrome.cgi?span=event&#38;ID=210257',
*	description => 'Wear a white ribbon',
*	dc => array (
*			subject => 'Peace'
*		),
*	ev => array (
*		startdate => '2002-06-01T11:00:00',
*		enddate => '2002-06-01T12:00:00',
*		type => 'Protest',
*		location => 'Northampton, MA'
*	)
* )
*
* NOTES:
*	Taxonomy and Content Modules:
*		My xml parsing is still pretty elementary.  The more complicated 
*		modules like the Taxonomy, and Content modules, won't be properly
*		parsed. Hopefully I'll be able to address this soon.
*
*	Image, and Textinput
*		I don't use Image and Textinput, so I didn't add them.  If someone
*		Wants to use them, let me know, and I'll take the 10 minutes to add
*		them.
*
*/

class MagpieRSS {
	var $parser;
	
	# current_item - item currently being parsed
	# items - collection of parsed items
	# channel - hash of channel fields
	#
	var $current_item	= array();
    var $items			= array();
	var $channel		= array();
	
	var $parent_field	= array('RDF');
	var $current_field	= '';
	var $current_namespace	= false;
	
	var $ERROR = "";
	
	function MagpieRSS ($source) {
		$this->parser = xml_parser_create( );
		
		# pass in parser, and a reference to this object
		# setup handlers
		#
		xml_set_object( $this->parser, &$this );
		xml_set_element_handler($this->parser, 'start_element', 'end_element');
		xml_set_character_data_handler( $this->parser, 'cdata' ); 
	
		
		$status = xml_parse( $this->parser, $source );
		
		if (! $status ) {
			$errorcode = xml_get_error_code( $this->parser );
			$errormsg = xml_error_string( $errorcode );
			$this->error( "RSS parse failure: $errormsg" );
		}
		
		xml_parser_free( $this->parser );
		
	}
	
	function start_element ($p, $element, &$attrs) {
		$element 	= strtolower( $element );
				
		# check for a namespace, and split if found
		#
		$namespace	= false;
		if ( strpos( $element, ':' ) ) {
			list($namespace, $element) = split( ':', $element, 2); 
		}
		$this->current_field = $element;
		if ( $namespace and $namespace != 'rdf' ) {
			$this->current_namespace = $namespace;
		}
		
		if ( $element == 'channel' ) {
			array_unshift( $this->parent_field, 'channel' );
		}
		elseif ( $element == 'items' ) {
			array_unshift( $this->parent_field, 'items' );
		}
		elseif ( $element == 'item' ) {
			array_unshift( $this->parent_field, 'item' );
		}
		
		
	}
	
	function end_element ($p, $element) {
		$element = strtolower($element);
							
		if ( $element == 'item' ) {		
			$this->items[] = $this->current_item;
			$this->current_item = array();
			array_shift( $this->parent_field );
		}
		elseif ( $element == 'channel' or $element == 'items' ) {
			array_shift( $this->parent_field );
		}
		
		$this->current_field = '';
		$this->current_namespace = false;
	}
	
	function cdata ($p, $text) {
		# skip item, channel, items first time we see them
		#
		if ( $this->parent_field[0] == $this->current_field or
			 ! $this->current_field ) {
			return;
		}
		elseif ( $this->parent_field[0] == 'channel') {
			if ( $this->current_namespace ) {
				$this->channel[ $this->current_namespace ][ $this->current_field ] .= $text;
			}
			else {
				$this->channel[ $this->current_field ] .= $text;
			}
		
		}
		elseif ( $this->parent_field[0] == 'item' ) {
			if ( $this->current_namespace ) {
				$this->current_item[ $this->current_namespace ][ $this->current_field ] .= $text;
			}
			else {
				$this->current_item[ $this->current_field ] .= $text;
			}
		}
	}
	
	function error ($errormsg, $lvl=E_USER_ERROR) {
		// append PHP's error message if track_errors enabled
		if ( $php_errormsg ) { 
			$errormsg .= " ($php_errormsg)";
		}
		$this->ERROR = $errormsg;
		trigger_error( $errormsg, $lvl);		
	}
		
	# debugging functions
	#
	function show_list () {
		echo "<ol>\n";
		foreach ($this->items as $item) {
			echo "<li>", $this->show_item( $item );
		}
		echo "</ol>";
	}
	
	function show_channel () {
		echo "channel:<br>";
		echo "<ul>";
		while ( list($key, $value) = each( $this->channel ) ) {
			echo "<li> $key: $value";
		}
		echo "</ul>";
	}
	
	function show_item ($item) {
		echo "item: $item[title]";
		echo "<ul>";
		while ( list($key, $value) = each($item) ) {
			if ( is_array($value) ) {
				echo "<br><b>$key</b>";
				echo "<ul>";
				while ( list( $ns_key, $ns_value) = each( $value ) ) {
					echo "<li>$ns_key: $ns_value";
				}
				echo "</ul>";
			}
			else {
				echo "<li> $key: $value";
			}
		}
		echo "</ul>";
	}
	


} # end class RSS
?>
