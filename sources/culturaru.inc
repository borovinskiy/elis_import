<?php
/**
 * Import forms and proccessing from cultura.ru
 */


function import_culturaru_files() {
  require_once(drupal_get_path("module","elis_import") . "/epub/import/import.php");
}


function import_cultura_epub($node) {
  
  
  ini_set('memory_limit','2G');

  
  module_load_all();  

  ElisInstallHelper::addAllowedValueToField('node', 'epub', 'field_import_service', 'culture.ru', 'Культура.РФ');
  
  $out = '';
  
  if ($node->type == "subj") {  
    
    $out .= "<h2>Импорт документов с портала culture.ru</h2>";
    $out .= "<p>Портал culture.ru содержит раздел с художественной литературой в формате EPUB.</p>";
    $out .= '<p>При импорте произойдет импорт всех документов, расположенных по адресу <a href="https://www.culture.ru/books/">https://www.culture.ru/books/</a> с сохранением структуры поддиректорий.</p>';
    $out .= "<p>Т.е. в текущей папке создадутся подпапки с именами рубрик.</p>";
    $out .= "<p>Все импортированные документы будут автоматически опубликованы.</p>";
    $out .= "<p>Внимание! Возможность импорта в ELiS не подразумевает передачу вам авторских прав на указанные документы. Импортируя документы, вы соглашаетесь с тем, что имеете права на это, например документы размещены в Public domain.</p>";
    $out .= "<h2>Инструкция по импорту документов</h2>";
    $out .= "<ul><li>Зайдите на сервер ELiS по SSH под администратором (root).</li>";
    $out .= "    <li>Наберите команду для запуска скрипта импорта под пользователем elis: ";
    $out .= "        <pre>sudo -u elis php " . variable_get('elis_lib_dir', '/usr/lib64/elis') . "/drupal/importFromCultureRu.php catalogNid=" . $node->nid . "</pre>";
    $out .= "    </li>";
    $out .= "    <li>Дождитесь окончания импорта (может занять несколько часов) не прерывая выполнение скрипта импорта.</li>";
    $out .= "</ul>";
    $out .= "<p>В случае разрыва соединения вы можете повторить процедуру импорта, при этом документы с одинаковым названием не будут повторно импортированы.</p>";
    
  }
  return $out;
}

/**
 * Форма импорта
 * @depricated
 */
function culturaru_batch_form($form, &$form_state) {
  $form['license_allowed'] = array(
    '#title' => 'Я имею права для импорта и публикации всего контента расположенного на удаленном ресурсе',
    '#type' => 'checkbox',   
    '#required'=> TRUE,
    '#default_value' => FALSE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Импортировать',
  );
  return $form;
}

/**
 * Form submit callback.
 * @depricated
 */
function culturaru_batch_form_submit($form, &$form_state) {
  
  import_culturaru_files();        // attach required files
  
  $node = $form_state['build_info']['args'][0];
  $catalog_nid = $node->nid;
    
  $isUseSubfolders = true;  // needed create subfolders for catalogUrls
    
  $catalog_node = node_load($catalog_nid);  // root node for all subcatalogs    
  
  
  $operations = array();
  
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
    
    // Создаём массив с операциями
    foreach ($urls as $url) {
      $epubVOs = parseUrl($url);
      foreach ($epubVOs as $value) {
        $epubVO = new EpubVO();
        $epubVO = clone $value;
        $operations[] = array('culturaru_change_data', array($epubVO, $parent_nid, $catalog_nid));
      }
    }
    
  }

  batch_set(array(
    // Массив операций и их параметров
    'operations' => $operations,    
    'finished' => 'culturaru_batch_finished',    
    // Опционально, по умолчанию t('Completed @current of @total.')
    'progress_message' => 'Выполнено @current из @total.',
    // Сообщение показываемое при ошибке выполнения операции.
    // Опционально, по умолчанию t('An error has occurred.')
    'error_message' => 'Произошла ошибка.',
    'file' => drupal_get_path('module','elis_import') . '/culturaru.inc',
  ));
 
  // Если Batch API используется не из _submit функции,
  // то дополнительно нужно вызвать batch_process();
}

/**
 * Batch process callback.
 * @deprecated
 */
function culturaru_change_data($epubVO, $parent_nid, $catalog_nid, &$context) {
  
  ini_set('memory_limit','2G');
  set_time_limit(7200);
  
  try {
  
    import_culturaru_files();
    module_load_all();

    
    $parentNode = node_load($parent_nid);
    if ($parentNode !== false) {
      importAsEpubIfNotExists($epubVO,$parentNode);
      $context['message'] = 'Импортирован материал <em>' . check_plain($epubVO->title) . '</em> с источника ' . check_plain($epubVO->fileUrl);
    } else {
      $context['message'] = 'Ошибка импорта материала <em>' . check_plain($epubVO->title) . '</em> с источника ' . check_plain($epubVO->fileUrl);
    }
    
  } catch (Exception $err) {
    watchdog("elis", $err->getMessage());
  }
  
  $context['results']['url'] = $url;
  $context['results']['catalog_nid'] = $catalog_nid;
  
}


/**
 * Batch finish callback.
 * @depricated 
 */
function culturaru_batch_finished($success, $results, $operations) {
  if ($success) {
    drupal_set_message('Документы импортированы.');
  }
  else {
    drupal_set_message('Завершено с ошибками.' . print_r($results,true), 'error');
  }
}








