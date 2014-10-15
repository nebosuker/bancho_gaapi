<?php
/**
 * @file
 * 統計出力用
 */
// GA API用コード読み込み
module_load_include('inc', 'bancho_gaapi', 'gaapi_core');

define('BANCHO_GAAPI_STATS_RPP', 50);

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_statistics_main() {

  // querystringをきれいにする
  bancho_gaapi_cleanup_params();

  $a1 = arg(4);
  $s = '';
  $vocabularies = array();
  foreach (taxonomy_get_vocabularies() as $k => $v) {
//    if (count($v->nodes) > 0) {
    $vocabularies[$k] = $v;
//    }
  }
//dpm($vocabularies);
  //print '<pre>'.nl2br(check_plain(print_r(taxonomy_get_vocabularies(), true))).'</pre>';
  // タイプ別
  if ($a1 == 'types') {
    // タイトル設定
    $title = 'ノードタイプごと';
    drupal_set_title(drupal_get_title() . ' - ' . $title);
    // コンテンツ
    $s = bancho_gaapi_statistics_data($a1);
  }
  elseif ($a1 == 'nodes') {
    // タイトル設定
    $title = '記事ごと';
    drupal_set_title(drupal_get_title() . ' - ' . $title);
    // コンテンツ
    $s = bancho_gaapi_statistics_data($a1);
  }
  elseif ($a1 == 'users') {
    // タイトル設定
    $title = '筆者ごと';
    drupal_set_title(drupal_get_title() . ' - ' . $title);
    // コンテンツ
    $s = bancho_gaapi_statistics_data($a1);
  }
  elseif ($a1 == 'editors') {
    // タイトル設定
    $title = '担当編集者ごと';
    drupal_set_title(drupal_get_title() . ' - ' . $title);
    // コンテンツ
    $s = bancho_gaapi_statistics_data($a1);
  }
  elseif ($a1 == 'planners') {
    // タイトル設定
    $title = '企画編集者ごと';
    drupal_set_title(drupal_get_title() . ' - ' . $title);
    // コンテンツ
    $s = bancho_gaapi_statistics_data($a1);
  }
  // vid別
  elseif (is_numeric($a1)) {
    if (in_array($a1, array_keys($vocabularies))) {
      // タイトル設定
      $title = check_plain($vocabularies[$a1]->name);
      drupal_set_title(drupal_get_title() . ' - ' . $title);
      // コンテンツ
      $s = bancho_gaapi_statistics_data($a1);
    }
  }
  // 指定なし→メニュー
  else {

    $vocab_items = array();
    foreach ($vocabularies as $tid => $vocabulary) {
      $vocab_items[] = l($vocabulary->name, $_GET['q'] . '/' . $tid);
    }

    $misc_items = array(
      l('ノードタイプごと', $_GET['q'] . '/types'),
      l('記事ごと', $_GET['q'] . '/nodes'),
    );
    
    $user_items = array(
      l('筆者ごと', $_GET['q'] . '/users'),
      l('担当編集者ごと', $_GET['q'] . '/editors'),
      l('企画編集者ごと', $_GET['q'] . '/planners'),
    );

    $s = sprintf(
      "<p>統計をチェックしたいカテゴリ分類を選んでください。</p>\n%s", 
      theme(
        'item_list', 
        array(
          'items' => array(
            '分類ごと' . theme('item_list', array('items' => $vocab_items)),
            'ユーザーごと' . theme('item_list', array('items' => $user_items)),
            'その他' . theme('item_list', array('items' => $misc_items)),
          )
        )
      )
    );
    $form = drupal_get_form('bancho_gaapi_statistics_settings');
    $s .= drupal_render($form);
  }

  $content_array = array(
    '#type' => 'markup',
    '#markup' => $s,
  );
  

  // jquery.table_colorscaleがあれば使う
  $colorscale_path = drupal_get_path('module', 'bancho_gaapi') . '/jquery.table_colorscale/jquery.table_colorscale.js';
  if (file_exists(realpath($colorscale_path))) {
    $content_array['#attached']['js'][$colorscale_path] = array('preprocess' => FALSE);
    $content_array['#attached']['js'][drupal_get_path('module', 'bancho_gaapi') . '/bancho_gaapi.statistics.js'] = array('preprocess' => FALSE);
  }

  return $content_array;
}


