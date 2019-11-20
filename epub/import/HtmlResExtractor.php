<?php

/* 
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

require_once('ResExtractor.php');

class HtmlResExtractor extends ResExtractor {
  
  protected $dom;    
  
  /**
   * BasePath (without real path)
   */
  protected function getBasePath() {
    $url = $this->parsedUrl;    
    $basePath =  $url['scheme'] . "://" . $url['host'];
    $basePath .= isset($url['port']) ? ":" . $url['port'] : "";    
    return $basePath;
  }
  
  protected function getBaseUrl() {
    $baseUrl = $this->getBasePath();
    $baseUrl .= $this->parsedUrl['path'];
    return $baseUrl;
  }


  public function parse() {
    
    libxml_use_internal_errors(true);
    
    $this->dom = new DOMDocument();
    $this->dom->preserveWhiteSpace = false;
    $this->dom->loadHTMLFile($this->url);    
  }
}