<?php
/**
 * @file
 * Web担の週間などランキング記事作成用のスクリプト
 */


// GA API用コード読み込み
module_load_include('inc', 'bancho_gaapi', 'gaapi_core');


function bancho_gaapi_ranking_weekly_main() {

  // drupal_get_formを重ねて使うと変になるので、
  // argで判別して、引き渡すデータは$_SESSIONを経由させることにした
  if (arg(3) == 'node_form') {
    if (isset($_SESSION['bancho_gaapi_ranking_weekly_node_form_data'])) {
      // セッションデータはbancho_gaapi_ranking_weekly_form_alter_main()で削除

      global $user;

      $node_type = variable_get('bancho_gaapi_ranking_weekly_node_type', 'story');
      $node = (object)FALSE;
      $node->nid = NULL;
      $node->vid = NULL;
      $node->type = $node_type;
      $node->uid = $user->uid;
      $node->name = (isset($user->name) ? $user->name : '');
      $node->language = LANGUAGE_NONE;
      // このフォームのみform_alterで操作できるようにしておく
      // form_alterで操作しないと面倒でしかたがない
      // see bancho_gaapi_ranking_weekly_form_alter()
      // see bancho_gaapi_ranking_weekly_form_alter_main()
      $node->bancho_gaapi_ranking_weekly = TRUE;

    //  $form_state_node_form = array();
    //  $form_state_node_form['build_info']['args'] = array($node);
      // node/add用のnode_form()が入ってるファイルが必要（標準nodeモジュール）
    //  form_load_include($form_state, 'inc', 'node', 'node.pages');
    //  $form = drupal_build_form($node_type .'_node_form', $form_state_node_form);

      // node/add用のnode_form()が入ってるファイルが必要（標準nodeモジュール）
      module_load_include('inc', 'node', 'node.pages');
      $form = drupal_get_form($node_type .'_node_form', $node);
      // scheduler settingはform_alterで出てこなかったのでやむを得ずここで
      // ここで値を設定する場合はformのbuild後なので
      // #default_valueじゃなくて#valueを使わなきゃだめ
      $form['options']['scheduler_settings']['publish_on']['#value'] = $form['options']['date']['#default_value'];

      // $form['#action']はform_alterで毎回指定しないと、
      // validateとかpreviewとかした後のactionが元に戻っちゃう

//dpm($form);
      return $form;
    }
    else {
      drupal_goto('node/add/bancho_gaapi_ranking_weekly/');
    }
    
  }

  return drupal_get_form('bancho_gaapi_ranking_weekly_rankingform');

}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_ranking_weekly_rankingform($form, &$form_state) {
  // 設定項目を読み込んで、未設定項目があればフォームは設定だけにする
  $setting_keys = array(
//  key_name => donot show main form if value is empty
    'to_dow'=>TRUE,
    'publish_dow'=>TRUE,
    'node_type'=>TRUE,
    'corner_terms'=>TRUE,
    'content_terms'=>TRUE,
    'type_terms'=>TRUE,
    'tag_terms'=>TRUE,
    'flag_terms'=>TRUE,
    'use_title_head'=>TRUE,
    'exclude_tids'=>FALSE,
    'exclude_nids'=>FALSE,
    'field_image'=>FALSE,
  );
  $settings = array('ready' => TRUE);
  foreach ($setting_keys as $item=>$check) {
    $settings[$item] = variable_get('bancho_gaapi_ranking_weekly_' . $item, FALSE);
    if ($settings[$item] === FALSE && $check) {
      $settings['ready'] = FALSE;
    }
  }

  // 設定が済んでる場合にのみフォームを表示
  if (!$settings['ready']) {
    $form['warning'] = array(
      '#markup' => '<p>まず各種設定をしてください。</p>',
    );
  }

  else {
    // to_dowは対象期間の末尾曜日
    // GAは
    if (date('w') == $settings['to_dow']) {
      $to = time();
    }
    else {
      $to = time() - (86400 * (date("w") + (date("w") < $settings['to_dow'] ? 7 : 0) - $settings['to_dow']));
    }
    $from = $to - 60 * 60 * 24 * 6;



    // 「GAから取得」と「nidのリストを指定」の切り替え
    drupal_add_js('
(function($){
$(document).ready(function(){
  $("#edit-src-ga, #edit-src-nids")
  .change(function(){
    var css_legend_common = {
      padding:"0.25em 0.5em",
      marginLeft:"0.5em",
      marginTop:"-2em",
      marginBottom:"-2.5em",
      width:"12em",
      height:"",
      position:"relative",
    };
    var css_active_legend = $.extend({
      backgroundColor:"#ff0000",
      fontWeight:"bold",
    }, css_legend_common);
    var css_inactive_legend = $.extend({
      backgroundColor:"transparent",
      fontWeight:"normal",
    }, css_legend_common);
    var css_active = {color: "#000"};
    var css_inactive = {color: "#ccc"};
    if ($("#edit-src-ga").is(":checked")) {
      $("#wrapper_ga *").removeAttr("disabled").css(css_active);
      $("#wrapper_ga legend").css(css_active_legend);
      $("#wrapper_nids *").attr("disabled",true).css(css_inactive);
      $("#wrapper_nids legend").css(css_inactive_legend);
    }
    else if($("#edit-src-nids").is(":checked")) {
      $("#wrapper_nids *").removeAttr("disabled").css(css_active);
      $("#wrapper_nids legend").css(css_active_legend);
      $("#wrapper_ga *").attr("disabled",true).css(css_inactive);
      $("#wrapper_ga legend").css(css_inactive_legend);
    }
  })
  .trigger("change");
});}(jQuery))', array('type'=>'inline', 'scope' => 'footer', 'weight'=>0));


    $default_values = variable_get(
      'bancho_gaapi_ranking_weekly_default',
      array()
    );

    $form['src'] = array(
      '#type' => 'radios',
      '#title' => '作成元データ',
      '#default_value' => $default_values['src'] ? $default_values['src'] : 'ga',
      '#options' => array(
        'ga' => 'GAからデータを取得してランキングHTMLを出力',
        'nids' => 'nidのリストからランキングHTMLを出力',
      ),
      '#prefix' => '<div class="clearfix">',
      '#suffix' => '</div>',
    );
    $form['ga'] = array(
      '#type' => 'fieldset',
      '#title' => 'GAから取得',
      '#tree' => TRUE,
      '#prefix' => '<div id="wrapper_ga" style="width: 60%; margin-right: 5%; float: left; padding-top: 1em; overflow:hidden">',
      '#suffix' => '</div>',
      '#attributes' => array(
        'style' => 'overflow:hidden; padding-top: 1.5em;'
      )
    );
    $form['ga']['from'] = array(
      '#type' => 'textfield',
      '#title' => '開始日',
      '#default_value' => $default_values['ga']['from'] ? $default_values['ga']['from'] : date("Y-m-d", $from),
      '#description' => 'yyyy-mm-dd形式',
      '#size' => 15,
    );
    $form['ga']['to'] = array(
      '#type' => 'textfield',
      '#title' => '終了日',
      '#default_value' => $default_values['ga']['to'] ? $default_values['ga']['to'] : date("Y-m-d", $to),
      '#description' => 'yyyy-mm-dd形式',
      '#size' => 15,
    );
    $form['ga']['num'] = array(
      '#type' => 'select',
      '#title' => '対象件数',
      '#options' => drupal_map_assoc(array(1, 3, 5, 10, 20, 50, 100, 1000, 5000)),
      '#default_value' => $default_values['ga']['num'] ? $default_values['ga']['num'] : 20,
    );
    $form['ga']['profile'] = array(
      '#type' => 'select',
      '#title' => '対象プロファイル',
      '#options' => array(),
      '#default_value' => $default_values['ga']['profile'] ? $default_values['ga']['profile'] : 'current',
      '#prefix' => '<div style="overflow:hidden">',
      '#suffix' => '</div>',
    );

    $form['nids'] = array(
      '#type' => 'fieldset',
      '#title' => 'nidのリストを指定',
      '#tree' => TRUE,
      '#attributes' => array(
        'style' => 'overflow:hidden; padding-top: 1.5em;'
      ),
      '#prefix' => '<div id="wrapper_nids" style="width: 30%; float: left; padding-top: 1em; ">',
      '#suffix' => '</div><span class="clearfix"></span>',
    );
    $form['nids']['nid_list'] = array(
      '#type' => 'textarea',
      '#title' => 'nidリスト',
      '#description' => 'nidリスト（1行に1nid）が入力された場合は、その場でのGAのデータ取得は行わず、指定されたnidリストをランキングのHTMLにフォーマットします。',
      '#tree' => TRUE,
  //    '#parent' => array('wrapper_nids'),
    );

    $form_info = bancho_gaapi_form_strings();
    foreach ($form_info as $profile => $info) {
      if (bancho_gaapi_profile_info($profile, 'id')) {
        $form['ga']['profile']['#options'][$profile] = bancho_gaapi_profile_info($profile, 'name');
      }
    }

    $image_styles = image_styles();
    $styles = array('0' => 'なし');
    foreach ($image_styles as $style) {
      $styles[$style['name']] = $style['label'];
    }

    $form['teaser_icon'] = array(
      '#type' => 'select',
      '#title' => '各記事にティーザーアイコンを表示するか',
      '#default_value' => $default_values['teaser_icon'] ? $default_values['teaser_icon'] : '0',
      '#options' => $styles,
    );
    $form['teaser_text'] = array(
      '#type' => 'radios',
      '#title' => '各記事にティーザーテキストを表示するか',
      '#default_value' => $default_values['teaser_text'] ? $default_values['teaser_text'] : '0',
      '#options' => array(
        '0' => 'なし',
        'short' => '1行ティーザー',
        'long' => 'ティーザー',
      ),
    );

    $form['created'] = array(
      '#type' => 'radios',
      '#title' => '各記事の公開日時の表記',
      '#default_value' => $default_values['created'] ? $default_values['created'] : '0',
      '#options' => array(
        '0' => 'なし',
        'short' => str_replace('/0', '/', strftime('%Y/%m/%d')),
        'long' => str_replace('/0', '/', strftime('%Y年%m月%d日 %H:%M')),
      ),
    );

    $form['sbm'] = array(
      '#type' => 'radios',
      '#title' => '各記事のソーシャル反応数',
      '#default_value' => $default_values['sbm'] ? $default_values['sbm'] : '0',
//      '#disabled' => TRUE,
      '#options' => array('0'=>'なし', '3'=>'上位3つのみ', '5'=>'上位5つのみ', '10'=>'上位10個のみ', '99'=>'全部あり'),
      '#description' => '表示すればするほど、ランキング記事の表示が重くなるので注意。標準では付けても「上位3つのみ」ぐらい。あとのは、内部的な使用のためだと思ってください。'
    );

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'ランキングを作成',
      '#weight' => 98,
      '#attributes' => array('style'=>'font-size: 2em; padding: .5em 1.5em;')
    );
    $form['save_node_default'] = array(
      '#type' => 'submit',
      '#submit' => array('bancho_gaapi_ranking_weekly_save_default'),
      '#value' => 'この作成設定を標準として保存',
      '#weight' => 99,
      '#prefix' => '<br style="margin-bottom: 2em">',
      '#suffix' => '<div>※［設定］内容ではなく、ランキング記事作成フォームの現在の内容をデフォルトにする</div>'
    );
  }


  // 設定

  // ノードタイプ一覧
  $node_types = array();
  foreach (node_type_get_types() as $type) {
    $node_types[$type->type] = $type->name;
  }
  
  // term一覧
  // $vids（int or array)を指定するとそのvidのterm一覧、指定しないと全vidの一覧
  function bancho_gaapi_ranking_weekly_rankingform_termlist($vids=NULL) {
    // term一覧（taxonomy_form_term()から一部ぱくり）
    $vids = (array)$vids;
    if (empty($vids)) {
      $vocabularies = array();
      foreach (taxonomy_get_vocabularies() as $vocab) {
        $vids[] = $vocab->vid;
        $vocabularies[$vocab->vid] = $vocab->name;
      }
    }
    $terms = array(0=>'--設定しない--');
    foreach ($vids as $vid) {
      if (count($vids) > 1) {
        $terms[] = '<' . $vocabularies[$vid] . '>';
      }
      $tree = taxonomy_get_tree($vid);
      foreach ($tree as $item) {
        $terms[$item->tid] = str_repeat('-', $item->depth) . $item->name;
      }
    }
    return $terms;
  }


  // Field APIフィールド一覧
  $fields = array(0 => '設定しない');
  foreach (field_info_fields() as $key => $value) {
    $fields[$key] = $value['field_name'];
  }
  ksort($fields);



  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => '設定',
    '#tree' => TRUE,
    '#weight' => 98,
    '#collapsible' => TRUE,
    '#collapsed' => $settings['ready'],
  );

  $form['settings']['to_dow'] = array(
    '#type' => 'select',
    '#title' => 'ランキング対象期間',
    '#options' => array(
      '0' => '月～日',
      '1' => '火～月',
      '2' => '水～火',
      '3' => '木～水',
      '4' => '金～木',
      '5' => '土～金',
      '6' => '日～土',
    ),
    '#default_value' => $settings['to_dow'],
  );

  $form['settings']['publish_dow'] = array(
    '#type' => 'select',
    '#title' => 'ランキング記事の掲載予定曜日',
    '#options' => array(
      '0' => '日',
      '1' => '月',
      '2' => '火',
      '3' => '水',
      '4' => '木',
      '5' => '金',
      '6' => '土'
    ),
    '#default_value' => $settings['publish_dow'] ? $settings['publish_dow'] : 1,
  );
    
  $form['settings']['node_type'] = array(
    '#type' => 'select',
    '#title' => '作成するランキング記事のノードタイプ',
    '#options' => $node_types,
    '#default_value' => $settings['node_type'] ? $settings['node_type'] : 'story',
  );

  $form['settings']['use_title_head'] = array(
    '#type' => 'select',
    '#title' => 'title要素用のタイトルフィールド',
    '#options' => $fields,
    '#default_value' => $settings['use_title_head'] ? $settings['use_title_head'] : 0,
  );

  $options = bancho_gaapi_ranking_weekly_rankingform_termlist(variable_get('bancho_vid_corner', NULL));
  $form['settings']['corner_terms'] = array(
    '#type' => 'select',
    '#title' => 'コーナー名',
    '#options' => $options,
    '#default_value' => $settings['corner_terms'] ? $settings['corner_terms'] : 0,
  );
  $options = bancho_gaapi_ranking_weekly_rankingform_termlist(variable_get('bancho_vid_content', NULL));
  $form['settings']['content_terms'] = array(
    '#type' => 'select',
    '#title' => '内容（テーマ）カテゴリ',
    '#options' => $options,
    '#multiple' => TRUE,
    '#default_value' => $settings['content_terms'] ? $settings['content_terms'] : 0,
  );
  $options = bancho_gaapi_ranking_weekly_rankingform_termlist(variable_get('bancho_vid_type', NULL));
  unset($options[0]);
  $form['settings']['type_terms'] = array(
    '#type' => 'checkboxes',
    '#title' => '記事種別カテゴリ',
    '#options' => $options,
    '#multiple' => TRUE,
    '#default_value' => $settings['type_terms'] ? $settings['type_terms'] : 0,
  );
  $options = bancho_gaapi_ranking_weekly_rankingform_termlist(variable_get('bancho_vid_tag', NULL));
  $form['settings']['tag_terms'] = array(
    '#type' => 'select',
    '#title' => 'タグ',
    '#options' => $options,
    '#multiple' => TRUE,
    '#default_value' => $settings['tag_terms'] ? $settings['tag_terms'] : 0,
  );
  $options = bancho_gaapi_ranking_weekly_rankingform_termlist(variable_get('bancho_vid_flag', NULL));
  unset($options[0]);
  $form['settings']['flag_terms'] = array(
    '#type' => 'checkboxes',
    '#title' => 'フラグ',
    '#options' => $options,
    '#multiple' => TRUE,
    '#default_value' => $settings['flag_terms'] ? $settings['flag_terms'] : 0,
  );

