<?php
/*
 * Project:     MagpieRSS: a simple RSS integration tool
 * File:        rss_parse.inc  - parse an RSS or Atom feed
 *				return as a simple object.
 *
 * Handles RSS 0.9x, RSS 2.0, RSS 1.0, and Atom 0.3
 *
 * The lastest version of MagpieRSS can be obtained from:
 * http://magpierss.sourceforge.net
 *
 * For questions, help, comments, discussion, etc., please join the
 * Magpie mailing list:
 * magpierss-general@lists.sourceforge.net
 *
 * Author:      Kellan Elliott-McCrea <kellan@protest.net>
 * Version:		0.6a
 * License:		GPL
 *
 *
 *  ABOUT MAGPIE's APPROACH TO PARSING:
 *   - Magpie is based on expat, an XML parser, and therefore will only parse
 *     valid XML files.  This includes all properly constructed RSS or Atom.
 *
 *   - Magpie is an inclusive parser.  It will include any elements that 
 *     it can turn into a key value pair in the parsed feed object it returns. 
 *      
 *   - Magpie supports namespaces, and will return any elements found in a 
 *     namespace in a sub-array, with the key point to that array being the 
 *     namespace prefix.  
 *     (e.g. if an item contains a <dc:date> element, then that date can 
 *     be accessed at $item['dc']['date']
 *      
 *   - Magpie supports nested elements by combining the names.  If an item 
 *     includes XML like:
 *      <author>
 *        <name>Kellan</name>
 *      </author>
 *      
 *    The name field is accessible at $item['author_name']
 *  
 *   - Magpie makes no attempt validate a feed beyond insuring that it
 *     is valid XML.   
 *     RSS validators are readily available on the web at:
 *       http://feeds.archive.org/validator/
 *       http://www.ldodds.com/rss_validator/1.0/validator.html
 *
 *
 * EXAMPLE PARSED RSS ITEM:
 *
 * Magpie tries to parse RSS into easy to use PHP datastructures.
 *
 * For example, Magpie on encountering (a rather complex) RSS 1.0 item entry:
 *
 * <item rdf:about="http://protest.net/NorthEast/calendrome.cgi?span=event&#38;ID=210257">
 *   <title>Weekly Peace Vigil</title>
 *   <link>http://protest.net/NorthEast/calendrome.cgi?span=event&#38;ID=210257</link>
 *   <description>Wear a white ribbon</description>
 *   <dc:subject>Peace</dc:subject>
 *   <ev:startdate>2002-06-01T11:00:00</ev:startdate>
 *   <ev:location>Northampton, MA</ev:location>
 *   <ev:type>Protest</ev:type>
 * </item>
 * 
 * Would transform it into the following associative array, and push it
 * onto the array $rss-items
 *
 * array(
 *	title => 'Weekly Peace Vigil',
 *	link => 'http://protest.net/NorthEast/calendrome.cgi?span=event&#38;ID=210257',
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
 *
 *
 *  A FEW NOTES ON PARSING Atom FEEDS
 *
 *  Atom support is considered alpha.  Atom elements will be often be available
 *  as their RSS equivalent, summary is available as description for example.
 *
 *  Elements of mode=xml, as flattened into a single string, just as if they
 *  had been wrapped in a CDATA container.
 *
 *  See:  http://laughingmeme.org/archives/001676.html
 *
 */

define('RSS', 'RSS');
define('ATOM', 'Atom');


class MagpieRSS {
	/*
	 * Hybrid parser, and object.  (probably a bad idea! :)
	 *
	 * Useage Example:
	 *
	 * $some_rss = "<?xml version="1.0"......
	 *
	 * $rss = new MagpieRSS( $some_rss );
	 *
	 * // print rss chanel title
	 * echo $rss->channel['title'];
	 *
	 * // print the title of each item
	 * foreach ($rss->items as $item ) {
	 *	  echo $item[title];
	 * }
	 *
	 * see: rss_fetch.inc for a simpler interface
	 */
	 
	var $parser;
	
	var $current_item	= array();	// item currently being parsed
    var $items			= array();	// collection of parsed items
	var $channel		= array();	// hash of channel fields
	var $textinput		= array();
	var $image			= array();
	var $feed_type;
	var $feed_version;

	// parser variables
	var $stack				= array(); // parser stack
	var $inchannel			= false;
	var $initem 			= false;
	var $incontent			= false; // if in Atom <content mode="xml"> field 
	var $intextinput		= false;
	var $inimage 			= false;
	var $current_field		= '';
	var $current_namespace	= false;
	
