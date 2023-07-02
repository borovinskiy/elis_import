<?php 

/** 
 * Import resources from http://store.temocenter.ru
 */

function import_temocenter_files() {
  require_once(drupal_get_path("module","elis_import") . "/epub/import/import.php");
}

/**
 * Create navigation before run import
 */
function create_temocenter_navigation() {
  $vocabulary = taxonomy_vocabulary_machine_name_load('classification');
  if ($vocabulary) {
    /*
    $rootTerm = ElisImportHelper::insertTermsToVocabulariIfNotExists(array('Учебный ресурс'),$vocabulary)[0];
    ElisImportHelper::insertTermsChainToVocabulary(array('Информационный ресурс'), $vocabulary, $rootTerm);
    ElisImportHelper::insertTermsChainToVocabulary(array('Практикум'), $vocabulary, $rootTerm);
    ElisImportHelper::insertTermsChainToVocabulary(array('Контроль'), $vocabulary, $rootTerm); */
    $_terms = ElisImportHelper::insertTermsChainToVocabulary(array('Учебный ресурс'), $vocabulary, $term_resource, 'resource/learning');
    $term_learning_resource = end($_terms);
    ElisImportHelper::insertTermsChainToVocabulary(array('Учебный информационный ресурс'), $vocabulary, $term_learning_resource, 'resource/learning/info');
    ElisImportHelper::insertTermsChainToVocabulary(array('Практикум'), $vocabulary, $term_learning_resource, 'resource/learning/practicum');
    ElisImportHelper::insertTermsChainToVocabulary(array('Контроль знаний'), $vocabulary, $term_learning_resource, 'resource/learning/control/knowledge');
  }
  
}

function import_all_temocenter_to_catalog($catalogNid) {
  
  create_temocenter_navigation();
  
  $catalog_node = node_load($catalogNid);
  if ($catalog_node == false || $catalog_node->type != 'subj') {
    die("error: imported catalog Nid is not have type 'subj'");
  }
  $temocenterExtractor = new TemocenterExtractor();
  $temocenterExtractor->rootCatalogNode = $catalog_node;
  $temocenterExtractor->importAll();
}

class TemocenterExtractor {
  public $offset = 0;
  private $limit = 500;
  const SERVICE_NAME = 'temocenter';
  private $requestTimeoutMs = 1000;
  private $baseUrl = 'http://store.temocenter.ru';
  public $templateUrl = 'http://store.temocenter.ru/catalog.html?treeid=0&branch=0&mtype=all&stype=sa&mstart={offset}&mlimit={limit}&agefrom=3&ageto=18%2B';
  public $rootCatalogNode = false;
  
  public $isImportOnlyBrowserResource = true;
  public $isImportResourcesWithFlash = false;
  public $isImportResourcesWithOmsPlayer = false;
  public $isImportMaps = false;
  
  public function importAll() {
    if (!module_exists('elis_web')) die('For import from temocenter you must enabled eils_web module.');
    
    do  {
      print "Run iteration step with offset " . $this->offset . "...";
      $importVOs = $this->findAndImportDocumentsStep();
    } while (count($importVOs));
  }
  