//webtan
//  1843, // 人気記事ランキング親
//  2875, // アクセス数上位
//  2874, // 被リンク数
//  2573, //まとめ記事
  $options = bancho_gaapi_ranking_weekly_rankingform_termlist();
  $form['settings']['exclude_tids'] = array(
    '#type' => 'select',
    '#title' => '殿堂入り（除外）のterm',
    '#multiple' => TRUE,
    '#options' => $options,
    '#size' => '10',
    '#description' => 'Ctrlキーを押しながらクリックで複数選択',
    '#default_value' => $settings['exclude_tids']&&is_array($settings['exclude_tids'])&&count($settings['exclude_tids'])>0 ? $settings['exclude_tids'] : array('0'=>'0'),
  );

//webtan
//  3786, // 企画書
//  4371, // Dropbox
//  10999, // g/o2o
//  12806, // Facebookページ
  $form['settings']['exclude_nids'] = array(
    '#type' => 'textarea',
    '#title' => '殿堂入り（除外）のノード',
    '#rows' => '10',
    '#description' => 'ノードIDを1行に1つ記述。<br />カンマが入っていても無視。<br />「//」から行末まではコメントとして無視。',
    '#default_value' => $settings['exclude_nids'] ? $settings['exclude_nids'] : '',
  );

  $form['settings']['save'] = array(
    '#type' => 'submit',
    '#value' => '設定を保存',
    '#validate' => array('bancho_gaapi_ranking_weekly_settings_validate'),
    '#submit' => array('bancho_gaapi_ranking_weekly_settings_submit'),
    '#weight' => 99,
  );

  // 設定でノードのフィールドを使うためのダミーノード
  $node = (object)FALSE;
  $node->nid = NULL;
  $node->vid = NULL;
  $node->type = variable_get('bancho_gaapi_ranking_weekly_node_type', 'story');
  $entity_lang = field_language('node', $node, 'body', entity_language('node', $node));
  $form['#parents'] = array();

  // $entity_langはsubmitでも使うので保存しておく
  $form['settings']['entity_lang'] = array(
    '#type' => 'hidden',
    '#value' => $entity_lang,
  );

  // デフォルトのティーザーアイコン
  $field = variable_get('bancho_field_teaser_icon','');
  $form += (array) _field_invoke_default('form', 'node', $node, $form, $form_state, array('language'=>NULL, 'field_name' => $field));
  $elem = array_merge(array(), $form[$field]);
  $form['settings'][$field] = $elem;
  unset($form[$field]);
  $form['settings'][$field][$entity_lang][0]['#title'] = 'デフォルトのティーザーアイコン';
  $form['settings'][$field][$entity_lang][0]['#default_value'] = $settings['field_image'];