/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_statistics_settings($form, &$form_state) {

  $types = bancho_gaapi_get_types();
  $bancho_gaapi_statistics_settings_types = variable_get('bancho_gaapi_statistics_settings_types', FALSE);
  if (!$bancho_gaapi_statistics_settings_types) {
    $bancho_gaapi_statistics_settings_types = array();
    foreach ($types as $key => $value) {
      $bancho_gaapi_statistics_settings_types[$key] = $key;
    }
  }
  // Field APIフィールド一覧
  foreach (field_info_fields() as $key => $value) {
    $fields[$key] = $value['field_name'];
  }
  ksort($fields);

  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => '設定',
    '#tree' => TRUE,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['settings']['types'] = array(
    '#type' => 'checkboxes',
    '#title' => '標準で有効にするノードタイプ',
    '#options' => $types,
    '#default_value' => $bancho_gaapi_statistics_settings_types,
  );
  $form['settings']['submit'] = array(
    '#type' => 'submit',
    '#value' => '保存',
  );
  return $form;
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_statistics_settings_submit($form, &$form_state) {
  variable_set(
    'bancho_gaapi_statistics_settings_types',  
    $form_state['values']['settings']['types']
  );
}


/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_statistics_data($target = FALSE) {

  $bancho_fields = array(
    'corner' => variable_get('bancho_field_corner', ''),
    'author' => variable_get('bancho_field_reffer_author', FALSE),
    'editor' => variable_get('bancho_field_editor', FALSE),
    'planner' => variable_get('bancho_field_planner', FALSE),
  );
  foreach (array('corner') as $field) {
    if (empty($field)) {
      drupal_set_message("$fieldが設定されていません。" . l('こちら', 'admin/config/bancho/bancho_common') . 'から設定してください。');
    }
  }



  // フィールドのDB関連情報をまとめておく
  $field_db_info = bancho_common_get_field_data();



  // 標準のSELECT SQL
  // node_post_age_avgは、15日未満の記事が日数マイナス効果を出さないため

  $query = db_select('bancho_gaapi', 'ga');

  $query->leftJoin('node', 'n', 'ga.nid = n.nid');
  $query->leftJoin('taxonomy_index', 'tn', 'n.nid = tn.nid');
  $query->leftJoin('taxonomy_term_data', 'td', 'td.tid = tn.tid');

  // 表で使う値
  $query->addField('td', 'tid', 'itemId');
  $query->addField('td', 'name', 'name');


  $query->addExpression('COUNT(DISTINCT(n.nid))', 'num');
  $query->addExpression('AVG(ga.total_2week_pv)', 'pv_initial_avg');
  $query->addExpression('AVG(ga.total_pv)', 'pv_total_avg');
  $query->addExpression('AVG(ga.total_visits)', 'visits_total_avg');
  $query->addExpression('SUM(ga.total_pv)', 'pv_total');
  $query->addExpression('AVG(UNIX_TIMESTAMP() - n.created)', 'node_age_avg');
  $query->addExpression('AVG(ga.total_delta)', 'pv_total_delta');

  // 統計とかで使う値
  $query->addField('n', 'type', 'node_type');

  $conditions = db_and();
  $conditions->condition('n.status', 1, '=');
  
  $groupBy = array();
  $orderBy = array();



  // キーワード検索のときの検索対象フィールド名デフォルト
  $search_target = array('td.tid', 'td.name');


  // 対象vid指定の場合
  if (is_numeric($target)) {
    $query->isNotNull('td.name');
    $conditions->condition('td.vid', $target, '=');
    $groupBy[] = 'tn.tid';
  }

  // タイプごとの場合
  elseif ($target === 'types') {
    // nameとtidをn.typeに差し替え
    $fields =& $query->getFields();
    unset($fields['itemId']);
    unset($fields['name']);
    $query->addField('n', 'type', 'itemId');
    $query->addField('n', 'type', 'name');

    $groupBy[] ='n.type';
    $search_target = array('n.type', 'n.type');
  }


  // ユーザーごとの場合
  elseif ($target === 'users') {
    // nameとtidをu関連に差し替え
    
    if ($bancho_fields['author']) {
      $fields =& $query->getFields();
      unset($fields['itemId']);
      unset($fields['name']);

      $query->leftJoin($field_db_info[$bancho_fields['author']]['table'], 'fa', "(n.nid = fa.entity_id AND fa.entity_type = 'node')");
      $query->leftJoin('node', 'na', 'fa.' . $field_db_info[$bancho_fields['author']]['column'] . ' = na.nid');
      $query->leftJoin('users', 'u_users', 'n.uid = u_users.uid');
      $query->leftJoin($field_db_info[$bancho_fields['author']]['table'], 'fau', "(u_users.uid = fau.entity_id AND fau.entity_type = 'user')");
      $query->leftJoin('node', 'nau', 'fau.' . $field_db_info[$bancho_fields['author']]['column'] . ' = nau.nid');

      // グループ化と表示
      $query->addExpression('COALESCE(na.nid, nau.nid, u_users.uid)', 'itemId');
      $query->addExpression('COALESCE(na.title, nau.title, u_users.name)', 'name');

      // 絞り込みリンク用
      $query->addField('na', 'nid', 'itemId_na');
      $query->addField('nau', 'nid', 'itemId_nau');
      $query->addField('u_users', 'uid', 'itemId_u');

      $groupBy[] = 'itemId';
      $search_target = array('fa.entity_id', 'na.title','fau.entity_id', 'nau.title');
    }
    else {
      $fields =& $query->getFields();
      unset($fields['itemId']);
      unset($fields['name']);

      $query->leftJoin('users', 'u', 'n.uid = u.uid');

      $query->addField('u', 'uid', 'itemId');
      $query->addField('u', 'name', 'name');

      // 絞り込みリンク用
      $query->addField('u', 'uid', 'itemId_u');

      $field_realname = variable_get('bancho_field_realname', '');
      if ($field_realname) {
        $query->addField(
            'pv',
            $field_db_info[$field_realname]['column'],
            'realname'
          );
        $query->leftJoin(
            $field_db_info[$field_realname]['table'],
            'pv',
            "(u.uid=pv.entity_id AND pv.entity_type='user')"
          );
      }

      $groupBy[] = 'u.uid';
      $search_target = array('u.uid', 'u.name', 'pv.value');
    }
  }


  // 担当編集者ごとの場合
  elseif ($target === 'editors') {
    // nameとtidをu関連に差し替え
    
    if ($bancho_fields['editor']) {
      $fields =& $query->getFields();
      unset($fields['itemId']);
      unset($fields['name']);

      $query->rightJoin($field_db_info[$bancho_fields['editor']]['table'], 'fe', "(n.nid = fe.entity_id AND fe.entity_type = 'node')");
      $query->leftJoin('users', 'ue', 'fe.' . $field_db_info[$bancho_fields['editor']]['column'] . ' = ue.uid');

      $query->addField('ue', 'uid', 'itemId');
      $query->addField('ue', 'name', 'name');

      // 絞り込みリンク用
      $query->addField('ue', 'uid', 'itemId_u');

      $groupBy[] = 'itemId';
      $search_target = array('ue.uid', 'ue.name');
    }
    else {
    }
  }


  // 企画編集者ごとの場合
  elseif ($target === 'planners') {
    // nameとtidをu関連に差し替え
    
    if ($bancho_fields['planner']) {
      $fields =& $query->getFields();
      unset($fields['itemId']);
      unset($fields['name']);

      $query->rightJoin($field_db_info[$bancho_fields['planner']]['table'], 'fe', "(n.nid = fe.entity_id AND fe.entity_type = 'node')");
      $query->leftJoin('users', 'ue', 'fe.' . $field_db_info[$bancho_fields['planner']]['column'] . ' = ue.uid');

      $query->addField('ue', 'uid', 'itemId');
      $query->addField('ue', 'name', 'name');

      // 絞り込みリンク用
      $query->addField('ue', 'uid', 'itemId_u');

      $groupBy[] = 'itemId';
      $search_target = array('ue.uid', 'ue.name');
    }
    else {
    }
  }


  // 記事ごとの場合
  elseif ($target === 'nodes') {
    // ノードタイプもtidも指定されていなければ何もしない{
    if (!empty($_GET['options']['type']['bancho_gaapi_select_menu']) || (!isset($_GET['options']['type']) && !isset($_GET['tids']) && !isset($_GET['uids']))) {
//      $_GET['options']['type']['bancho_gaapi_select_menu'] = TRUE;
//      $sql_where .= ' AND 0 ';
    }


    // nameとtidをn関連に差し替え
    $fields =& $query->getFields();
    unset($fields['itemId']);
    unset($fields['name']);
    $query->addField('n', 'nid', 'itemId');
    $query->addField('n', 'title', 'name');

    $expressions =& $query->getExpressions();
    unset($expressions['num']);
    unset($expressions['pv_initial_avg']);
    unset($expressions['pv_total_avg']);
    unset($expressions['visits_total_avg']);
    unset($expressions['pv_total']);
    unset($expressions['node_age_avg']);
    unset($expressions['pv_total_delta']);
    $query->addExpression('1', 'num');
    $query->addExpression('ga.total_2week_pv', 'pv_initial_avg');
    $query->addExpression('ga.total_pv', 'pv_total_avg');
    $query->addExpression('ga.total_visits', 'visits_total_avg');
    $query->addExpression('ga.total_pv', 'pv_total');
    $query->addExpression('UNIX_TIMESTAMP() - n.created', 'node_age_avg');
    $query->addExpression('ga.total_delta', 'pv_total_delta');

//    $sql_groupby = '';
    $search_target = array('n.nid', 'n.title');
  }

  $title_appendix = array();

  // tid指定での絞り込み
  if (!empty($_GET['tids'])) {

    $or = db_or();
    foreach ((array) $_GET['tids'] as $tid) {
      if (is_numeric($tid)) {
        $or->condition('tn.tid', $tid, '=');
        // タイトル指定用
        $obj = taxonomy_term_load($tid);
        $title_appendix[] = check_plain($obj->name);
      }
    }
    $conditions->condition($or);
  }


  // uid指定での絞り込み
  if (!empty($_GET['uids'])) {
    $query->leftJoin('users', 'u2', 'n.uid=u2.uid');

    $or = db_or();
    foreach ((array) $_GET['uids'] as $uid) {
      if (is_numeric($uid) ) {
        $or->condition('u2.uid', $uid, '=');
        // タイトル指定用
        $obj = user_load($uid);
        $title_appendix[] = 'ユーザー' . check_plain($obj->name);
      }
    }
    $conditions->condition($or);
  }


  // 筆者ノード指定での絞り込み（uid）
  if (!empty($_GET['users_uids']) || $_GET['users_uids'] === '0') {
    $query->leftJoin('users', 'uq', 'uq.uid = n.uid');
    $or_e = db_or();
    foreach ((array) $_GET['users_uids'] as $uid) {
      if (is_numeric($uid) ) {
        $or_e->condition('uq.uid', $uid, '=');
        // タイトル指定用
        $obj = user_load($uid);
        $title_appendix[] = '筆者 ' . check_plain($obj->name);
      }
    }
    $conditions->condition($or_e);
  }


  // 筆者ノード指定での絞り込み（nid）
  if (!empty($_GET['users_nids'])) {
    if (!empty($bancho_fields['author'])) {
      $query->leftJoin($field_db_info[$bancho_fields['author']]['table'], 'faq', "(n.nid = faq.entity_id AND faq.entity_type = 'node')");
      $query->leftJoin('users', 'uq', 'n.uid = uq.uid');
      $query->leftJoin($field_db_info[$bancho_fields['author']]['table'], 'faqu', "(uq.uid = faqu.entity_id AND faqu.entity_type = 'user')");

      $or_users_nids = db_or();
      foreach ((array) $_GET['users_nids'] as $nid) {
        if (is_numeric($nid) ) {
          // 筆者ノード参照が記事ノードに直接指定されている場合
          $or_users_nids->condition('faq.' . $field_db_info[$bancho_fields['author']]['column'], $nid, '=');
          // 筆者ノード参照は記事ノードに直接指定されておらず、
          // 記事ノードのuidに筆者ノード参照が指定されている場合。
          // ただし、記事ノードに他の筆者ノード参照がある場合を除く
          // （statistics/usersとの整合性のため
          $cond_usernode_via_user = db_and();
          $cond_usernode_via_user->condition('faqu.' . $field_db_info[$bancho_fields['author']]['column'], $nid, '=');
          $cond_usernode_via_user->isNull('faq.' . $field_db_info[$bancho_fields['author']]['column']);
          $or_users_nids->condition($cond_usernode_via_user);
          // タイトル指定用
          $obj = node_load($nid);
          $title_appendix[] = '筆者 ' . check_plain($obj->title);
        }
      }

      $conditions->condition($or_users_nids);
    }
    else {
      drupal_set_message('筆者ノード指定で絞り込みがされましたが、筆者ノードのフィールドが指定されていません');
    }
  }


  // 担当編集者指定での絞り込み
  if (!empty($_GET['editors_uids'])) {
    $query->leftJoin($field_db_info[$bancho_fields['editor']]['table'], 'faqe', "(n.nid = faqe.entity_id AND faqe.entity_type = 'node')");
    $query->leftJoin('users', 'uaqe', 'uaqe.uid = faqe.' . $field_db_info[$bancho_fields['editor']]['column']);
    $or_e = db_or();
    foreach ((array) $_GET['editors_uids'] as $uid) {
      if (is_numeric($uid) ) {
        $or_e->condition('faqe.' . $field_db_info[$bancho_fields['editor']]['column'], $uid, '=');
        // タイトル指定用
        $obj = user_load($uid);
        $title_appendix[] = '担当編集者 ' . check_plain($obj->name);
      }
    }
    $conditions->condition($or_e);
  }


  // 企画編集者指定での絞り込み
  if (!empty($_GET['planners_uids'])) {
    $query->leftJoin($field_db_info[$bancho_fields['planner']]['table'], 'faqp', "(n.nid = faqp.entity_id AND faqp.entity_type = 'node')");
    $query->leftJoin('users', 'uaqp', 'uaqp.uid = faqp.' . $field_db_info[$bancho_fields['planner']]['column']);
    $or_p = db_or();
    foreach ((array) $_GET['planners_uids'] as $uid) {
      if (is_numeric($uid) ) {
        $or_p->condition('faqp.' . $field_db_info[$bancho_fields['planner']]['column'], $uid, '=');
        // タイトル指定用
        $obj = user_load($uid);
        $title_appendix[] = '企画編集者 ' . check_plain($obj->name);
      }
    }
    $conditions->condition($or_p);
  }


  // 最低記事年齢での絞り込み
  if (!empty($_GET['options']['lb_age']) && is_numeric($_GET['options']['lb_age'])) {
    $conditions->condition('(UNIX_TIMESTAMP() - n.created)/86400', $_GET['options']['lb_age'], '>');
    // タイトル指定用
    $title_appendix[] = '記事年齢' . check_plain($_GET['options']['lb_age']) . '日以上';
  }


  // 最低pv数での絞り込み
  if (!empty($_GET['options']['lb_pv']) && is_numeric($_GET['options']['lb_pv'])) {
    switch ($_GET['options']['lb_pv_type']) {
      case 'initial':
        $lb_pv_type = 'ga.total_2week_pv';
        break;
      default:
        $lb_pv_type = 'ga.total_pv';
        break;
    }
    $conditions->condition($lb_pv_type, $_GET['options']['lb_pv'], '>');
    // タイトル指定用
    $title_appendix[] = check_plain($_GET['options']['lb_pv']) . 'PV以上';
  }


  // 日付範囲
  if (!empty($_GET['options']['date']['from'])) {
    $num = strtotime($_GET['options']['date']['from']);
    if ( $num > 0) {
      $conditions->condition('n.created', $num, '>=');
      $title_appendix_temp = check_plain($_GET['options']['date']['from']) . ～;
    }
  }
  if (!empty($_GET['options']['date']['to'])) {
    $num = strtotime($_GET['options']['date']['to']);
    if ( $num > 0) {
      $conditions->condition('n.created', $num, '<=');
      $title_appendix_temp .= (empty($title_appendix_temp) ? '～' : '') . check_plain($_GET['options']['date']['to']);
    }
  }
  if (!empty($title_appendix_temp)) {
    $title_appendix[] = $title_appendix_temp . ' 公開の記事';
  }


  // キーワード検索
  if (!empty($_GET['options']['search'])) {
    $or = db_or();
    foreach ($search_target as $search ) {
      $or->condition(
          $search,
          sprintf('%%%s%%', $_GET['options']['search']),
          'LIKE'
        );
    }
    $conditions->condition($or);
    $title_appendix[] = check_plain($_GET['options']['search']);
  }


  // 漫画記事を除く
  // TODO：管理画面設定必要
  if (!empty($_GET['options']['exclude_manga']) && $_GET['options']['exclude_manga']) {
    $query->leftJoin('taxonomy_index', 'tn_manga', 'n.nid = tn_manga.nid');
    $conditions->condition('tn_manga', 2766, '<>');
    $title_appendix[] = '漫画記事を除く';
  }


  if (count($title_appendix)) {
    drupal_set_title(drupal_set_title() . "：\n" . implode(' | ', $title_appendix));
  }


  // ノードタイプ情報を取得しておく
  $node_types = bancho_gaapi_get_types();

  // type指定
  $types = array();
  if (isset($_GET['options']['type'])) {
    foreach ($_GET['options']['type'] as $k => $v) {
      if ($k != "bancho_gaapi_select_menu" && array_key_exists($v, $node_types)) {
        $types[] = $v;
      }
    }
  }
  else {
    $types_settings = variable_get('bancho_gaapi_statistics_settings_types', array());
    foreach ($types_settings as $k => $v) {
      if ($k == $v && array_key_exists($v, $node_types)) {
        $types[] = $v;
      }
    }
  }
  if (count($types)) {
    $conditions->condition('n.type', $types, 'IN');
  }

//dpm($query->__toString());

  // ソート順
  $sort_key = 'pv_total_delta';
  $sort_order = 'desc';
  foreach ($_GET as $key => $value) {
    if (strtolower($key) == 'key') {
      switch ($value) {
        case 'itemId':
        case 'name':
        case 'num':
        case 'pv_initial_avg':
        case 'pv_total_avg':
        case 'visits_total_avg':
        case 'pv_total':
        case 'node_age_avg':
        case 'pv_total_delta':
          $sort_key = $value;
          break;
      }
    }
    elseif (strtolower($key) == 'order') {
      switch ($value) {
        case 'asc':
          $sort_order = 'asc';
          break;
        case 'desc':
          $sort_order = 'desc';
          break;
      }
    }
  }




  // 標準偏差のキー
  switch ($sort_key) {
    case 'itemId':
    case 'name':
    case 'num':
    case 'pv_total':
      $pop_var_key = 'NULL';
      break;
    case 'pv_initial_avg':
      $pop_var_key = 'ga.total_2week_pv';
      break;
    case 'pv_total_avg':
      $pop_var_key = 'ga.total_pv';
      break;
    case 'visits_total_avg':
      $pop_var_key = 'ga.total_visits';
      break;
    case 'node_age_avg':
      $pop_var_key = 'UNIX_TIMESTAMP() - n.created';
      break;
    case 'pv_total_delta':
    default:
      $pop_var_key = 'ga.total_delta';
      break;
  }
  if ($target === 'nodes') {
    $pop_var_key = 'NULL';
  }

  $query->addExpression(
    ($pop_var_key != 'NULL' ? 'STDDEV_POP(' . $pop_var_key . ') ' : "CONCAT('-')"), 
    'STDDEV_POP'
  );


  // クエリ結果に含まれるノードタイプ一覧を作るため
  $selected_node_types = array();



  // 全体データ取得


  // あとで$queryを別の非extendedで使うためにここは別オブジェクトにして実行

  // http://www.drup-all.com/blog/table-sort-pagination-drupal-7
  // まずTarbleSort系の処理
  $orderByHeader = array();
  foreach (array($query->getFields(), $query->getExpressions()) as $fs) {
    foreach ($fs as $f) {
      $headerTemp = array(
        'data' => $f['alias'],
        'field' => $f['alias']
      );
      if ($sort_key == $f['alias']) {
        $headerTemp['sort'] = $sort_order;
      }
      $orderByHeader[] = $headerTemp;
    }
  }

  $query->condition($conditions);
  foreach ($groupBy as $groupByItem) {
    $query->groupBy($groupByItem);
  }


  $queryTableSort = $query->extend('TableSort')->orderByHeader($orderByHeader);


  $queryRows = $queryTableSort
    ->extend('PagerDefault')
    ->limit((!empty($_GET['nopaging']) || arg(5)=='nopaging') ? 65535 : BANCHO_GAAPI_STATS_RPP);

  $queryRows->distinct();

//watchdog('bancho_debug', '$query=%query', array('%query'=>$queryRows->__toString()));

  $db_res = $queryRows->execute();

  $pager = theme('pager');


  $stats = array(
    'num' => 0,
    'pv' => 0,
    'pv_initial' => 0,
    'age' => 0,
  );
  $position = 0;
  $items = array();
  $ga_profile_id = str_replace(
    'ga:', 
    '', 
    variable_get("bancho_gaapi_current_profile", '')
  );
  $today = date("Ymd");

  global $pager_page_array;

  foreach ($db_res as $row) {
    if (!empty($row->node_type) && !isset($selected_node_types[$row->node_type])) {
      $selected_node_types[$row->node_type] = $row->node_type;
    }

    if ($target == 'users' && !empty($row->realname)) {
      $realname = $row->realname;
    }
    else {
      $realname = FALSE;
    }

    $item = array();

    // 順位
    $item['position'] = array(
      'class' => array('td_num'),
      'data' => $pager_page_array[0] * BANCHO_GAAPI_STATS_RPP + ++$position,
    );

    // itemId
    $item['itemId'] = array(
      'class' => array((is_numeric($row->itemId) ? 'td_num' : 'td_name')),
      'data' => check_plain($row->itemId),
    );
    switch ($target) {
    case 'users':
    case 'editors':
    case 'planners':
      if (!empty($row->itemId_na)) {
        $item['itemId']['data'] = 'nid' . $item['itemId']['data'];
      }
      elseif (!empty($row->itemId_nau)) {
        $item['itemId']['data'] = 'nid' . $item['itemId']['data'];
      }
      elseif (!empty($row->itemId_u)) {
        $item['itemId']['data'] = 'uid' . $item['itemId']['data'];
      }
      break;
    }


    $item['name'] = array(
      'class' => array('td_name'),
      'data' => '',
    );
    if (!is_numeric($row->itemId)) {
      // カテゴリ名
      $item['name']['data'] = check_plain($node_types[$row->itemId]);
    }
    elseif ($target === 'nodes') {
      // 記事ごとの場合
      $item['name']['data'] = l(
        $row->name,
        'node/' . $row->itemId,
        array(
          'attributes' => array('title' => 'この記事を表示する')
        )
      );
      $item['name']['data'] .= 
        ' ['
         . theme('bancho_gaapi_galink', array('type' => 'page', 'nid' => $row->itemId, 'created' => NULL, 'path' => NULL, 'anchor' => 'GA'))
         . theme('bancho_gaapi_galink', array('type' => 'referrer', 'nid' => $row->itemId))
         . theme('bancho_gaapi_galink', array('type' => 'keywords', 'nid' => $row->itemId))
         . ']';
    }
    elseif ($target === 'users' || $target == 'editors' || $target == 'planners') {
      // 筆者ごと・担当者ごと・企画者ごとの場合
/*
      if ($row->itemId == 0) {
        $item['name']['data'] = '匿名';
      }
      else {
        $query_name = $target;
        if (!empty($row->itemId_na)) {
          $query_name .= '_nids';
          $detaiL_url_prefix = 'node/';
        }
        elseif (!empty($row->itemId_nau)) {
          $query_name .= '_nids';
          $detaiL_url_prefix = 'node/';
        }
        elseif (!empty($row->itemId_u)) {
          $query_name .= '_uids';
          $detaiL_url_prefix = 'user/';
        }
        $item['name']['data'] = l(
          ($realname ? check_plain($realname) : ($row->name ? check_plain($row->name) : 'uid:' . $row->itemId)) . theme('image', array('path' => 'misc/arrow-asc.png')),
          'admin/reports/bancho_gaapi/statistics/nodes',
          array(
            'attributes' => array('title' => 'このユーザーに限定して記事単位のアクセス数を確認'),
            'query' => array_merge(array($query_name => $row->itemId), (!empty($_GET['options']) ? array('options' => $_GET['options']) : array())),
            'html' => TRUE
          )
        );
        $item['name']['data'] .=' [' . l('?', $detaiL_url_prefix . $row->itemId, array('attributes' => array('title' => 'このユーザーの情報を確認'))) . ']';
      }
*/
      $query_name = $target;
      if ($row->itemId == 0) {
        $query_name .= '_uids';
        $detaiL_url_prefix = FALSE;
      }
      elseif (!empty($row->itemId_na)) {
        $query_name .= '_nids';
        $detaiL_url_prefix = 'node/';
      }
      elseif (!empty($row->itemId_nau)) {
        $query_name .= '_nids';
        $detaiL_url_prefix = 'node/';
      }
      elseif (!empty($row->itemId_u)) {
        $query_name .= '_uids';
        $detaiL_url_prefix = 'user/';
      }
      $item['name']['data'] = l(
        ($row->itemId == 0 ? '匿名' :($realname ? check_plain($realname) : ($row->name ? check_plain($row->name) : 'uid:' . $row->itemId))) . theme('image', array('path' => 'misc/arrow-asc.png')),
        'admin/reports/bancho_gaapi/statistics/nodes',
        array(
          'attributes' => array('title' => 'このユーザーに限定して記事単位のアクセス数を確認'),
          'query' => array_merge(array($query_name => $row->itemId), (!empty($_GET['options']) ? array('options' => $_GET['options']) : array())),
          'html' => TRUE
        )
      );
      if ($row->itemId != 0) {
        $item['name']['data'] .=' [' . l('?', $detaiL_url_prefix . $row->itemId, array('attributes' => array('title' => 'このユーザーの情報を確認'))) . ']';
      }
    }
    else {
      // それ以外の場合（termごととか）
      $item['name']['data'] = l(
        check_plain($row->name) . theme('image', array('path' => 'misc/arrow-asc.png')),
        'admin/reports/bancho_gaapi/statistics/nodes',
        array(
          'attributes' => array('title' => 'このコーナーに限定して記事単位のアクセス数を確認'),
          'query' => array_merge(
            array('tids' => $row->itemId),
            (!empty($_GET['options']) ? array('options' => $_GET['options']) : array())
          ),
          'html' => TRUE
        )
      );
    }

    if ($target !== 'nodes') {
      // 記事数
      $item['num'] = array(
        'class' => array('td_num'),
        'data' => check_plain(number_format($row->num)),
      );
    }

    // 14日平均pv
    $item['pv_initial_avg'] = array(
      'class' => array('td_pv', 'blur'),
      'data' => check_plain(number_format($row->pv_initial_avg)),
    );

    // 累積平均pv
    $item['pv_total_avg'] = array(
      'class' => array('td_pv', 'blur'),
      'data' => check_plain(number_format($row->pv_total_avg)),
    );

    // 累積平均visits
    $item['visits_total_avg'] = array(
      'class' => array('td_pv', 'blur'),
      'data' => check_plain(number_format($row->visits_total_avg)),
    );

    if ($target !== 'nodes') {
      // 累積pv
      $item['pv_total'] = array(
        'class' => array('td_pv', 'blur'),
        'data' => check_plain(number_format($row->pv_total)),
      );
    }

    // 平均記事年齢
    $item['node_age_avg'] = array(
      'class' => array('td_num'),
      'data' => check_plain(number_format($row->node_age_avg / 86400, 1)),
    );

    // 15日目以降
    $item['pv_total_delta'] = array(
      'class' => array('td_num', 'blur'),
      'data' => check_plain(number_format($row->pv_total_delta)),
    );

    if ($target !== 'nodes') {
      // 標準偏差
      $item['STDDEV_POP'] = array(
        'class' => array('td_num', 'blur'),
        'data' => ($row->STDDEV_POP == '-' ? '-' : check_plain(number_format(round($row->STDDEV_POP, 2)))),
      );
    }
    $items[] = $item;
  }
  // 表の上に表示する統計情報
  if ($items) {


    $queryStats = db_select('bancho_gaapi', 'ga');
    foreach ($query->getTables() as $k=>$v) {
      if ($v['join type']) {
        $queryStats->addJoin($v['join type'], $v['table'], $v['alias'], $v['condition'], $v['arguments']);
      }
    }
    $queryStats->addExpression('count(DISTINCT(n.nid))', 'num');
    $queryStats->addExpression('SUM(DISTINCT(ga.total_pv))', 'pv_total');
    $queryStats->addExpression('SUM(DISTINCT(ga.total_2week_pv))', 'pv_initial');
    $queryStats->addExpression('SUM(UNIX_TIMESTAMP() - n.created)', 'node_age');
    $queryStats->condition($conditions);
// statsuにgroupbyは不要
//    foreach ($groupBy as $groupByItem) {
//      $queryStats->groupBy($groupByItem);
//    }

//watchdog('bancho_debug', '$queryStats=%query', array('%query'=>$queryStats->__toString()));

    $statRow = $queryStats->execute()->fetchObject();

    $stats['num'] = $statRow->num;
    $stats['pv'] = $statRow->pv_total;
    $stats['pv_initial'] = $statRow->pv_initial;
    $stats['age'] += $statRow->node_age;

    // caption作成
    $table_caption = '※15日目以降pv増加数<span style="font-size: 0.8em">(7日あたり' . ($target == 'nodes' ? '' : '平均') . ')</span>';
    drupal_add_css('
.bancho_gaapi_statistics caption{
  caption-side: top;
  text-align: right;
  color: #660000;
}', array('type' => 'inline')
    );

    // テーブルHTML作成
    // 標準のでやるとsortのキーがthの中身になってURLがアレなので
    // やむを得ず自作
    $table_header_data = bancho_gaapi_get_tableheader($_GET['q'], $target);
    // 表頭ヘッダー
    // theme_tableのheaderに['data']だけ放り込むとstickyにできる
//    array_unshift($items, $table_header_data);
    // 表末ヘッダー
    $items[] = $table_header_data;
    $s = '';
    global $pager_total, $pager_total_items;
    if ($pager_total[0] > 1 && $pager_total_items[0] < 250) {
      $s .= '<div style="text-align: center">' . l('ページネーションなしで表示', arg(0) . '/'.arg(1).'/'.arg(2).'/'.arg(3).'/'.arg(4).'/nopaging', array('query' => drupal_get_query_parameters($_GET, array('q', 'page')))) . '</div>';
    }
    elseif (!empty($_GET['page']) && $_GET['page'] == 'full') {
      $s .= '<div style="text-align: center">' . l('ページネーションありで表示', $_GET['q'], array('query' => drupal_get_query_parameters($_GET, array('q', 'page')))) . '</div>';
    }



    $s .= $pager;

    $s .= theme(
      'table', 
      array(
        'header' => $table_header_data['data'],
        'rows' => $items,
        'attributes' => array(
          'class' => array('bancho_gaapi_statistics', 'sticky-header'),
        ), 
        'caption' => $table_caption,
        'sticky' => TRUE,
      )
    );

    $s .= $pager;
    $s =  sprintf(
      '
<p>全体平均（14日）： <span class="blur">%s</span> pv/記事、 全体平均（累計）： <span class="blur">%s</span> pv/記事<br />
対象総記事数： %s 記事（平均年齢 %s 日）、 対象累計pv数合計： <span class="blur">%s</span> pv<br />
このデータに含まれるノードタイプ： %s</p>
<div><strong>※ヒント：</strong>
<ul>
%s
%s
%s
%s
</div>
%s', 
      number_format($stats['pv_initial'] / $stats['num'], 1), 
      number_format($stats['pv'] / $stats['num'], 1), 
      number_format($stats['num']), 
      number_format($stats['age'] / $stats['num'] / 86400, 1), 
      number_format($stats['pv']), 
      check_plain(implode('、 ', $selected_node_types)), 
      ($target == 'nodes' ? '' : '<li>カテゴリ名をクリックすると、そのカテゴリ内でノード単位に絞り込んで調べられます。</li>'), 
      (isset($_GET['options']['type']) ? '' : '<li>オプションで<strong>対象ノードタイプを絞り込む</strong>と、また違った情報が見えてきます。</li>'), 
      (isset($_GET['key']) ? '' : '<li>表の見出しセルをクリックすると並べ替えの基準を変更できます。</li>'), 
      (isset($_GET['order']) ? '' : '<li>表の見出しセルの▲をクリックすると並べ替えの方向を変更できます。</li>'), 
      $s
    );
  }
  else {
    // 記事ごとの場合、ノードタイプの指定がなければ指定させる
    if (!empty($_GET['options']['type']['bancho_gaapi_select_menu'])) {
      $s = '<p style="font-size: 1.5em; font-weight: bold">チェックしたいノードタイプを指定して[送信]してください。</p>';
    }
    else {
      $s = '<p>指定された条件ではデータが見つかりません。</p>';
    }
  }

  // パンくずリスト設定
  $bc = drupal_get_breadcrumb();
  $bc[] = l(
    'アクセス解析データの統計', 
    'admin/reports/bancho_gaapi/statistics'
  );
  drupal_set_breadcrumb($bc);

  // オプションのフォーム
  $form = drupal_get_form('bancho_gaapi_statistics_form');


  return drupal_render($form) . $s;
}



/**
 * theme_tableで使う$header用の配列を返す
 *@return theme_tableで使う$header用の配列
 */
function bancho_gaapi_get_tableheader($url, $target){
  if (empty($url)){
    return;
  }
  $key='';
  $order='';
  $query = array();
  foreach($_GET as $k=>$v){
    if ($k == 'key'){
      $key = $v;
    }
    elseif($k == 'order'){
      $order = $v;
    }
    elseif($v !== '' && $k !== 'q'){
      $query[$k] = $v;
    }
  }

  $tableheader = array();

  $tableheader['data'][] = array(
    'data'=>'順位',
    'style'=>'text-align: center'
  );

  $tableheader['data'][] = bancho_gaapi_get_tableheader_list_helper(
    'itemId', ($target == 'nodes' ? 'nid' : ($target == 'users' ? 'uid' : 'tid')), $key, $order, $url, $query, array('style'=>'text-align: center')
  );
  $tableheader['data'][] = bancho_gaapi_get_tableheader_list_helper(
    'name', ($target == 'nodes' ? '記事タイトル' : ($target == 'users' ? '筆者名' : 'カテゴリ名')), $key, $order, $url, $query
  );
  if ($target !== 'nodes'){
    $tableheader['data'][] = bancho_gaapi_get_tableheader_list_helper(
      'num', '記事数', $key, $order, $url, $query, array('class'=>array('bancho_gaapi_statistics_data'), 'style'=>'text-align: center')
    );
  }
  $tableheader['data'][] = bancho_gaapi_get_tableheader_list_helper(
    'pv_initial_avg', ($target == 'nodes' ? '' : '平均').'pv<br /><span style="font-size: 0.8em">(公開後<br />14日)</span>', $key , $order, $url, $query, array('class'=>array('bancho_gaapi_statistics_data'), 'style'=>'text-align: center')
  );
  $tableheader['data'][] = bancho_gaapi_get_tableheader_list_helper(
    'pv_total_avg', ($target == 'nodes' ? '' : '平均').'pv<br /><span style="font-size: 0.8em">(累計)</span>', $key, $order, $url, $query, array('class'=>array('bancho_gaapi_statistics_data'), 'style'=>'text-align: center')
  );
  $tableheader['data'][] = bancho_gaapi_get_tableheader_list_helper(
    'visits_total_avg', ($target == 'nodes' ? '' : '平均').'visit<br /><span style="font-size: 0.8em">(累計)</span>', $key, $order, $url, $query, array('class'=>array('bancho_gaapi_statistics_data'), 'style'=>'text-align: center')
  );
  if ($target !== 'nodes'){
    $tableheader['data'][] = bancho_gaapi_get_tableheader_list_helper(
      'pv_total', '累計pv', $key, $order, $url, $query, array('class'=>array('bancho_gaapi_statistics_data'), 'style'=>'text-align: center')
    );
  }
  $tableheader['data'][] = bancho_gaapi_get_tableheader_list_helper(
    'node_age_avg', ($target == 'nodes' ? '' : '平均<br />').'記事<br />年齢<span style="font-size: 0.8em">（日）</span>', $key, $order, $url, $query, array('class'=>array('bancho_gaapi_statistics_data'), 'style'=>'text-align: center')
  );
  $tableheader['data'][] = bancho_gaapi_get_tableheader_list_helper(
    'pv_total_delta', 'pv増加数<sup style="color: #660000">※</sup>', empty($key) ? 'pv_total_delta' : $key, $order, $url, $query, array('class'=>array('bancho_gaapi_statistics_data'), 'style'=>'text-align: center')
  );
  if ($target !== 'nodes'){
    $tableheader['data'][] = array(
      'header'=>1,
      'data'=>'キー<br />値の<br />標準<br />偏差',
      'style'=>'text-align: center',
      'class'=>array('bancho_gaapi_statistics_data'), 
    );
  }
  return $tableheader;
}


function bancho_gaapi_get_tableheader_list_helper($target_key, $label, $key, $order, $url, $query, $attributes=array()){

//drupal_set_message('$qs='. $qs."<br />");

  if ($target_key==$key){
    $tr = array(
      'header'=>1,
      'data'=>$label . l(
        preg_replace('/ title="[^"]+"/', '', theme('tablesort_indicator', array('style'=>strtolower($order) == 'asc' ? 'desc' : 'asc'))),
        $url,
        array(
          'attributes' =>array(
            'title'=> sprintf(
              'この項目を%sで並べ替える（現在の並べ替え順：%s）',
              strtolower($order) == 'asc' ? '降順' : '昇順',
              strtolower($order) == 'asc' ? '昇順' : '降順'
            )
          ),
          'query' => array_merge(
            $query,
            array(
              'key' => $target_key,
              'order' => strtolower($order) == 'asc' ? 'desc' : 'asc'
            )
          ),
          'html' => TRUE
        )
      )
    );
  }
  else {
    $tr = array(
      'header'=>1,
      'data'=>l(
        $label, 
        $url,
        array(
          'attributes' => array('title'=> 'この項目で並べ替え'),
          'query' => array_merge(
            $query,
            array(
              'key' => $target_key,
              'order' => 'desc'
            )
          ),
          'fragment' => NULL,
          'absolute' => TRUE,
          'html' => TRUE
         )
      )
    );
  }
  return array_merge($tr, $attributes);
}



/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_statistics_form($form, &$form_state) {
  if (isset($_GET['options']['date'])) {
    $daterange = array(
      'from' => $_GET['options']['date']['from'],
      'to' => $_GET['options']['date']['to'],
    );
  }
  else {
    $daterange = array(
      'from' => '',
      'to' => '',
    );
  }
  $form = array();
  $form['#method'] = 'GET';
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => 'オプションを指定',
    '#collapsible' => TRUE,
    '#collapsed' => (isset($_GET['options']) ? FALSE : TRUE),
    '#tree' => TRUE,
  );
  $form['options']['date'] = array(
    '#type' => 'fieldset',
    '#title' => '対象記事の公開日時を指定（YYYY/MM/DD形式） ※アクセス集計対象期間ではない',
    '#tree' => TRUE,
  );
  $form['options']['date']['from'] = array(
    '#type' => 'textfield',
    '#default_value' => $daterange['from'],
    '#attributes' => array('style' => 'display: inline; width: 10em'),
    '#theme_wrappers' => array(),
  );
  $form['options']['date']['separetor'] = array(
    '#type' => 'markup',
    '#markup' => ' <span>～</span> ',
    '#theme_wrappers' => array(),
  );
  $form['options']['date']['to'] = array(
    '#type' => 'textfield',
    '#default_value' => $daterange['to'],
    '#attributes' => array('style' => 'display: inline; width: 10em'),
    '#theme_wrappers' => array(),
  );
  $form['options']['lb_age'] = array(
    '#type' => 'textfield',
    '#field_prefix' => '最低記事年齢：',
    '#field_suffix' => '日以上',
    '#default_value' => empty($_GET['options']['lb_age']) ? '' : $_GET['options']['lb_age'],
    '#attributes' => array('style' => 'width: 10em'),
  );
  $form['options']['lb_pv'] = array(
    '#type' => 'textfield',
    '#field_prefix' => '最低PV数：',
    '#field_suffix' => 'pv以上',
    '#default_value' => empty($_GET['options']['lb_pv']) ? '' : $_GET['options']['lb_pv'],
    '#attributes' => array('style' => 'width: 10em;'),
    '#prefix' => '<div class="clearfix"><div style="float: left;">',
    '#suffix' => '</div>',
  );
  $form['options']['lb_pv_type'] = array(
    '#type' => 'radios',
    '#options' => array(
      'initial' => '公開後14日間',
      'total' => '累計',
    ),
    '#default_value' => (isset($_GET['options']['lb_pv_type']) ? $_GET['options']['lb_pv_type'] : 'total'),
    '#attributes' => array('style' => 'display: inline; margin-left: 2em'),
    '#prefix' => '<div style="float: left;">',
    '#suffix' => '</div></div>',
  );
  if (isset($_GET['tids'])) {
    $form['tids'] = array(
      '#type' => 'hidden',
      '#value' => empty($_GET['tids']) ? '' : $_GET['tids'],
    );
  }
  if (isset($_GET['uid'])) {
    $form['uid'] = array(
      '#type' => 'hidden',
      '#value' => empty($_GET['uid']) ? '' : $_GET['uid'],
    );
  }
  if (isset($_GET['uids'])) {
    $form['uids'] = array(
      '#type' => 'hidden',
      '#value' => empty($_GET['uids']) ? '' : $_GET['uids'],
    );
  }
  if (isset($_GET['users_uids'])) {
    $form['users_uids'] = array(
      '#type' => 'hidden',
      '#value' => empty($_GET['users_uids']) ? '' : $_GET['users_uids'],
    );
  }
  if (isset($_GET['users_nids'])) {
    $form['users_nids'] = array(
      '#type' => 'hidden',
      '#value' => empty($_GET['users_nids']) ? '' : $_GET['users_nids'],
    );
  }
  if (isset($_GET['editors_uids'])) {
    $form['editors_uids'] = array(
      '#type' => 'hidden',
      '#value' => empty($_GET['editors_uids']) ? '' : $_GET['editors_uids'],
    );
  }
  if (isset($_GET['planners_uids'])) {
    $form['planners_uids'] = array(
      '#type' => 'hidden',
      '#value' => empty($_GET['planners_uids']) ? '' : $_GET['planners_uids'],
    );
  }
  if (isset($_GET['key'])) {
    $form['key'] = array(
      '#type' => 'hidden',
      '#value' => empty($_GET['key']) ? '' : $_GET['key'],
    );
  }
  if (isset($_GET['order'])) {
    $form['order'] = array(
      '#type' => 'hidden',
      '#value' => empty($_GET['order']) ? '' : $_GET['order'],
    );
  }

  // ノードタイプ
  $node_types = bancho_gaapi_get_types();
  $node_options = array();
  $node_types_css_selectors = array();
  foreach ($node_types as $k => $v) {
    $node_types_css_selectors[] = '#edit-options-type-' . str_replace('_', '-', $k) . '-wrapper';
  }
  if (!isset($_GET['options']['type'])) {
    $node_options = variable_get('bancho_gaapi_statistics_settings_types', array());
  }
  else {
    foreach ($_GET['options']['type'] as $v) {
      $node_options[] = $v;
    }
  }
  $form['options']['type'] = array(
    '#type' => 'checkboxes',
    '#multiple' => TRUE,
    '#title' => '対象ノードタイプ',
    '#default_value' => $node_options,
    '#options' => $node_types,
  );
  $form['options']['search'] = array(
    '#type' => 'textfield',
    '#title' => '検索キーワード',
    '#default_value' => empty($_GET['options']['search']) ? '' : $_GET['options']['search'],
  );
  $form['options']['exclude_manga'] = array(
    '#type' => 'checkbox',
    '#title' => '漫画記事を除く',
    '#default_value' => empty($_GET['options']['exclude_manga']) ? '' : $_GET['options']['exclude_manga'],
  );
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => '送信',
  );


  $css_text = <<<CSS_TEXT
