<?php
/**
 * Import resource from same web page 
 */

require_once 'EpubVO.php';

require_once("CulturaRuResExtractor.php");

// depricated
function parseUrlToFile($url,$outFile) {
  $epubVOs = parseUrl($url);
  $json = json_encode($epubVOs);
  file_put_contents($outFile, $json);
}

function parseUrl($url) {
    
  $extractor = new CulturaRuResExtractor();  
  $extractor->loadResourceFromUrl($url);
  $extractor->parse();        
  return $extractor->epubVOs;
  
}

function importFromEpubVO($epubVOs,$parentNode = false) {
  foreach ($epubVOs as $value) {
    $epubVO = new EpubVO();
    $epubVO = clone $value;    
    importAsEpubIfNotExists($epubVO,$parentNode);
  }
}

function importFromEpubVOFile($resFile,$parentNode) {
  print "Run importFromEpubVOFile\n";  
  $epubVOs = json_decode(file_get_contents($resFile));
  foreach ($epubVOs as $value) {
    $epubVO = new EpubVO();
    $epubVO = clone $value;    
    importAsEpubIfNotExists($epubVO,$parentNode);
  }
}

function importAsEpubIfNotExists($epubVO,$parentNode = false) {
  $result = db_select("node","n")->fields("n")->condition("title",$epubVO->title)->execute();
  $is_node_exists = false;
  $node = false; // default
  while ($res = $result->fetchObject()) {
    $node = node_load($res->nid);
    if ($node->title == $epubVO->title) {
      // check authors name in database exists
      if (isset($node->field_authors) && isset($node->field_authors[LANGUAGE_NONE]) && isset($epubVO->authorName)) {
        foreach ($node->field_authors[LANGUAGE_NONE] as $author) {          
          $term = taxonomy_term_load($author['tid']);
          if (isset($term->name)) {
            if (is_string($epubVO->authorName) && $term->name == $epubVO->authorName) {
              $is_node_exists = true;
            } else if (is_array($epubVO->authorName) && in_array($term->name,$epubVO->authorName)) {
              $is_node_exists = true;
            }
          }
        }
      }
    }
  }
  if ($is_node_exists) {
    print "node for title '" . $epubVO->title . "' is exists and not imported again\n";
  } else {    
    print "import epubVO\n";
    $node = importAsEpub($epubVO,$parentNode);
  }
  return $node;
}

function importAsEpub($epubVO,$parentNode = false) {
      
  global $user;
  
  $account = $user;
  
  $ext = "epub";    
  
  $node = new stdClass();
  $node->title = $epubVO->title;
  $node->type = "epub";
  node_object_prepare($node);
  $node->language = LANGUAGE_NONE;
  $node->uid = $account->uid;
  $node->status = 1;
  $node->promote = 0;
  $node->comment = 0;
  
  if (is_array($epubVO->authorName)) {
    ElisImportHelper::attachAuthorsToNode($node,$epubVO->authorName);
  } else if (is_string($epubVO->authorName)) {
    ElisImportHelper::attachAuthorsToNode($node,array($epubVO->authorName));
  }
  
  
  if ($epubVO->licenseUrl) { 
    ElisImportHelper::attachLicenseUrlToNode($node, $epubVO->licenseUrl);
    if (preg_match("#creativecommons\.org#",$epubVO->licenseUrl)) {
      ElisImportHelper::attachAllowUnprotectedDownloadToNode($node,"1");
    }
  }
  
  $filename = md5($epubVO->title) . "." . $ext;  
  
  drupal_mkdir('private://epub');  
  $file = file_save_data(
    file_get_contents($epubVO->fileUrl),
    'private://epub/' . $filename,
    FILE_EXISTS_REPLACE  
  );
  
  $file->uid = $account->uid;
  
  //require_once '../Epub.class.php';
  $epub = new Epub();
  $epub->loadAsEpub(drupal_realpath($file->uri));
  
  drupal_mkdir("private://covers");  
  
  // default get cover from epub else from url
  $coverFileBytes = $epub->findEpubCoverFile();
  if ($coverFileBytes === false || $coverFileBytes === NULL) {   
    print "download cover " . $epubVO->coverUrl . "\n";
    $coverFileBytes = file_get_contents($epubVO->coverUrl);    
    print "cover bytes: " . strlen($coverFileBytes) . "\n";
  }  
  $coverUri = 'private://covers/' . $file->fid;
  $coverFile = file_save_data(
    $coverFileBytes,
    $coverUri
  );
        
  
  ElisImportHelper::attachFileToNodeField($node, 'field_epub_file', $file);
  if ($coverFile != false) {
    ElisImportHelper::attachCoverFile($node, $coverFile);
  }  
  
  if ($parentNode) {
    ElisImportHelper::attachParentCatalogToNode($node, $parentNode);
  }
  
  if ($epubVO->publisherName) {
    ElisImportHelper::attachPublisherToNode($node, array($epubVO->publisherName));
  }
  
  if ($epubVO->isInteractiveEpub) {
    $node->field_epub_type[LANGUAGE_NONE][0]['value'] = 'interactive';
  }
  
  if ($epubVO->importSource) {
    ElisImportHelper::attachTextToNodeField($node, 'field_import_source', $epubVO->importSource);
  }
  
  node_save($node);  
  print "node save with nid " . $node->nid . "\n";
  return $node;
}

function import_all_epubs_to_catalog($catalog_nid) {    
  
  $isUseSubfolders = true;  // needed create subfolders for catalogUrls
  
  print "$catalog_nid\n";
  $catalog_node = node_load($catalog_nid);  // root node for all subcatalogs    
  
  foreach(CulturaRuResExtractor::getCatalogUrls() as $theme=>$urls) {
    // $theme - string of name of catalog    
    
    if ($isUseSubfolders) {
      $subjCatalog = SubjNodeBuilder::getNodeByTitleOrCreate($theme);
      if (!isset($subjCatalog->nid)) {        
        print "node with title: '" . $theme . "' is created\n";
      }
      ElisImportHelper::attachParentCatalogToNode($subjCatalog, $catalog_node); // set catalog_nid to parent for subj_nid      
      node_save($subjCatalog);
    }    
    
    $parent_nid = $isUseSubfolders ? $subjCatalog->nid : $catalog_nid;      
    
    print "find by " . count($urls) . " urls\n";
    
    foreach ($urls as $url) {
      print "grub from url: $url \n";
      $epubVOs = parseUrl($url,$resFile);                  
      importFromEpubVO($epubVOs,$parent_nid);
      unlink($resFile);      
    }
    
  }
}