//dpm($form);

  drupal_add_css('.form-radios .form-item {display: inline; margin-right: 1em !important}', array('type' => 'inline'));
  return $form;
}

/**
 * save form defaults
 */
function bancho_gaapi_ranking_weekly_save_default($form, &$form_state) {
  $bancho_gaapi_ranking_weekly_default = array_merge(array(), $form_state['values']);
  unset($bancho_gaapi_ranking_weekly_default['ga']['from']);
  unset($bancho_gaapi_ranking_weekly_default['ga']['to']);
  variable_set(
    'bancho_gaapi_ranking_weekly_default',
    $bancho_gaapi_ranking_weekly_default
  );
  drupal_set_message('作成フォームのデフォルト選択を保存しました');
}

/**
 * validate function for save settings button
 */
function bancho_gaapi_ranking_weekly_settings_validate($form, &$form_state) {}
/**
 * submit function for save settings button
 */
function  bancho_gaapi_ranking_weekly_settings_submit($form, &$form_state){

  variable_set(
    'bancho_gaapi_ranking_weekly_to_dow',
    $form_state['values']['settings']['to_dow']
  );
  variable_set(
    'bancho_gaapi_ranking_weekly_publish_dow',
    $form_state['values']['settings']['publish_dow']
  );
  variable_set(
    'bancho_gaapi_ranking_weekly_node_type',
    $form_state['values']['settings']['node_type']
  );
  variable_set(
    'bancho_gaapi_ranking_weekly_use_title_head',
    $form_state['values']['settings']['use_title_head']
  );

  variable_set(
    'bancho_gaapi_ranking_weekly_corner_terms',
    $form_state['values']['settings']['corner_terms']
  );
  variable_set(
    'bancho_gaapi_ranking_weekly_content_terms',
    $form_state['values']['settings']['content_terms']
  );
  variable_set(
    'bancho_gaapi_ranking_weekly_type_terms',
    $form_state['values']['settings']['type_terms']
  );
  variable_set(
    'bancho_gaapi_ranking_weekly_tag_terms',
    $form_state['values']['settings']['tag_terms']
  );
  variable_set(
    'bancho_gaapi_ranking_weekly_flag_terms',
    $form_state['values']['settings']['flag_terms']
  );


  variable_set(
    'bancho_gaapi_ranking_weekly_exclude_tids',
    $form_state['values']['settings']['exclude_tids']
  );
  variable_set(
    'bancho_gaapi_ranking_weekly_exclude_nids',
    $form_state['values']['settings']['exclude_nids']
  );

//dpm($form_state['values']['settings']);

  variable_set(
    'bancho_gaapi_ranking_weekly_field_image',
    $form_state['values']['settings']['field_image'][$form_state['values']['settings']['entity_lang']][0]
  );


}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_ranking_weekly_rankingform_validate($form, &$form_state) {
  if (!trim($form_state['values']['nids']['nid_list'])) {
    if (!strtotime($form_state['values']['ga']['from'])) {
      form_set_error('from', '日付設定が不正です');
    }
    if (!strtotime($form_state['values']['ga']['to'])) {
      form_set_error('to', '日付設定が不正です');
    }
  }
}

