<?php

/* 
 * EpubVO for send info
 */

class EpubVO {
  /**
   * Title of EPUB
   */
  public $title;
  
  /**
   * Author name as simple string
   * @var String
   */
  public $authorName;
  
  /**
   * Text description
   * Nullable
   */
  public $description;
  
  /**
   * External cover url
   */
  public $coverUrl;
  
  /**
   * External EPUB file url
   * @var String
   */
  public $fileUrl;  
  
  /**
   * Source (like canonical URL) of EPUB from it imported. Typically is http-page with resource description
   * @var String
   */
  public $importSource;
  
  /**
   * Import Service machine name like 'temocenter', 'culture.ru' of source from it imported
   * @var String
   */
  public $importService;
  
  /**
   * License URL
   * @var String
   */
  public $licenseUrl;
  
  /**
   * Epub is interactive or not
   * @var bool 
   */
  public $isInteractiveEpub = false;
  
  /**
   * Name of publisher
   * @var string publisher text name
   */
  public $publisherName;
}