	var $ERROR = "";
	
/*======================================================================*\
    Function: MagpieRSS
    Purpose:  Constructor, sets up XML parser,parses source,
			  and populates object.. 
	Input:	  String containing the RSS to be parsed
\*======================================================================*/
	function MagpieRSS ($source) {
		
		# if PHP xml isn't compiled in, die
		#
		if (!function_exists('xml_parser_create')) {
			$this->error( "Failed to load PHP's XML Extension. " . 
						  "http://www.php.net/manual/en/ref.xml.php",
						   E_USER_ERROR );
		}
		
		$parser = @xml_parser_create();
		
		if (!is_resource($parser))
		{
			$this->error( "Failed to create an instance of PHP's XML parser. " .
						  "http://www.php.net/manual/en/ref.xml.php",
						  E_USER_ERROR );
		}

		
		$this->parser = $parser;
		
		# pass in parser, and a reference to this object
		# setup handlers
		#
		xml_set_object( $this->parser, $this );
		xml_set_element_handler($this->parser, 
				'feed_start_element', 'feed_end_element' );
						
		xml_set_character_data_handler( $this->parser, 'feed_cdata' ); 
	
		$status = xml_parse( $this->parser, $source );
		
		if (! $status ) {
			$errorcode = xml_get_error_code( $this->parser );
			if ( $errorcode != XML_ERROR_NONE ) {
				$xml_error = xml_error_string( $errorcode );
				$error_line = xml_get_current_line_number($this->parser);
				$error_col = xml_get_current_column_number($this->parser);
				$errormsg = "$xml_error at line $error_line, column $error_col";

				$this->error( $errormsg );
			}
		}
		
		xml_parser_free( $this->parser );
	}
	
	function feed_start_element($p, $element, &$attrs) {
		$el = $element = strtolower($element);
		$attrs = array_change_key_case($attrs, CASE_LOWER);
		
		// check for a namespace, and split if found
		$ns	= false;
		if ( strpos( $element, ':' ) ) {
			list($ns, $el) = split( ':', $element, 2); 
		}
		if ( $ns and $ns != 'rdf' ) {
			$this->current_namespace = $ns;
		}
			
		# if feed type isn't set, then this is first element of feed
		# identify feed from root element
		#
		if (!isset($this->feed_type) ) {
			if ( $el == 'rdf' ) {
				$this->feed_type = RSS;
				$this->feed_version = '1.0';
			}
			elseif ( $el == 'rss' ) {
				$this->feed_type = RSS;
				$this->feed_version = $attrs['version'];
			}
			elseif ( $el == 'feed' ) {
				$this->feed_type = ATOM;
				$this->feed_version = $attrs['version'];
				$this->inchannel = true;
			}
			return;
		}
	
		if ( $el == 'channel' ) 
		{
			$this->inchannel = true;
		}
		elseif ($el == 'item' or $el == 'entry' ) 
		{
			$this->initem = true;
			if ( isset($attrs['rdf:about']) ) {
				$this->current_item['about'] = $attrs['rdf:about'];	
			}
		}
		
		// if we're in the default namespace of an RSS feed,
		//  record textinput or image fields
		elseif ( 
			$this->feed_type == RSS and 
			$this->current_namespace == '' and 
			$el == 'textinput' ) 
		{
			$this->intextinput = true;
		}
		
		elseif (
			$this->feed_type == RSS and 
			$this->current_namespace == '' and 
			$el == 'image' ) 
		{
			$this->inimage = true;
		}
		
		elseif ( $this->feed_type == ATOM and $el == 'content') 
		{
			$this->incontent = true;
		}
		
		// if in an Atom <content> field treat tags as text
		elseif ($this->feed_type == ATOM and $this->incontent and $this->initem) 
		{
			$attrs_str = join(' ', 
					array_map('map_attrs', 
					array_keys($attrs), 
					array_values($attrs) ) );
					
			$this->append( 
					$this->current_item['atom_content'], 
					"<$element $attrs_str>"  );
					
			array_unshift( $this->stack, $el );
		}
		
		// Atom support many links per containging element.
		// Magpie treats link elements of type rel='alternate'
		// as being equivalent to RSS's simple link element.
		//
		elseif ($this->feed_type == ATOM and $el == 'link' ) 
		{
			if ( isset($attrs['rel']) and $attrs['rel'] == 'alternate' ) 
			{
				$link_el = 'link';
			}
			else {
				$link_el = 'link_' . $attrs['rel'];
			}
			
			if ( $this->initem ) {
				if ( $this->current_namespace ) {
					$this->current_item[ $this->current_namespace ][$link_el] =
					 	$attrs['href'];
				}
				else {
					$this->current_item[$link_el] = $attrs['href'];
				}
			}
			elseif ( $this->inchannel ) {
				if ( $this->current_namespace ) {
					$this->channel[ $this->current_namespace ][ $link_el ] =
						$attrs['href'];
				}
				else {
					$this->channel[$link_el] = $attrs['href'];
				}	
			}
		}
		// set stack[0] to current element
		else {
			array_unshift($this->stack, $el);
		}
	}
	

	
	function feed_cdata ($p, $text) {
		
		if ($this->feed_type == ATOM and $this->incontent and $this->initem) 
		{
			$this->append( $this->current_item['atom_content'], $text );
		}
		else
		if ( $this->initem ) 
		{
			$current_el = join('_', array_reverse($this->stack));
			if ( $this->current_namespace ) 
			{
				$this->append($this->current_item[ $this->current_namespace ][ $current_el ],
					$text);
			}
			else {
				$this->append($this->current_item[ $current_el ], $text );
			}
		}
		elseif ( $this->intextinput ) 
		{
			$current_el = join('_', array_reverse($this->stack));
			if ( $this->current_namespace ) 
			{
				$this->append($this->textinput[ $this->current_namespace ][ $current_el ],
					$text);
			}
			else {
				$this->append($this->textinput[ $current_el ], $text );
			}
		}
		elseif ( $this->inimage ) 
		{
			$current_el = join('_', array_reverse($this->stack));
			if ( $this->current_namespace ) 
			{
				$this->append($this->image[ $this->current_namespace ][ $current_el ],
					$text);
			}
			else {
				$this->append($this->image[ $current_el ], $text );
			}
		}
		elseif ($this->inchannel ) 
		{
			$current_el = join('_', array_reverse($this->stack));
			if ( $this->current_namespace ) 
			{
				$this->append($this->channel[ $this->current_namespace ][ $current_el ],
					$text);
			}
			else {
				$this->append($this->channel[ $current_el ], $text );
			}
		}
	}
	