/**
 * ランキング対象ノードの$nid一覧を作って$_SESSIONに保存して
 * node_form用のURLにリダイレクト
 */
function bancho_gaapi_ranking_weekly_rankingform_submit($form, &$form_state) {
//watchdog('bancho_debug', 'bancho_gaapi_ranking_weekly_rankingform_submit');
  bancho_gaapi_ranking_weekly_settings_submit($form, $form_state);
//watchdog('bancho_debug', 'bancho_gaapi_ranking_weekly_settings_submit done');

  $nodes = array();
  $num = 0;

  switch ($form_state['values']['src']) {

  case 'ga':  // GAからデータを取得
    $nodes = bancho_gaapi_ranking_weekly_gaapi($form, $form_state);
    $num = $form_state['values']['ga']['num'];
    break;

  case 'nids':  // nid一覧から
    $nids = preg_split(
      '/ *[\r\n]+ */',
      trim($form_state['values']['nids']['nid_list']),
      NULL,
      PREG_SPLIT_NO_EMPTY
    );
    foreach ($nids as $i => $nid) {
      $nid = trim($nid);
      if (intval($nid) == $nid) {
        $nodes[$nid] = count($nids) - $i;
      }
    }
    arsort($nodes); // キーがnid、値がmetricなのでarsortでOK
    $num = count($nodes);
    break;
  }

  if ($num) {
    $_SESSION['bancho_gaapi_ranking_weekly_node_form_data'] = array(
      'nodes'=>$nodes,
      'num'=>$num,
      'ranking_form_values'=>$form_state
    );
    $form_state['redirect'] = 'node/add/bancho_gaapi_ranking_weekly/node_form';
  }
  else {
    drupal_set_message('指定された条件ではランキング対象ノードが見つかりません');
  }
}