  private function findAndImportDocumentsStep() {
    global $user;  
    $vocabulary_classification = taxonomy_vocabulary_machine_name_load('classification');
    $doc = new DOMDocument();
    usleep($this->requestTimeoutMs * 1000);
    $doc->loadHTML(mb_convert_encoding(file_get_contents($this->getUrlList()), 'HTML-ENTITIES', 'UTF-8'));
    $this->offset += $this->limit;
    $xpath = new DOMXPath($doc);
    $importVOs = array();
    $elements = $xpath->query('//div[@class="kris-resulrmod-box"]');
    if (!is_null($elements)) {
      foreach ($elements as $box) {   // document iterator
        
        $isImportAllowed = true;
        
        $isFlashRequired = false;
        $isBrowserRequired = false;
        $isOmsPlayerRequired = false;
        $isResourceHaveMap = false;
        
        $fileUrl = $this->baseUrl . trim($xpath->query('.//div[@class="kris-resulrmod-info-txt download_link_tmp_place"]/a[@data-hintinfo="Скачать"]',$box)->item(0)->getAttribute("href"));
        
        $title = trim($xpath->query('.//h2[@class="kris-resulrmod-info__h2"]/a',$box)->item(0)->textContent);
        
        if (preg_match("#карт#i", $title)) { // find map in title
          $isResourceHaveMap = true;
        }
        
        $modforbrowser = $xpath->query('.//div[@class="kris-modfor-browser"]/span/img',$box);
        if ($modforbrowser) {
          foreach ($modforbrowser as $forbrowser) {
            if (preg_match("#браузер#", $forbrowser->getAttribute("data-hintinfo"))) {
              $isBrowserRequired = true;
            }
            if (preg_match("#Flash#", $forbrowser->getAttribute("data-hintinfo"))) {
              $isFlashRequired = true;
            }
            if ($forbrowser->getAttribute("data-hintinfo") == "Воспроизводится проигрывателем") {
              $isOmsPlayerRequired = true;
            }
          }
          
          if ($this->isImportOnlyBrowserResource && !$isBrowserRequired) $isImportAllowed = false;
          if (!$this->isImportResourcesWithOmsPlayer && $isOmsPlayerRequired) $isImportAllowed = false;
          if (!$this->isImportResourcesWithFlash && $isFlashRequired) $isImportAllowed = false;
          if (!$this->isImportMaps && $isResourceHaveMap) $isImportAllowed = false;
          print "is import allowed: " . $isImportAllowed . " browser: " . $isBrowserRequired . " flash: " . $isFlashRequired . " oms: " . $isOmsPlayerRequired . " map: " . $isResourceHaveMap . "\n";
        }
        
        
        
        if ($isImportAllowed && preg_match("#\.zip$#",$fileUrl)) {  // import only resources for browser (with zip extension)
          
          $parent_node = $this->rootCatalogNode; // parent node for this importVO
          $doctype = false;
          $classification_term_names = array();
          
          $importVO = new stdClass();
          
          $importVO->title = trim($xpath->query('.//h2[@class="kris-resulrmod-info__h2"]/a',$box)->item(0)->textContent);
          $importVO->description = trim($xpath->query('.//div[@class="kris-resulrmod-info-txt"]',$box)->item(0)->textContent);
          $importVO->coverUrl = $this->baseUrl . trim($xpath->query('.//div[@class="kris-resumod-img"]/a/img',$box)->item(0)->getAttribute("src"));
          $importVO->fileUrl = $fileUrl;
          $importVO->licenseUrl = 'http://store.temocenter.ru/about.html';
          //$importVO->isInteractiveEpub = true;
          $resourceUrl = trim($xpath->query('.//h2[@class="kris-resulrmod-info__h2"]/a',$box)->item(0)->getAttribute("href"));
          // get page from resource url like http://store.temocenter.ru/res.html?rid=6227
          if ($resourceUrl) {
            $importVO->importSource = $this->baseUrl . $resourceUrl;
            $resourceDoc = new DOMDocument();
            usleep($this->requestTimeoutMs * 1000);
            $resourceDoc->loadHTML(mb_convert_encoding(file_get_contents($importVO->importSource), 'HTML-ENTITIES', 'UTF-8'));
            $resourceXPath = new DOMXPath($resourceDoc);
            
            
            // find same metadata
            $moddopinfo = $resourceXPath->query('//div[@class="kris-infomod-body-r"]/div[@class="kris-moddopinfo"]');
            if ($moddopinfo) {
              foreach ($moddopinfo as $dopinfo) {
                $text_str = $dopinfo->textContent;
                // find authors
                if (preg_match("#\s*Автор учебных материалов:\s(.*)\s*$#",$text_str,$matches)) {
                  $authors_novalidated = explode(",",trim($matches[1]));
                  $importVO->authorsName = array();
                  foreach ($authors_novalidated as $author) {
                    
                    $is_valid = true;
                    
                    if (preg_match("#наук#i",$author)) $is_valid = false;  // исключаем "канд. ист. наук, Иванов И.И."
                    if (preg_match("#учитель#i",$author)) $is_valid = false;
                    if (preg_match("#проф\.#i",$author)) $is_valid = false;
                    if (preg_match("#\s*(.+)\s(.+)\s(.+)\s(.+)\s*#i",$author)) $is_valid = false; // 3 or more spaces beetwen books "sdfs sdfs sdfsd sdfsd"
                    
                    if ($is_valid) $importVO->authorsName[] = trim($author);
                  }
                }
                // find publisher
                if (preg_match("#Издатель:(.*)$#",$text_str,$matches)) {
                  $importVO->publisherName = trim($matches[1]); //explode(",",trim($matches[1]));
                }
                
                if (preg_match("#\s*Ограничение доступности:\s(.*)\s*#",$text_str,$matches)) {
                  $term_names_accessability = explode(",",trim($matches[1]));
                  foreach ($term_names_accessability as $term_accessability) {
                    if (!in_array($term_accessability, $term_names_classification)) {
                      $classification_term_names[] = $term_accessability;
                    }
                  }
                }
              }
            }
            
            // find classifications
            $modlinkblue = $resourceXPath->query('//div[@class="kris-infomod-body-r"]/div[@class="kris-modlinkblue"]/a');
            $classification_terms = array();
            
            if ($modlinkblue && $vocabulary_classification) {
              foreach ($modlinkblue as $linkblue) {
                // find class
                if (preg_match("#^\s*(\d+)\s*$#",$linkblue->textContent, $matches)) {
                  $term_name = $matches[1] . ' класс';
                  $classification_term_names[] = $term_name;
                }
                if (preg_match("#^\s*Дошкольное образование\s*$#",$linkblue->textContent, $matches)) {
                  $classification_term_names[] = "Дошкольное образование";
                }
                if (preg_match("#^\s*Дополнительное образование\s*$#",$linkblue->textContent, $matches)) {
                  $classification_term_names[] = "Дополнительное образование";
                }
              }
            }  
            
            // extract navigation
            $navigation = $resourceXPath->query('//div[@class="kris-infomod-body-r"]/div[@class="kris-modlinkbleck"]/a');
            if ($navigation && $this->rootCatalogNode) {
              $catalog_string_tree = array();
              foreach ($navigation as $nav) {
                $catalog_string_tree[] = $nav->textContent;
              }
              
              $parent_node = false;
              $parent_nodes_chain = ElisImportHelper::insertTextChainAsCatalogTree($catalog_string_tree, $this->rootCatalogNode); // create navigation
              
              if (count($parent_nodes_chain)) {
                $parent_node = ($parent_nodes_chain[count($parent_nodes_chain)-1]);
              }
            }
            
            $doctype = trim($resourceXPath->query('//div[@class="kris-fortype"]/img')->item(0)->getAttribute('data-hintinfo')); // "Практикум", "Информационный ресурс", "Контроль"
            
            switch ($doctype) {
              case "Практикум":
                $doctype = "Практикум";
                break;
              case "Информационный ресурс":
                $doctype = "Учебный информационный ресурс";
                break;
              case "Контроль":
                $doctype = "Контроль знаний";
                break;
            }
            
          }
          
          $exists_node = find_node_if_exists_elis_import($importVO);
          
          if ($exists_node !== false) {
            print "node is exists";
            continue;
          }
          
          //$import_node = importAsEpubIfNotExists($importVO, $parent_node);
          
          $account = $user;
  
          $ext = "zip";    
  
          $import_node = new stdClass();
          $import_node->title = $importVO->title;
          $import_node->type = "web";
          node_object_prepare($import_node);
          $import_node->language = LANGUAGE_NONE;
          $import_node->uid = $account->uid;
          $import_node->status = 1;
          $import_node->promote = 0;
          $import_node->comment = 0;
          
          
          if ($resourceUrl) {
            ElisImportHelper::attachTextToNodeField($import_node, 'field_import_source', $importVO->importSource);
          }
          // ElisImportHelper::attachTextToNodeField($import_node, 'field_import_service', self::SERVICE_NAME);
          
          if ($doctype) {
            ElisImportHelper::attachClassificationTermsToNodeIfExists($import_node, $doctype);
          }
          
          ElisImportHelper::attachBooleanToNodeField($import_node, 'field_allow_unprotected_download', '0');  // we have a bug that resource download with zip extension
          
          if (isset($importVO->authorsName) && count($importVO->authorsName)) {
            ElisImportHelper::attachAuthorsToNode($node,$importVO->authorsName);
          }
          
          ElisImportHelper::attachLicenseUrlToNode($node, $importVO->licenseUrl);
          
          if (count($classification_term_names)) {
            ElisImportHelper::attachClassificationTermsToNodeIfExists($import_node, $classification_term_names);
          }
          
          $filename = md5($importVO->fileUrl) . "." . $ext;  
  
          drupal_mkdir('private://imported');  
          $file = file_save_data(
            file_get_contents($importVO->fileUrl),
            'private://imported/' . $filename,
            FILE_EXISTS_REPLACE  
          );
          $file->uid = $account->uid;
          $realpath = drupal_realpath($file->uri);
          drupal_mkdir("private://covers");
          $coverFileBytes = file_get_contents($importVO->coverUrl);    
          print "cover bytes: " . strlen($coverFileBytes) . "\n";
          $coverUri = 'private://covers/' . $file->fid;
          $coverFile = file_save_data(
            $coverFileBytes,
            $coverUri
          );
          ElisImportHelper::attachFileToNodeField($import_node, 'field_file', $file);
          if ($coverFile != false) {
            ElisImportHelper::attachCoverFile($import_node, $coverFile);
          }
          if ($parent_node) {
            ElisImportHelper::attachParentCatalogToNode($import_node, $parent_node);
          }
          if ($importVO->publisherName) {
            ElisImportHelper::attachPublisherToNode($import_node, array($importVO->publisherName));
          }
          
          node_save($import_node);
          print "___________\n";

          $importVOs[] = $importVO;
        }
      }
    }
    return $importVOs;
  }
  
  private function getUrlList() {
    $url = preg_replace("#{offset}#", $this->offset, $this->templateUrl);
    $url = preg_replace("#{limit}#", $this->limit, $url);
    return $url;
  }
}