	function feed_end_element ($p, $el) {
		$el = strtolower($el);
		
		if ( $el == 'item' or $el == 'entry' ) 
		{
			$this->items[] = $this->current_item;
			$this->current_item = array();
			$this->initem = false;
		}
		elseif ($this->feed_type == RSS and $this->current_namespace == '' and $el == 'textinput' ) 
		{
			$this->intextinput = false;
		}
		elseif ($this->feed_type == RSS and $this->current_namespace == '' and $el == 'image' ) 
		{
			$this->inimage = false;
		}
		elseif ($this->feed_type == ATOM and $el == 'content') 
		{	
			$this->incontent = false;		
		}
		elseif ($el == 'channel' or $el == 'feed' ) 
		{
			$this->inchannel = false;
		}
		elseif ($this->feed_type == ATOM and $this->incontent and $this->initem ) {
			// balance tags properly
			// note:  don't think this is actually neccessary
			if ( $this->stack[0] == $el ) 
			{
				$this->append($this->current_item['atom_content'], "</$el>");
			}
			else {
				$this->append($this->current_item['atom_content'], "<$el />");
			}
			array_shift( $this->stack );
		}
		else {
			if ( $el == 'description' and !$this->current_namespace) {
				if ($this->initem)
				{
					$this->current_item['summary'] = $this->current_item['description'];
				}
				elseif ($this->inchannel) {
					$this->channel['tagline'] = $this->channel['description'];
				}
			}
			elseif ( $el == 'summary' and !$this->current_namespace) {
				if ($this->initem) {
					$this->current_item['description'] = $this->current_item['summary'];
				}
			}
			elseif ($el == 'tagline' and !$this->current_namespace) {
				if ( $this->inchannel ) {
					$this->channel['description'] = $this->channel['tagline'];
				}
			}
			elseif ($this->initem and $el == 'content:encoded' ) {
				$this->current_item['atom_content'] = $this->current_item['content']['encoded'];
			}
			array_shift( $this->stack );
		}
		
		$this->current_namespace = false;
	}
	
	function append (&$str1, $str2="") {
		if (!isset($str1) ) {
			$str1="";
		}
		$str1 .= $str2;
	}
	
	function error ($errormsg, $lvl=E_USER_WARNING) {
		// append PHP's error message if track_errors enabled
		if ( $php_errormsg ) { 
			$errormsg .= " ($php_errormsg)";
		}
		$this->ERROR = $errormsg;
		if ( MAGPIE_DEBUG ) {
			trigger_error( $errormsg, $lvl);		
		}
		else {
			error_log( $errormsg, 0);
		}
	}
	
	function is_rss () {
		if ( $this->feed_type == RSS ) {
			return $this->feed_version;	
		}
		else {
			return false;
		}
	}
	
	function is_atom() {
		if ( $this->feed_type == ATOM ) {
			return $this->feed_version;
		}
		else {
			return false;
		}
	}

/*======================================================================*\
	EVERYTHING BELOW HERE IS FOR DEBUGGING PURPOSES
\*======================================================================*/
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

/*======================================================================*\
	END DEBUGGING FUNCTIONS	
\*======================================================================*/
	


} # end class RSS

function map_attrs($k, $v) {
	return "$k=\"$v\"";
}


?>