/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_ranking_weekly_gaapi($form, &$form_state) {


  $profile = $form_state['values']['ga']['profile'];

  // API実行のパラメータを設定
  // （idsとmax-resultsはbancho_gaapi_do_apicall()が自動設定）
  $params = array(
    'metrics' => 'ga:pageviews',
    'dimensions' => 'ga:pagePath',
    'sort' => 'ga:pagePath',
    'start-date' => date("Y-m-d", strtotime($form_state['values']['ga']['from'])),
    'end-date' => date("Y-m-d", strtotime($form_state['values']['ga']['to'])),
  );


  // まずはstaticキャッシュをクリア
  bancho_gaapi_ranking_weekly_gaapi_data_reset();

  $requests_done = bancho_gaapi_do_apicall(
    $profile,
    $params,
    'bancho_gaapi_ranking_weekly_entry',  //$callback
    'bancho_gaapi_ranking_weekly_gaapi_data_reset',  //$resetcallback
    'bancho_gaapi_ranking_weekly' //$task
  );

//watchdog('bancho_debug', "\$requests_done=<pre>" . check_plain(print_r($requests_done,TRUE)). '</pre>');

  // staticキャッシュに貯めたデータを取得
  $nodes = bancho_gaapi_ranking_weekly_entry(FALSE);

//watchdog('bancho_debug', "\$nodes=<pre>" . check_plain(print_r($nodes,TRUE)). '</pre>');

  arsort($nodes);
//dpm($nodes);
  return $nodes;

}



/*
 * データをリセット（初期化時、またはデータ取得途中でエラー再試行の場合
 */
function bancho_gaapi_ranking_weekly_gaapi_data_reset($profile='', $params='', $callback='', $task=''){
  bancho_gaapi_ranking_weekly_entry(FALSE, FALSE, FALSE, FALSE, TRUE);
}


/**
 * GAAPIで取得した各レコードに対して呼び出されるコールバック関数
 * 
 */
function bancho_gaapi_ranking_weekly_entry($entry, $profile, $task, $params, $reset = FALSE) {

//watchdog('bancho_debug', "bancho_gaapi_ranking_weekly_entry" . check_plain($entry['dimension']['ga:pagePath']));

  // ノードに対して複数のURLでアクセスされていたらGAのga:pagePathが別になって
  // 複数の行でレポートされるので、それを1つにまとめる必要がある
  // キーがnidで値がそれまでに記録された数の$optionをキーとした配列
  static $nodes;

  // $reset指定時には$nodesクリア
  if ($reset || !isset($nodes)) {
//bancho_debug_to_file(ddebug_backtrace(TRUE), 'trace', '/tmp/debug/trace.txt', FALSE);
    $nodes = array();
  }

  if (!$entry) {
    // $entryがなかったらstaticキャッシュしたデータを全部返す
    return $nodes;
  }

  // 処理対象URLをDrupalパスとして扱えるよう前処理＆非対象チェック
  $url = bancho_gaapi_gapath2drpath($entry['dimension']['ga:pagePath']);
//watchdog('bancho_gaapi', '1' . $url);
  if (!$url) {
    return TRUE;
  }

  // ノードのページなら進む、ノードでなければ戻る
  $url = drupal_get_normal_path($url);
//watchdog('bancho_gaapi', '2' . $url);
  $nid = bancho_gaapi_is_this_node_url($url);
//watchdog('bancho_gaapi', '3' . $nid);

  if ($nid !== FALSE) {

    // 同一ノードに複数URLでのアクセスがあると
    // dimensions=ga:pagePath なので複数行になるので
    // static変数で保持しておいて加算していく
    $metric = 'ga:pageviews';
    if (isset($nodes[$nid])) {
      $nodes[$nid] = (int) $nodes[$nid] + (int) $entry['metric'][$metric];
    }
    else {
      $nodes[$nid] = $entry['metric'][$metric];
    }
//dpm($nodes);

  }

  return TRUE;
}





