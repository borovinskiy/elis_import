<?php

/* 
 * Resource extractor from same url
 */

require_once('EpubVO.php');

abstract class ResExtractor {
  
  // active resource url
  protected $url;
  
  protected $parsedUrl;
  
  // original resource string
  protected $originalFile;
    
  /**
   * Load remote page
   * @param String $url - http url of remote page with resources
   */
  public function loadResourceFromUrl($url) {
    $this->parsedUrl = parse_url($url);
    $this->url = $url;
    $this->originalFile = @file_get_contents($this->url);            
  }
  
  /**
   * Parse loaded resource
   */
  public function parse() {
    
  }
  
}