<style type="text/css">
#edit-options-date-from-wrapper,
#edit-options-date-to-wrapper,
#edit-options-lb-pv-wrapper,
.form-radios,
#edit-options-lb-pv-type-initial-wrapper,
#edit-options-lb-pv-type-total-wrapper,
%s{
  display: inline
}
.bancho_gaapi_statistics .td_num, .td_pv{
  text-align: right;
}
</style>
CSS_TEXT;
  drupal_add_html_head(sprintf($css_text, implode(',', $node_types_css_selectors)), $key = NULL /* TODO Set this variable. */);

  return $form;
}


/**
 * 検索フォームから送信されたパラメータで中身の指定されていないものが
 * ある場合はそのパラメータを削除した形でリダイレクトして
 * URLをきれいにする。
 */
function bancho_gaapi_cleanup_params() {
  // パラメータ一覧
  $legit_param_keys = array(
    'key',
    'order',
    'options',
    'tids',
    'uids',
    'page',
    'form_build_id',
    'form_id',
    'form_token',
    'op',
    'editors_uids',
    'planners_uids',
    'users_uids',
    'users_nids'
  );
  $new_query = array();
  $need_redirect = FALSE;

  foreach ($_GET as $k => $v) {
    // 標準でないquery_stringは削除
    if (!in_array($k, $legit_param_keys)) {
      if ($k != 'q') {
        $need_redirect = TRUE;
      }
    }
    else {
      $result = bancho_gaapi_cleanup_params_check($k, $v);
      $need_redirect = $need_redirect || $result['need_redirect'];
      $new_query[$k] = $result['new_qs'];
    }
  }
  if ($need_redirect) {
    drupal_goto($_GET['q'], array('query' => $new_query));
    exit;
  }
  else {
    return TRUE;
  }
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_cleanup_params_check($k, $v) {
  $new_query = array();
  $need_redirect = FALSE;
  if (is_array($v)) {
    foreach ($v as $array_k => $array_v) {
      $result = bancho_gaapi_cleanup_params_check($array_k, $array_v);
      $new_query[$array_k] = $result['new_qs'];
      if ($result['need_redirect']) {
        $need_redirect = TRUE;
      }
    }
  }
  else {
    if (empty($v) && $v !== '0') {
      $need_redirect = TRUE;
    }
    else {
      $new_query = $v;
    }
  }
  return array(
    'need_redirect' => $need_redirect,
    'new_qs' => $new_query,
  );
}

/*
 * query stringを作るsupport function
 */
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_append_qs($k, $v, $encode = TRUE) {

  if (is_array($v)) {
    $s = '';
    foreach ($v as $array_k => $array_v) {
      $s .= bancho_gaapi_append_qs($k . '[' . $array_k . ']', $array_v, $encode);
    }
    return $s;
  }
  else {
    return '&' . rawurlencode($k) . '=' . ($encode ? rawurlencode($v) : $v);
  }
}



/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_get_types() {
  static $node_types;
  if (!isset($node_types)) {
    $node_types = array();
    // ノードタイプ一覧（キーがシステム名、値が表示名）
    // node_get_typesだと順序がちょっと……
    $result = db_query('SELECT type, name FROM {node_type}');
    foreach ($result as $row) {
      $node_types[$row->type] = $row->name;
    }
  }
  return $node_types;
}