/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_ranking_weekly_build_node_form_data(&$form, $ranking_form_values, $nodes, $num) {

  // 特定termと特定nidは除外/殿堂入り
  // tid：
  $ng_tids = variable_get('bancho_gaapi_ranking_weekly_exclude_tids',array());
  // nid（カンマ削除/trim/コメント削除が必要）:
  $ng_nids = array();
  $ng_nids_lines = preg_split(
    '/[\n\r]+/',
    variable_get('bancho_gaapi_ranking_weekly_exclude_nids',''),
    -1,
    PREG_SPLIT_NO_EMPTY
  );
  foreach ($ng_nids_lines as $line) {
    $line = preg_replace('~//.*$~', '', $line);
    $line = preg_replace('/[ ,]/', '', $line);
    if ($line && preg_match('/^[0-9]+$/', $line)) {
      $ng_nids[] = $line;
    }
  }
  

  $teaser_icon_mode = $ranking_form_values['values']['teaser_icon'];
  $startdate = strtotime($ranking_form_values['values']['ga']['from']);
  $enddate = strtotime($ranking_form_values['values']['ga']['to']);

  // 別ファイルのテンプレート読み込み
  module_load_include('inc', 'bancho_gaapi_ranking_weekly', 'bancho_gaapi_ranking_weekly_tmplates');
  $tmpl = bancho_gaapi_ranking_weekly_tmplates();

  // ランキングのnidを処理してテンプレート変数を作る
  $rep = array(
    'RANKING_BODY1' => '',
    'RANKING_BODY2' => '',
    'RANKING_PERIOD' => date("Y年n月j日", $startdate) . '～' . date("Y年n月j日", $enddate),
    'HEADER_ICON_CLEARFIX' => $teaser_icon_mode ? ' class="clearfix"' : '',
  );
  if (date("Y", $startdate) != date("Y", $enddate)) {
    $rep['RANKING_PERIOD_SHORT'] = date("Y/n/j", $startdate) . '～' . date("Y/n/j", $enddate);
  }
  else {
    $rep['RANKING_PERIOD_SHORT'] = date("Y/n/j", $startdate) . '～' . date("n/j", $enddate);
  }
  $i = 0;

  // ランキング対象ノードから情報を集める
  foreach ($nodes as $nid => $pv) {

    if ( !is_int($nid) ) {
      continue;
    }
    if (++$i > $num) {
      break;
    }

    // 各ノード
    $node_obj = node_load($nid);
    if (!$node_obj) {
      $i--;
      continue;
    }

    // 非公開の記事は除外
    if (!$node_obj->status) {
      $i--;
      continue;
    }

    // 特定nidは除外/殿堂入り
    if (in_array($nid, $ng_nids)) {
      $i--;
      continue;
    }

   // 特定termは除外/殿堂入り
    foreach (_get_entity_terms('node', $node_obj->type, $node_obj) as $tid) {
      if (in_array($tid, $ng_tids)) {
        $i--;
        continue;
      }
    }

    // ティーザーアイコン
    $teaser_icon = '';
    $teaser_icon_field = variable_get('bancho_field_teaser_icon', '');
    if ($teaser_icon_mode && $teaser_icon_field) {
// これするとlazyload_img処理済みのHTMLになって二重処理になる
//      $image = field_view_field('node', $node_obj, $teaser_icon_field, $teaser_icon_mode);
//      $teaser_icon = render($image);
      $image = field_get_items('node', $node_obj, $teaser_icon_field);
      if ($image) {
        $image = reset($image);
        $teaser_icon = l(
          theme('image_style', array(
            'style_name' => $teaser_icon_mode,
            'path' => $image['uri'],
            'attributes' => array(
              'class' => $tmpl['teaser_icon_img_classes']
            )
          )), 'node/' . $node_obj->nid,
          array(
            'html'=>TRUE,
            'attributes' =>array(
              'class'=>$tmpl['teaser_icon_link_classes']
            ), 
          )
        );
      }
    }

    // 公開日時
    $created = '';
    switch ($ranking_form_values['values']['created']) {
      case 'short':
        $created = strftime('%Y/%m/%d', $node_obj->created);
        break;
      case 'long':
        $created = strftime('%Y年%m月%d日 %H:%M', $node_obj->created);
        break;
    }
    if ($created) {
      $created = $tmpl['created']['prefix'] . str_replace('/0', '/', $created) . $tmpl['created']['suffix'];
    }


    // ティーザーテキスト
    $teaser_text = '';
    switch ($ranking_form_values['values']['teaser_text']) {
      case 'short':
        $briefdesc = field_get_items('node', $node_obj, variable_get('bancho_field_brief_desc', ''));
        if ($briefdesc) {
          $briefdesc = reset($briefdesc);
          $teaser_text = check_markup($briefdesc['value'], $briefdesc['format']);
        }
        break;
      case 'long':
        $body = field_get_items('node', $node_obj, 'body');
        if ($body) {
          $body = reset($body);
          $teaser_text = check_markup($body['summary'], $body['format']);
        }
        break;
    }
    if ($teaser_text) {
      $teaser_text = $tmpl['teaser_text']['prefix'] . $teaser_text . $tmpl['teaser_text']['suffix'];
    }

    $sbm = '';
    if ($ranking_form_values['values']['sbm'] && $i<=$ranking_form_values['values']['sbm']) {
      if (function_exists('bancho_sbm_get_counts')) {
        $sbm = theme(
          'bancho_sbm_sbmlines',
          array(
            'sbm' => bancho_sbm_get_counts($node_obj),
            'node' => $node_obj,
            'page' => 1,
            'pos' => 0
          )
        );

      }
    }
    if ($sbm) {
      $sbm = $tmpl['sbm']['prefix'] . $sbm . $tmpl['sbm']['suffix'];
    }


    $titleStr = $tmpl['title']['prefix'];
    if (function_exists('bancho_corner_name')) {
      $titleStr .= $tmpl['title_corner_name']['prefix'] . theme('bancho_corner_name', array($nid, FALSE, FALSE, TRUE)) . $tmpl['title_corner_name']['suffix'];
    }
    $titleStr .= l($node_obj->title, 'node/' . $nid);
    $titleStr .= $tmpl['title']['suffix'];

    if ($i >= 1 && $i <= 10) {
      $rep['RANKING_BODY1'] .= sprintf(
        $tmpl['ranking_body_1'], 
        $i, 
        $teaser_icon,
        $titleStr, 
        $created, 
        $sbm, 
        $teaser_text, 
        "\n\n"
      );
    }
    else {
      $rep['RANKING_BODY2'] .= sprintf(
        $tmpl['ranking_body_2'],
        $i, 
        $teaser_icon,
        $titleStr, 
        $created, 
        $sbm, 
        $teaser_text, 
        "\n\n"
      );
    }
  }

  // ランキング記事のHTMLテンプレートにデータを埋める
  $body_html = $tmpl['body_html'];


  $teaser_html = $tmpl['teaser_html'];

  foreach ($rep as $key => $value) {
    $body_html = str_replace('<%' . $key . '%>', $value, $body_html);
    $teaser_html = str_replace('<%' . $key . '%>', $value, $teaser_html);
  }

  if (module_exists('bancho_lazyload_img')) {
    $body_html = bancho_lazyload_img_revert_filter($body_html);
    $teaser_html = bancho_lazyload_img_revert_filter($teaser_html);
  }

  // ここまでbody用のHTML作り

  // ここからnode-formをいじる


  $publish_dow = variable_get('bancho_gaapi_ranking_weekly_publish_dow', 0);
  $dt = strtotime(date("Y-m-d 08:00:00", time() + (86400 * ($publish_dow + ($publish_dow < date("w") ? 7 : 0)  - date("w")))));

//  $form['#do_not_revert_components'] = TRUE;

  // langのためにダミーノード作成（無駄）
  global $user;
  $node_type = variable_get('bancho_gaapi_ranking_weekly_node_type', 'story');
  $node = (object)FALSE;
  $node->nid = NULL;
  $node->vid = NULL;
  $node->type = $node_type;
  $node->uid = $user->uid;
  $node->name = (isset($user->name) ? $user->name : '');
  $node->language = LANGUAGE_NONE;
  $node->created = $dt;

  $entity_lang = field_language('node', $node, 'body', entity_language('node', $node));

  $form['body'][$entity_lang][0]['#default_value'] = $body_html;
  $form['body'][$entity_lang][0]['#format'] = variable_get('bancho_nodetype_filter_html', '');
//  $form['body'][$entity_lang][0]['value']['#default_value'] = $body_html;
//  $form['body'][$entity_lang][0]['value']['#value'] = $body_html;

//  $form['body'][$entity_lang][0]['summary']['#value'] = $teaser_html;
  $form['body'][$entity_lang][0]['summary']['#default_value'] = $teaser_html;
//  $form['body'][$entity_lang][0]['#format'] = 'full_html';

  // フィールド系は#default_valueじゃなくて#value
  // あと、格納場所がtypeでばらばら
  // 結果としてform_alterならシンプルな#default_valueで済んだ
  function bancho_gaapi_ranking_weekly_set_field_val(&$form, $field, $entity_lang, $values){
    if ($field) {
      if (isset($form[$field][$entity_lang][0])) {
        if (!empty($form[$field][$entity_lang][0]['#type'])) {
          switch ($form[$field][$entity_lang][0]['#type']) {
          case 'managed_file':
            $form[$field][$entity_lang][0]['#value'] = $values;
            break;

          case 'text_format':
//            $form[$field][$entity_lang][0]['value']['#default_value'] = $values;
//            $form[$field][$entity_lang][0]['value']['#value'] = $values;
            $form[$field][$entity_lang][0]['#default_value'] = $values;
            break;
          }
        }
        else {
          if (!empty($form[$field][$entity_lang][0]['value'])) {
            $form[$field][$entity_lang][0]['value']['#default_value'] = $values;
//            $form[$field][$entity_lang][0]['value']['#value'] = $values;
//            $form[$field][$entity_lang][0]['#default_value'] = $values;
          }
        }
      }
      else {
        switch ($form[$field][$entity_lang]['#type']) {
        case 'select':
//          $form[$field][$entity_lang]['#value'] = (array)$values;
          $form[$field][$entity_lang]['#default_value'] = (array)$values;
          break;

        case 'checkboxes':
          $form[$field][$entity_lang]['#default_value'] = (array)$values;
// form_alterなら面倒なことはしなくていい
//          foreach ($form[$field][$entity_lang]['#options'] as $option=>$s) {
//            if (in_array($option, $values)) {
//              $form[$field][$entity_lang][$option]['#checked'] = TRUE;
//            }
//          }
          break;
        }
      }
    }
  }


  // タイトルは、head_titleのフィールドが指定されたらそっちに入れてノードのtitleは空にしておく
  $use_title_head = variable_get('bancho_gaapi_ranking_weekly_use_title_head', FALSE);
  if (empty($use_title_head)) {
    $form['title']['#value'] = sprintf('%s／人気記事ランキング%s', '', $rep['RANKING_PERIOD_SHORT']);
  }
  else {
    bancho_gaapi_ranking_weekly_set_field_val(
      $form,
      $use_title_head,
      $entity_lang,
      sprintf('%s／人気記事ランキング%s', '%t', $rep['RANKING_PERIOD_SHORT'])
    );
  }

//    'uid' => 2,
//    'name' => 'webtan',
//    'field_planner' => array(0 => array('uid' => 2,),),
//    'field_editor' => array(0 => array('uid' => $user->uid,),),



  
  bancho_gaapi_ranking_weekly_set_field_val(
    $form,
    variable_get('bancho_field_teaser_icon', ''),
    $entity_lang,
    variable_get('bancho_gaapi_ranking_weekly_field_image', array())
  );

  bancho_gaapi_ranking_weekly_set_field_val(
    $form,
    variable_get('bancho_field_corner', ''),
    $entity_lang,
    variable_get('bancho_gaapi_ranking_weekly_corner_terms', array())
  );

  bancho_gaapi_ranking_weekly_set_field_val(
    $form,
    variable_get('bancho_field_content', ''),
    $entity_lang,
    variable_get('bancho_gaapi_ranking_weekly_content_terms', array())
  );

  bancho_gaapi_ranking_weekly_set_field_val(
    $form,
    variable_get('bancho_field_type', ''),
    $entity_lang,
    variable_get('bancho_gaapi_ranking_weekly_type_terms', array())
  );

  bancho_gaapi_ranking_weekly_set_field_val(
    $form,
    variable_get('bancho_field_tag', ''),
    $entity_lang,
    variable_get('bancho_gaapi_ranking_weekly_tag_terms', array())
  );

  bancho_gaapi_ranking_weekly_set_field_val(
    $form,
    variable_get('bancho_field_flag', ''),
    $entity_lang,
    variable_get('bancho_gaapi_ranking_weekly_flag_terms', array())
  );

  bancho_gaapi_ranking_weekly_set_field_val(
    $form,
    variable_get('bancho_field_brief_desc', ''),
    $entity_lang,
    '他のユーザーはどんな記事を読んでいるのか？先週1週間で最も人気の高かった記事をチェック！'
  );


  $form['scheduler_settings']['publish_on']['#default_value'] = date("Y-m-d H:i:s", $dt);
  $form['author']['date']['#default_value'] = format_date($node->created, 'custom', 'Y-m-d H:i:s O');



  // フォーム上の表示を追加
  $form['bancho_gaapi_ranking_weekly_notice'] = array(
    '#type' => 'markup',
    '#weight' => -100,
    '#markup' => '<p>タイトルと1行ティーザーは内容を反映して<span style="font-weight: bold; color: #ff0000">手動で編集</span>すること。<br /><strong>タイトルの例：</strong>検索結果は1ページ目しか見られていない？ プロの意見と統計データ／人気記事ランキング2009-7-4～7-10<br /><strong>1行ティーザーの例：</strong>急上昇の○×が△◇<br />&nbsp;&nbsp;またはトップの記事の1行ティーザーからもってくるとか</p>',
  );

//dpm($form);

}



