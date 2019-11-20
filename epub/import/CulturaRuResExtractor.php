<?php

/* 
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

require_once('HtmlResExtractor.php');

class CulturaRuResExtractor extends HtmlResExtractor {
  
  const SERVICE_NAME = 'culture.ru';
  
  /**
   * Results EpubVO that extracted from URL
   * @var Array of <EpubVO> 
   */
  public $epubVOs = array();
  
  public function parse() {
    parent::parse();        
    
    $epubVO = new EpubVO();
    $epubVO->importService = self::SERVICE_NAME;
    
    $doc = $this->dom;       
    
    $xpath = new DOMXpath($doc);
    $query = '//article//section//a[@class="card-cover"]';
    
    $books = $xpath->query($query);
    print "finded books: " . $books->length ."\n";
    foreach ($books as $book) {
      
      $coverSrc = false;
      $localHref = $book->getAttribute("href");
      
      $epubVO = $this->extractFromPersonalResourcePage($this->getBasePath() . $localHref);
      
      if ($epubVO->licenseUrl != null && preg_match("#^https?\:\/\/creativecommons\.org\/licenses\/#",$epubVO->licenseUrl)) {
        array_push($this->epubVOs,$epubVO);
      } else {
        print "License '$epubVO->licenseUrl' is not Creative Commons. Import skipped\n";
      }  
      
      print "___________\n";
    }
  }
  
  /**
   * Извлекает данные со страницы ресурса (конкретной книги)
   * например с такой https://www.culture.ru/books/74/kavkazets
   * @return EpubVO заполненый объект EpubVO, но может бросить исключение.
   */
  protected function extractFromPersonalResourcePage($url) {
    print "extract from personal url: " . $url . "\n";
    $doc = new DOMDocument();
    
    
    $doc->loadHTML(mb_convert_encoding(file_get_contents($url), 'HTML-ENTITIES', 'UTF-8'));
    
    $xpath = new DOMXPath($doc);
    
    $metas = $xpath->query('//meta');
    
    $epubVO = new EpubVO();
    $epubVO->importService = self::SERVICE_NAME;
        
    foreach ($metas as $meta) {
      $property = $meta->getAttribute("property");
      $content = $meta->getAttribute("content");
      $name = $meta->getAttribute("name");      
      
      switch ($property) {
        case "og:title" :
          $epubVO->title = $meta->getAttribute("content");
          break;
        case "og:image" :
          $epubVO->coverUrl = $meta->getAttribute("content");
          break;
        case "og:description" :
          $epubVO->description = $meta->getAttribute("content");
          break;
      }
    }           
     
    $epubVO->title = trim($xpath->query('//article[@class="article"]//h1')->item(0)->textContent);
    
    $epubVO->authorName = trim($xpath->query('//div[@class="about-entity_column-primary"]//div[@class="attributes attributes__horizontal attributes__small-spacing"]/div/div[2]')->item(0)->textContent);
    
    $epubVO->fileUrl = trim($xpath->query('//div[@class="about-entity_column-primary"]//a[contains(@href,".epub")]')->item(0)->getAttribute("href")); // full epub path    
     
    $epubVO->importSource = $url;
    
    $epubVO->licenseUrl = trim($xpath->query('//article[@class="article"]//a[@rel="license"]')->item(0)->getAttribute("href"));
    
    // extract description (it can have multiple paragraphs <p></p>)
    $descriptionContent = $xpath->query('//article[@class="article"]//div[@class="styled-content_body"]')->item(0);  // all description as text
    $descriptionContentP = $xpath->query('//article[@class="article"]//div[@class="styled-content_body"]/p'); // description by paragraphs
    $epubVO->description = '';
    if ($descriptionContentP) {
      for ($i=0; $descriptionContentP->item($i); $i++) {
        $epubVO->description .= "<p>" . $descriptionContentP->item($i)->textContent . "</p>";
      }
    } else {
      $epubVO->description = trim($descriptionContent->textContent);
    }
    
    print $epubVO->title . "\n";
    
    return $epubVO;

  }
  
  /**
   * Return all Catalogs from www.culture.ru
   * DONT USE IT IF YOUR NOT HAVE RIGHTS
   */
  public static function getCatalogUrls() {
    return array(
      "Новелла" => array(
        "https://www.culture.ru/literature/books/novelette"
      ),
      "Очерк" => array(
        "https://www.culture.ru/literature/books/essay"
      ),
      "Повесть" => array(
        "https://www.culture.ru/literature/books/tale?page=1&limit=24",
        "https://www.culture.ru/literature/books/tale?page=2&limit=24",
        "https://www.culture.ru/literature/books/tale?page=3&limit=24"
      ),
      "Проза" => array(
        "https://www.culture.ru/literature/books/prose?limit=24&page=1",
        "https://www.culture.ru/literature/books/prose?limit=24&page=2",
        "https://www.culture.ru/literature/books/prose?limit=24&page=3",
        "https://www.culture.ru/literature/books/prose?limit=24&page=4",
        "https://www.culture.ru/literature/books/prose?limit=24&page=5",
        "https://www.culture.ru/literature/books/prose?limit=24&page=6",
        "https://www.culture.ru/literature/books/prose?limit=24&page=7",
        "https://www.culture.ru/literature/books/prose?limit=24&page=8",
        "https://www.culture.ru/literature/books/prose?limit=24&page=9",
        "https://www.culture.ru/literature/books/prose?limit=24&page=10",
        "https://www.culture.ru/literature/books/prose?limit=24&page=11",
        "https://www.culture.ru/literature/books/prose?limit=24&page=12",
        "https://www.culture.ru/literature/books/prose?limit=24&page=13",
        "https://www.culture.ru/literature/books/prose?limit=24&page=14"        
      ),
      "Пьеса" => array(
        "https://www.culture.ru/literature/books/play?page=1&limit=24",
        "https://www.culture.ru/literature/books/play?page=2&limit=24"
      ),
      "Поэзия" => array(
        "https://www.culture.ru/literature/books/poetry?page=1&limit=24",
        "https://www.culture.ru/literature/books/poetry?page=2&limit=24",
        "https://www.culture.ru/literature/books/poetry?page=3&limit=24",
        "https://www.culture.ru/literature/books/poetry?page=4&limit=24"
      ),
      "Поэма" => array(
        "https://www.culture.ru/literature/books/poem?page=1&limit=24",
        "https://www.culture.ru/literature/books/poem?page=2&limit=24"
      ),
      "Рассказ" => array(
        "https://www.culture.ru/literature/books/story?limit=24&page=1",
        "https://www.culture.ru/literature/books/story?limit=24&page=2",
        "https://www.culture.ru/literature/books/story?limit=24&page=3",
        "https://www.culture.ru/literature/books/story?limit=24&page=4",
        "https://www.culture.ru/literature/books/story?limit=24&page=5",
        "https://www.culture.ru/literature/books/story?limit=24&page=6",
        "https://www.culture.ru/literature/books/story?limit=24&page=7",
        "https://www.culture.ru/literature/books/story?limit=24&page=8",
        "https://www.culture.ru/literature/books/story?limit=24&page=9",
        "https://www.culture.ru/literature/books/story?limit=24&page=10"
      ),
      "Роман" => array(
        "https://www.culture.ru/literature/books/novel?page=1&limit=24",
        "https://www.culture.ru/literature/books/novel?page=2&limit=24",
        "https://www.culture.ru/literature/books/novel?page=3&limit=24"
      ),
      "Сказа" => array(
        "https://www.culture.ru/literature/books/tales?page=1&limit=24",
        "https://www.culture.ru/literature/books/tales?page=2&limit=24"
      ),
      "Статья" => array(
        "https://www.culture.ru/literature/books/article"
      ),
      "Эпопея" => array(
        "https://www.culture.ru/literature/books/epopee"
      )
      
    );
  }
}