/**
 * bancho_gaapi_ranking_weekly_form_node_form_alter()から呼ばれる
 */
function bancho_gaapi_ranking_weekly_form_alter_main(&$form, &$form_state, $form_id) {

  // node-formを自分で呼び出すときは、これを入れないとAjaxとかで不具合らしい
  form_load_include($form_state, 'inc', 'node', 'node.pages');

//dpm('bancho_gaapi_ranking_weekly_form_alter_main');
//dpm($form);

  // これは指定しないといろいろ面倒
  $node_type = variable_get('bancho_gaapi_ranking_weekly_node_type', 'story');
  $form['#action'] = url("node/add/$node_type");

  if (isset($_SESSION['bancho_gaapi_ranking_weekly_node_form_data'])) {

    // 保存してたセッションデータ
    $form_data = $_SESSION['bancho_gaapi_ranking_weekly_node_form_data'];
    // $formをいじる

    bancho_gaapi_ranking_weekly_build_node_form_data(
      $form,
      $form_data['ranking_form_values'],
      $form_data['nodes'],
      $form_data['num']
    );

    // セッションデータを破棄
    unset($_SESSION['bancho_gaapi_ranking_weekly_node_form_data']);
  }

}


// https://api.drupal.org/comment/50393#comment-50393
/**
* Fetchs all Taxonomy Term IDs from Entity Object. All fields of field type "taxonomy_term_reference" will be included.
*
* @param String $entity_type
* @param $string $bundle
* @param Object $entity
*
* @return array
*   Array with tids of entity
*/
function _get_entity_terms($entity_type, $bundle, $entity) {
  $tids = array();
  foreach (field_info_field_map() as $field_name => $field_info) {
    if ($field_info['type'] <> "taxonomy_term_reference") {
      continue;
    }
    if (array_key_exists($entity_type, $field_info['bundles'])) {
      if (in_array($bundle, $field_info['bundles'][$entity_type])) {
        if (isset($entity->{$field_name})) {
          $values = field_get_items($entity_type, $entity, $field_name);
          foreach ((array)$values as $tid) {
            $tids[] = $tid['tid'];
          }
        }
      }
    }
  }
  return array_unique($tids);
}
