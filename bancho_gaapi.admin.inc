<?php
/**
 * @file
 * 設定画面
 */


// GA API用コード読み込み
module_load_include('inc', 'bancho_gaapi', 'gaapi_core');

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_stub_settings() {
  $arg = arg(4);
  switch ($arg) {
    // argがoauth2callback＝OAuth2のコールバック処理
    case 'oauth2callback':
      bancho_gaapi_oauth2callback();
      break;
      // argにプロファイル指定＝プロファイル指定フォームへ
    case 'current':
    case 'base1':
    case 'base2':
      return drupal_get_form('bancho_gaapi_settings_profile');
      break;
    default:
      return drupal_get_form('bancho_gaapi_settings_account');
  }
}


/**
 * プロファイル情報のフォーム
 */
function bancho_gaapi_settings_account() {

  $common_data = bancho_gaapi_get_common_data();
  $status = variable_get("bancho_gaapi_profile_status", array());

  $js = 'if(event.keyCode==13){document.getElementById("%s").click()}';



  if (bancho_gaapi_profile_info('current', 'id') || bancho_gaapi_profile_info('base1', 'id') || bancho_gaapi_profile_info('base2', 'id')) {
    $form['debug'] = array(
      '#type' => 'fieldset',
      '#title' => 'debug',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    $form['debug']['manual_update'] = array(
      '#markup' => sprintf(
        '<div>%s | %s | %s | %s | %s | %s | %s<br />※リダイレクトを使うので20回を越えるとブラウザがループと判断して止めるかも。</div>', 
        l('1回処理 (全強制)', 'admin/config/bancho/bancho_gaapi/update_ga_data'), 
        l('1回処理', 'admin/config/bancho/bancho_gaapi/update_ga_data/repeat/1/1'), 
        l('3回連続処理', 'admin/config/bancho/bancho_gaapi/update_ga_data/repeat/3/1'), 
        l('5回連続処理', 'admin/config/bancho/bancho_gaapi/update_ga_data/repeat/5/1'), 
        l('10回連続処理', 'admin/config/bancho/bancho_gaapi/update_ga_data/repeat/10/1'), 
        l('20回連続処理', 'admin/config/bancho/bancho_gaapi/update_ga_data/repeat/20/1'), 
        l('ランキングを手動更新', 'admin/config/bancho/bancho_gaapi/update_ga_data/ranking')
      ),
    );

    foreach ($status as $key => $val) {
      if (!bancho_gaapi_profile_info($key, 'id')) {
        continue;
      }
      $form['debug'][$key] = array(
        '#markup' => sprintf('<div>%s (%s)</div>', check_plain($key), check_plain(bancho_gaapi_profile_info($key, 'id'))),
      );
      foreach ($val as $k => $v) {
        $form['debug']["{$key}_{$k}_name"] = array(
          '#markup' => sprintf('<div style="float: left; width: 10em; padding-left: 2em">%s</div> ', $k),
        );
        $form['debug']["{$key}_{$k}_info"] = array(
          '#markup' => sprintf('<div style="float: left; width: 13em; padding-left: 1em">%s</div> ', date("Y-m-d G:i:s", $v)),
        );
        $form['debug']["{$key}_{$k}"] = array(
          '#type' => 'submit',
          '#value' => sprintf('reset %s %s', check_plain($key), check_plain($k)),
          '#suffix' => '<br style="clear: both" />',
          '#submit' => array('bancho_gaapi_settings_account_reset_profile_status'),
          /*
 '#prefix'=>sprintf('<span style="padding: 0 1em 0 2em">%s = %s</span> ', $k, date("Y-m-d G:i:s", $v)),
 '#suffix'=>'<br />'
 */
        );
      }
    }

    $form['debug']['retry'] = array(
      '#type' => 'fieldset',
      '#title' => '特定日のデータを再取得',
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#tree' => TRUE,
    );

    $form['debug']['retry']['date'] = array(
      '#type' => 'textfield',
      '#title' => '対象日',
      '#default_value' => '',
      '#attributes' => array('onkeypress' => sprintf($js, 'edit-debug-retry-do')),
    );
    $form['debug']['retry']['desc1'] = array(
      '#markup' => '<p>フォーム送信時には[Enter]キー押下ではなく、<strong>ボタンをクリック</strong>するようにしてください。</p>',
    );
    $form['debug']['retry']['do'] = array(
      '#type' => 'submit',
      '#value' => '指定日の記事のデータを再取得',
      '#submit' => array('bancho_gaapi_settings_account_reget_ga_data'),
    );
    $form['debug']['retry']['desc2'] = array(
      '#markup' => sprintf('<div>※指定した日（%s形式で指定）に投稿された記事のデータを更新します。</div>', date("Y-m-d")),
    );
  }



  $form['max_results'] = array(
    '#type' => 'select',
    '#title' => '1回のAPIリクエストで取得するレコードの数',
    '#description' => '10000レコード取得の設定ではメモリ不足になったり実行時間がオーバーしたりする場合は下げてみてください。',
    '#options' => drupal_map_assoc(array(10000, 7500, 5000, 2500, 1000)),
    '#default_value' => bancho_gaapi_setting_info('max_results'),
  );
  $form['chunk'] = array(
    '#type' => 'select',
    '#title' => '1回の処理で実行するパス数',
    '#description' => 'プロファイルごとに何パス実行するか（基本的に1パスが1日分のデータ取得）。<br /><strong>1パス処理あたり数秒～5分弱かかります</strong>（サイトのユニークURLの数、DBのパフォーマンスやアクセス数による）。<br />累積pv数/1日前/過去7日間/30日間などの最新情報7種類を取得するのに加えて、当日/公開後1週間/公開後2週間/公開後1か月間/公開後3か月間の5種類の日付範囲でも個別に処理するうえに、それぞれのリクエストで対象URL（ga:pagePath）数が多いと1リクエストで取得しきれず2～3回（またはそれ以上）のリクエストに分けてデータを取得する必要がある場合もあります。<br />最大で「パス数×12×プロファイル数×分割取得数」回のリクエストが発生します。',
    '#options' => drupal_map_assoc(array(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 28, 29, 30, 31, 90, 180, 365)),
    '#default_value' => bancho_gaapi_setting_info('chunk'),
  );

/*
  $form['#attributes']['enctype'] = 'multipart/form-data';
  $form['inlinks_google'] = array(
    '#type' => 'file',
    '#title' => 'Google Webmaster Toolの被リンク情報を更新',
    '#size' => 40,
    '#description' => '<a href="https://www.google.com/webmasters/tools/?hl=ja">Google Webmaster Tool</a>でサイトを選び、［トラフィック］＞［サイトへのリンク］で右コンテンツエリアの［最も多くリンクされているコンテンツ］下の［詳細］をクリックした<a href="https://www.google.com/webmasters/tools/external-links-page?hl=ja&amp;siteUrl=' . rawurlencode(url(NULL, array('absolute' => TRUE))) . '">ページ</a>上部の「このテーブルをダウンロード」ボタンからダウンロードしたCSVファイルをアップロードしてください。<br /><strong>最終更新</strong>：' . (($lug = variable_get('bancho_gaapi_inlink_lu_g', 0)) ? date('Y/m/d H:i:s', $lug) : 'まだ'),
  );
*/

  $default_array = array();
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    $default_array[$i] = array(
      'run_at' => '',
      'method' => '',
    );
  }
  $ranking_settings = variable_get('bancho_gaapi_ranking_settings', $default_array);
  // ランキングプラグイン読み込み
  $ranking_methods = array('' => 'なし');
  foreach (module_implements('gaapi_ranking') as $module) {
    $list = module_invoke($module, 'gaapi_ranking', 'list');
    if (count($list) > 0) {
      foreach ($list as $key => $value) {
        $ranking_methods["{$module}|{$key}"] = check_plain($value);
      }
    }
  }
  $form["ranking"] = array(
    '#type' => 'fieldset',
    '#title' => 'ランキング',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    $form["ranking"][$i] = array(
      '#type' => 'fieldset',
      '#title' => "ランキング{$i}",
      '#collapsible' => FALSE,
      '#collapsed' => FALSE,
      '#tree' => TRUE,
    );
    $form["ranking"][$i]["run_at"] = array(
      '#type' => 'select',
      '#title' => "処理を実行する時間帯",
      '#options' => array_merge(array('' => '処理しない'), drupal_map_assoc(array(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23))),
      '#multiple' => TRUE,
      '#default_value' => $ranking_settings[$i]['run_at'],
      '#attributes' => array('style' => 'height: 6em; width: 6em'),
      '#prefix' => '<div style="width: 12em; float: left; padding-right: 2%">',
      '#suffix' => '</div>',
    );
    $form["ranking"][$i]["method"] = array(
      '#type' => 'select',
      '#title' => "処理アルゴリズム",
      '#options' => $ranking_methods,
      '#default_value' => $ranking_settings[$i]['method'],
    );
  }




  $form["run_at"] = array(
    '#type' => 'fieldset',
    '#title' => '情報を更新する時間帯',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );
  $form["run_at"]["prefix"] = array(
    '#markup' => '<div>処理時刻のdate("H")が、情報更新時間帯として指定されている場合にのみ実際の処理が実行される（複数選択）。</div>',
  );
  $run_ats = variable_get('bancho_gaapi_runats', array(
    'total' => array(1, 2, 8, 9, 15, 16, 20, 21),
    'last1day_prev' => array(2, 3, 4, 5),
    'last1day' => array(2, 3, 4, 5),
    'last7days_prev' => array(3, 4, 5, 6),
    'last7days' => array(3, 4, 5, 6),
    'last30days_prev' => array(4, 5, 6, 7),
    'last30days' => array(4, 5, 6, 7),
  ));
  foreach (array_keys($common_data['latest']) as $latest) {
    $form["run_at"]["run_{$latest}_at"] = array(
      '#type' => 'select',
      '#title' => "{$latest}",
      '#options' => array_merge(array('' => '処理しない'), drupal_map_assoc(array(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23))),
      '#multiple' => TRUE,
      '#default_value' => $run_ats[$latest],
      '#attributes' => array('style' => 'height: 6em; width: 7em'),
      '#prefix' => '<div style="width: 23%; float: left; padding-right: 2%">',
      '#suffix' => '</div>',
    );
  }
  $form["run_at"]["suffix"] = array(
    '#markup' => sprintf('<div style="clear: both">※処理状況によって後続の処理がスキップされることがあるため、必ず複数の時間帯を指定してください。複数の時間帯を指定していても、前回の実行から%s時間以上経過していなければ実行されません。</div>', round($common_data['default_interval'] / (60 * 60), 1)),
  );



  $clientID = variable_get('bancho_gaapi_clientID', NULL);
  $clientSecret = variable_get('bancho_gaapi_clientSecret', NULL);
  $form['clientID'] = array(
    '#type' => 'fieldset',
    '#title' => 'Google API OAuth 2.0 Client ID',
    '#collapsible' => FALSE,
    '#tree' => TRUE,
  );
  $form['clientID']['client_id'] = array(
    '#type' => 'textfield',
    '#title' => 'Google API Oauth Cliend ID',
    '#default_value' => check_plain($clientID),
  );
  $form['clientID']['client_secret'] = array(
    '#type' => 'textfield',
    '#title' => 'Google API Oauth Client secret',
    '#default_value' => check_plain($clientSecret),
  );
  $form['clientID']['desc'] = array(
    '#type' => 'fieldset',
    '#title' => 'Google APIコンソールでOAuth Client IDを作成する解説を読む',
    '#collapsible' => TRUE,
    '#collapsed' => $clientID ? TRUE : FALSE,
  );
  $form['clientID']['desc']['doc'] = array(
    '#markup' => '<div>How to get your Cliend ID: <br><iframe style="width: 100%; height: 800px; border: none;" src="' . base_path() . drupal_get_path('module', 'bancho_gaapi') . '/img/google_api_console.htm"></iframe></div>',
  );



  // ClientID未指定の場合はauth用フォームを出さない
  if (!$clientID || !$clientSecret) {
    $form['needClientID'] = array(
      '#markup' => '<div style="font-weight: bold; font-size: 1.2em; color: #ff0000">Google API Oauth Cliend IDとClient secretを指定して一度このフォームを保存すると、ここにアカウント認証のフォームが表示されます。</div>',
    );
  }
  else {
    $form_info = bancho_gaapi_form_strings();
    foreach ($form_info as $profile => $info) {
      $form[$profile] = array(
        '#type' => 'fieldset',
        '#title' => check_plain($info['title']),
        '#collapsible' => TRUE,
        '#collapsed' => ($profile == 'current' ? FALSE : (bancho_gaapi_profile_info($profile, 'id') ? FALSE : TRUE)),
        '#tree' => TRUE,
      );

      $form[$profile]['desc'] = array(
        '#markup' => $info['description'],
      );

      $form[$profile]['profile_info'] = array(
      );
      if (bancho_gaapi_profile_info($profile, 'id')) {
        $form[$profile]['profile_info']['#markup'] = sprintf(
          '<div>現在選択されているプロファイル：<strong>%s</strong> (%d)<br />OAuthで認証したユーザー：<strong>%s</strong></div>', 
          check_plain(bancho_gaapi_profile_info($profile, 'name')), 
          check_plain(bancho_gaapi_profile_info($profile, 'id')), 
          check_plain(bancho_gaapi_profile_info($profile, 'username'))
        );
      }
      else {
        $form[$profile]['profile_info']['#value'] = '<div>プロファイルが選択されていません。</div>';
      }

      // OAuthの認証リクエストURL用のパラメータ基本値
      // https://developers.google.com/accounts/docs/OAuth2#webserver
      // https://developers.google.com/accounts/docs/OAuth2WebServer
      // https://developers.google.com/accounts/docs/OAuth2WebServer#offline
      $google_oauth_auth_params = array(
        'response_type' => 'code',
        'client_id' => $clientID,
        'redirect_uri' => url(NULL, array('absolute' => TRUE)) . 'admin/config/bancho/bancho_gaapi/oauth2callback',
        'scope' => 'https://www.googleapis.com/auth/analytics.readonly',
        'state' => serialize(array(
          'profile' => $profile,
          'stage' => 'initial',
        )),
        'access_type' => 'offline',
        'approval_prompt' => (bancho_gaapi_profile_info($profile, 'refresh_token') ? 'auto' : 'force'),
      );
      if (! bancho_gaapi_profile_info($profile, 'id')) {
        $form[$profile]['oauth_auth'] = array(
          '#markup' => l('アカウントを認証する', 'https://accounts.google.com/o/oauth2/auth', array('query' => $google_oauth_auth_params)),
        );
      }
      else {
        $form_info = bancho_gaapi_form_strings();
        $form[$profile]['clear'] = array(
          '#type' => 'submit',
          '#value' => t('@account 用の認証情報をクリア', array('@account' => $form_info[$profile]['title'])),
          '#prefix' => '<div style="text-align: right">',
          '#suffix' => '</div>',
          '#submit' => array('bancho_gaapi_settings_account_clr_profile'),
        );
      }
    }
  }

  $form['submit_bottom'] = array(
    '#type' => 'submit',
    '#value' => t('設定を保存'),
  );



  $form['#submit'][] = 'bancho_gaapi_settings_account_submit';
  $form['#validate'][] = 'bancho_gaapi_settings_account_validate';


  if (!empty($_POST) && form_get_errors()) {
    drupal_set_message(t('The settings have not been saved because of the errors.'), 'error');
  }


  return $form;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_settings_account_validate($form, &$form_state) { }


/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_settings_account_default_submit_handler($form, &$form_state) {

  $common_data = bancho_gaapi_get_common_data();

  // とりあえず設定を保存
  bancho_gaapi_setting_info('chunk', $form_state['values']['chunk']);
  bancho_gaapi_setting_info('max_results', $form_state['values']['max_results']);

  $run_ats = array();
  foreach (array_keys($common_data['latest']) as $latest) {
    $run_ats[$latest] = $form_state['values']["run_at"]["run_{$latest}_at"];
  }
  variable_set('bancho_gaapi_runats', $run_ats);

  // 被リンクCSVがアップされていたら処理
//  bancho_gaapi_processcsv($form_state['values']['inlinks']['google']);


  $default_array = array();
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    $default_array[$i] = array(
      'run_at' => '',
      'method' => '',
    );
  }
  $ranking_settings = variable_get('bancho_gaapi_ranking_settings', $default_array);
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    $ranking_settings[$i]['run_at'] = $form_state['values']['ranking'][$i]['run_at'];
    $ranking_settings[$i]['method'] = $form_state['values']['ranking'][$i]['method'];
  }
  // 現在の設定と比較して、代わっていたらviewsのキャッシュをクリア
  $previous_ranking_settings = variable_get('bancho_gaapi_ranking_settings', array());
  if ($ranking_settings != $previous_ranking_settings) {
    variable_set('bancho_gaapi_ranking_settings', $ranking_settings);
    if (module_exists('views')) {
      views_invalidate_cache();
    }
  }

  variable_set('bancho_gaapi_clientID', $form_state['values']['clientID']['client_id']);
  variable_set('bancho_gaapi_clientSecret', $form_state['values']['clientID']['client_secret']);

  drupal_set_message('設定を保存しました。<br />各アクションを実行するにはボタンをクリックしてください（[Enter]キーでは設定保存になります）。', 'status');
  $form_state['redirect'] = 'admin/config/bancho/bancho_gaapi';

}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_settings_account_submit($form, &$form_state) {
  // 標準の送信ハンドラ
  bancho_gaapi_settings_account_default_submit_handler($form, $form_state);
}

// プロファイルの認証情報をクリア
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_settings_account_clr_profile($form, &$form_state) {
  // 標準の送信ハンドラ
  bancho_gaapi_settings_account_default_submit_handler($form, $form_state);
  // どのボタンがクリックされたかを判別
  $clr_profile = bancho_gaapi_tell_which_account_settings($form_state['clicked_button']['#value']);
  if ($clr_profile) {
    // refresh tokenをrevokeする
    $refresh_token = bancho_gaapi_profile_info($clr_profile, 'refresh_token');
    if ($refresh_token) {
      $result = drupal_http_request(
        'https://accounts.google.com/o/oauth2/revoke?token=' . $refresh_token
      );
      if ($result->error || $result->code > 200) {
        drupal_set_message(check_plain("$clr_profile のrefresh_tokenのrevokeに失敗しました。エラーコードはログに記録されています。"), 'error');
        $data = json_decode($result->data);
        watchdog('bancho_gaapi', 'Refresh token revoking failed for reason: ' . check_plain($data->error) . '.');
      }
    }

    // 認証に使ってたアカウント
    $username = bancho_gaapi_profile_info($clr_profile, 'username');
    // デフォルトのセットでクリア
    bancho_gaapi_profile_info($clr_profile, NULL, default_profile_info());

    drupal_set_message(check_plain("$clr_profile のDrupal上の情報をクリアしました。"));
  }
}

// 指定日の記事のデータを再取得
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_settings_account_reget_ga_data($form, &$form_state) {

  // 標準の送信ハンドラ
  bancho_gaapi_settings_account_default_submit_handler($form, $form_state);

  $date = strtotime($form_state['values']['debug']['retry']['date']);
  if ($date) {
    bancho_gaapi_do_infoupdate('date', $date);
  }
  else {
    drupal_set_message('再取得用に指定された日時が有効な書式ではありません', 'error');
  }
}

// ステータスをリセット
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_settings_account_reset_profile_status($form, &$form_state) {
  // 標準の送信ハンドラ
  bancho_gaapi_settings_account_default_submit_handler($form, $form_state);

  // ステータスをリセット
  if (preg_match('/^reset (..*) (..*)$/', $form_state['clicked_button']['#value'], $matches)) {

    $status = variable_get('bancho_gaapi_profile_status', array());
    $common_data = bancho_gaapi_get_common_data();
    if (in_array($matches[2], array_keys($common_data['latest']))) {
      $status[$matches[1]][$matches[2]] = 0;
    }
    else {
      $status[$matches[1]][$matches[2]] = bancho_gaapi_get_oldest_timestamp($matches[1]);
    }
    variable_set('bancho_gaapi_profile_status', $status);
    drupal_set_message(sprintf('%s %s のステータスをリセットしました。', check_plain($matches[1]), check_plain($matches[2])), 'status');
  }
}



/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_oauth2callback() {
  // https://developers.google.com/accounts/docs/OAuth2WebServer

//watchdog('bancho_debug', bancho_print_r($_REQUEST['state']));


  // OAuth側のエラーチェック
  if (isset($_REQUEST['error'])) {
    watchdog('bancho_gaapi', 'OAuth2 user authentication error: ' . check_plain($_REQUEST['error']));
    drupal_set_message('OAuth2 user authentication error: ' . check_plain($_REQUEST['error']), 'error');
    drupal_goto('admin/config/bancho/bancho_gaapi');
  }

  // OAuth自体は成功、stateで渡された情報をチェック
  $state = unserialize($_REQUEST['state']);

//dpm(array('$state',$state));

  if (!isset($state['profile'])) {
    watchdog('bancho_gaapi', 'OAuth2 user authentication succeeded, but no profile information returned. Maybe some OAuth authentication request URL might be wrong. Ask module creator.');
    drupal_set_message('OAuth2 user authentication succeeded, but no profile information returned. Maybe some OAuth authentication request URL might be wrong. Ask module creator.', 'error');
    drupal_goto('admin/config/bancho/bancho_gaapi');
  }

  $form_strings = bancho_gaapi_form_strings();
  if (! isset($form_strings[$state['profile']])) {
    watchdog('bancho_gaapi', 'OAuth2 user authentication succeeded, but profile information returned was wrong [' . check_plain($state['profile']) . ']. Maybe some OAuth authentication request URL might be wrong. Ask module creator.');
    drupal_set_message('OAuth2 user authentication succeeded, but profile information returned was wrong [' . check_plain($state['profile']) . ']. Maybe some OAuth authentication request URL might be wrong. Ask module creator.', 'error');
    drupal_goto('admin/config/bancho/bancho_gaapi');
  }

  if (!isset($state['stage'])) {
    watchdog('bancho_gaapi', 'OAuth2 user authentication succeeded, but no stage information returned. Maybe some OAuth authentication request URL might be wrong. Ask module creator.' . '<pre>states=' . check_plain(print_r($state, TRUE)) . '</pre>');
    drupal_set_message('OAuth2 user authentication succeeded, but no stage information returned. Maybe some OAuth authentication request URL might be wrong. Ask module creator.', 'error');
    drupal_goto('admin/config/bancho/bancho_gaapi');
  }

  //watchdog('bancho_debug', '<pre>' . check_plain(print_r($state,TRUE)) . '</pre>');

  switch ($state['stage']) {
    // 初期OAuth認証成功
    case 'initial':
      // 初期に返されるのはtoken取得用のcodeなので、トークンにして保存
      $code = $_REQUEST['code'];
//dpm(array('$code',$code));
      _bancho_gaapi_oauth_refresh_token($state['profile'], TRUE, $code);

      drupal_goto('admin/config/bancho/bancho_gaapi/' . $state['profile']);
      break;

    default:
      watchdog('bancho_gaapi', 'OAuth2 user authentication succeeded, but stage information returned was wrong. Maybe some OAuth authentication request URL might be wrong. Ask module creator.' . '<pre>states=' . check_plain(print_r($state, TRUE)) . '</pre>');
      drupal_set_message('OAuth2 user authentication succeeded, but stage information returned was wrong. Maybe some OAuth authentication request URL might be wrong. Ask module creator.', 'error');
      drupal_goto('admin/config/bancho/bancho_gaapi');
      break;
  }
}





/**
 * プロファイル選択のフォーム
 * OAuthからここにリダイレクトされてくる場合もある
 */
function bancho_gaapi_settings_profile() {

  $profile = arg(4);

  // プロファイル情報を取得
  $account_info = bancho_gaapi_get_profiles($profile);
//dpm(array('$account_info', $account_info));

  // 選択肢を作る
  $options = array();
  foreach ($account_info->items as $account) {
    if ($account->childLink->data->totalResults > 0) {
      foreach ($account->childLink->data->items as $property) {
        // ここで$profileを使うとcurrent|base1|base2の$profileを上書きしちゃう
        if ($property->childLink->data->totalResults > 0) {
          foreach ($property->childLink->data->items as $prof) {
            // キー値はアカウントID|プロパティID|プロファイルIDにしておく
            $options[$account->id . '|' . $property->internalWebPropertyId . '|' . $prof->id] = $account->name . ' -> ' . $property->name . ' -> ' .  $prof->name;
          }
        }
      }
    }
  }


  $form_info = bancho_gaapi_form_strings();

  $form['desc1'] = array(
    '#markup' => sprintf('<div><p><strong>%s</strong> の対象プロファイルを選択してください。</p><p>アクセスに使用したアカウント：<strong>%s</strong></p></div>', check_plain($form_info[$profile]['title']), check_plain($account_info->username)),
  );
  $form['profile'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => bancho_gaapi_profile_info($profile, 'id'),
  );
  // current, base1, base2
  $form['account'] = array(
    '#type' => 'hidden',
    '#value' => $profile,
  );
  // アカウント情報（あとでprofile_infoに保存する）
  $form['username'] = array(
    '#type' => 'hidden',
    '#value' => $account_info->username,
  );
  // 表示用に情報を引っ張る
  $form['profiles'] = array(
    '#type' => 'hidden',
    '#value' => serialize($options),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'このプロファイルを使う',
  );

  return $form;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_settings_profile_validate($form, &$form_state) { }

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_settings_profile_submit($form, &$form_state) {
  // ここのコードは「profile」とか「account」が紛らわしいので注意

  // フォームから送信された情報を整理
  $form_info = unserialize($form_state['values']['profiles']);

  // bancho_gaapiのプロファイル名（current|base1|base2)
  $profile = $form_state['values']['account'];

  // プロファイル情報保存用の変数を作る（この上でプロファイル名取得してる）
  $profile_info = bancho_gaapi_profile_info($profile);

  // 古いプロファイルIDをキープしておく
  // プロファイルIDが変わったらいろいろ処理するので（この後）
  // フォームで送信された情報で更新する前にキープしとかなきゃね
  $previous_profile_id = bancho_gaapi_profile_info($profile, 'id');


  // GAのプロファイル名
  $profile_info['name'] = $form_info[$form_state['values']['profile']];

  // 認証に使用したユーザー名
  $profile_info['username'] = $form_state['values']['username'];

  // GAのアカウント・プロパティ・プロファイルのID
  list(
    $profile_info['account_id'], 
    $profile_info['property_id'], 
    $profile_info['profile_id']
  ) = explode('|', $form_state['values']['profile']);

  // テーブルID（プロファイルIDと同じだけどね）
  $profile_info['id'] = $profile_info['profile_id'];

  // 保存する
  bancho_gaapi_profile_info($profile, NULL, $profile_info);


  // 初期statusデータをつくる
  $status = variable_get("bancho_gaapi_profile_status", array());

  $oldest = bancho_gaapi_get_oldest_timestamp($profile);

  if (!count($status)) {
    $common_data = bancho_gaapi_get_common_data();
    foreach (array_keys($common_data['todo']) as $key) {
      $status[$profile][$key] = $oldest;
    }
  }

  if (empty($status[$profile]['oldest'])) {
    $status[$profile]['oldest'] = $oldest;
  }

  // $statusを保存
  variable_set("bancho_gaapi_profile_status", $status);



  // プロファイルIDが変更された場合には
  if ($profile_info['id'] !== $previous_profile_id) {

    // どこまで過去データ（公開後○日目）を取得したかの情報をリセット
    // →リセットするといろいろ大変なので、なしにした。
    // リセットしたいときは、管理画面のdebugから手動で。
//    $status = variable_get('bancho_gaapi_profile_status', array());
//    $oldest = bancho_gaapi_get_oldest_timestamp($profile);
    //
//    $common_data = bancho_gaapi_get_common_data();
//    foreach(array_keys($common_data['todo']) as $option){
//      if (in_array($option, array_keys($common_data['latest']))){
//        $status[$profile][$option] = 0;
//      }
//      else{
//        $status[$profile][$option] = $oldest;
//      }
//    }
//    $status[$profile]['oldest'] = $oldest;
//    variable_set('bancho_gaapi_profile_status', $status);
  }

  drupal_set_message('プロファイルの選択を保存しました。', 'status');
  $form_state['redirect'] = 'admin/config/bancho/bancho_gaapi';

}


/**
 * support function
 */
/*
 * どのボタンがクリックされたかを$formのclicked_button][valueから判断して返す
 *@param $clicked_button_value
 * 送信されたフォームの$form_state['clicked_button']['#value']
 *@return
 * 'current'|'base1'|'base2'
 */
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_tell_which_account_settings($clicked_button_value) {
  $form_info = bancho_gaapi_form_strings();
  foreach ($form_info as $profile => $info) {
    if ($clicked_button_value === t('@account 用の認証情報をクリア', array('@account' => $info['title']))) {
      return $profile;
    }
  }
  return FALSE;
}


/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
/*
function bancho_gaapi_processcsv() {
  if ($_FILES['files']['size']['inlinks_google']) {
    _bancho_gaapi_processcsv($_FILES['files']['tmp_name']['inlinks_google'], 1, '_bancho_gaapi_processcsv_inlinks_google');
    drupal_set_message('アップロードされたGoogle Webmaster Toolの被リンク情報をDBに反映しました');
    variable_set('bancho_gaapi_inlink_lu_g', REQUEST_TIME);
  }
}
*/

/**
 * アップロードされたCSVの処理
 * @path: 処理対象ファイルのファイルパス
 * @skip: 最初の何行スキップするか
 * @cb: 各行の内容を処理するコールバック関数名
 */
/*
function _bancho_gaapi_processcsv($path, $skip = 0, $cb = false) {
  if (!$cb || !function_exists($cb)) {
    watchdog('bancho_critical', '_bancho_gaapi_processcsv called with no callback or wrong callback: ' . check_plain($cb));
    return FALSE;
  }
  $handle = fopen($path, "r");
  // 指定行数スキップ
  for ($i = 0; $i < $skip; $i++) {
    if (! fgets($handle)) {
      return FALSE;
    }
  }
  db_query('LOCK TABLES {bancho_gaapi} WRITE, {url_alias} READ, {watchdog} WRITE, {users} WRITE, {sessions} WRITE');
  while (($data = fgetcsv($handle, 1000, ",")) !== FALSE) {
    call_user_func($cb, $data);
  }
  db_unlock_tables();
  fclose($handle);
}
*/

/*
function _bancho_gaapi_processcsv_inlinks_google($data) {
  // Drupalの内部パスに変換
  static $basepath, $basepathlen;
  if (!isset($basepath)) {
    $basepath = url(NULL, array('absolute' => TRUE));
    $basepathlen = strlen($basepath);
  }
  if (substr($data[0], 0, $basepathlen) != $basepath) {
    return FALSE;
  }
  // 桁区切りされているのでやむを得ず
  $data[1] = str_replace(',', '', $data[1]);

  if (($nid = bancho_gaapi_isthisnode(substr($data[0], $basepathlen))) && is_numeric($data[1])) {
    $q = array($data[1], $nid);
    if (db_num_rows(db_query('SELECT nid FROM {bancho_gaapi} WHERE nid = :nid', array(':nid' => $nid)))) {
      $sql = 'UPDATE {bancho_gaapi} SET total_links_g=%d WHERE nid=%d';
    }
    else {
      $sql = 'INSERT INTO {bancho_gaapi} (total_links_g, nid) VALUES (%d, %d)';
    }
    $id = db_insert('bancho_gaapi')
      ->fields(array(
        'total_links_g' => $q['0'],
        'nid' => $q['1'],
      ))
      ->execute();
  }

}
*/





// admin/settings/bancho_gaapi/update_ga_data/repeat/3/1
// admin/config/bancho/bancho_gaapi/update_ga_data/repeat/3/1
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_manual_update() {
  variable_set('bancho_gaapi_suspend_cronrun_until', REQUEST_TIME + 3500);
  module_load_include('inc', 'bancho_gaapi', 'gaapi_core');

  // ランキング情報の手動アップデート
  if (arg(5) == 'ranking') {
    bancho_gaapi_do_rankingupdate(TRUE);
  }
  // 繰り返しありの手動アップデート（$forceなし）
  elseif (arg(5) == 'repeat' && is_numeric(arg(6)) && is_numeric(arg(7))) {
    bancho_gaapi_do_infoupdate();
    // repeat回数繰り返す
    if (arg(7) < arg(6)) {
      drupal_goto(sprintf('admin/config/bancho/bancho_gaapi/update_ga_data/repeat/%d/%d', arg(6), arg(7) + 1), array(), 302);
      exit();
    }
  }
  // 手動アップデート（$forceあり）
  elseif (!arg(5)) {
    bancho_gaapi_do_infoupdate('infoupdate', FALSE, TRUE);
  }
  variable_del('bancho_gaapi_suspend_cronrun_until');
//  cache_clear_all('variables', 'cache');
  drupal_goto('admin/config/bancho/bancho_gaapi');
}




/**
 * called from bancho_gaapi_form_alter()
 */
function bancho_gaapi_form_alter_main(&$form, &$form_state, $form_id) {
  if ($form_id != 'bancho_gaapi_settings_account') {
    return;
  }
  $options = array();
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    $options[$i] = bancho_gaapi_diminishing_ranking_default_array();
  }
  $options = variable_get('bancho_gaapi_ranking_options', $options);

  $ranking_settings = variable_get('bancho_gaapi_ranking_settings', array());
  $decrease_if_old_enough_options = drupal_map_assoc(array(86400, 172800, 345600, 604800, 1209600, 1814400, 2419200, 4838400, 9676800, 31536000, 63072000), 'format_interval');
  $decrease_every_options = drupal_map_assoc(array(86400, 172800, 345600, 604800, 1209600, 1814400, 2419200, 4838400, 9676800, 31536000), 'format_interval');
  $decrease_rate_options = drupal_map_assoc(bancho_gaapi_map_assoc_createarray(), 'bancho_gaapi_map_assoc_percentage');
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    $method = isset($ranking_settings[$i]['method']) ? $ranking_settings[$i]['method'] : '';
    if (preg_match('/^..*diminishing$/', $method)) {
      $form['ranking'][$i]['decrease_if_old_enough'] = array(
        '#type' => 'select',
        '#title' => 'これより前に投稿された記事は',
        '#options' => $decrease_if_old_enough_options,
        '#prefix' => '<br style="clear: both"/>',
        '#default_value' => $options[$i]['decrease_if_old_enough'],
      );
      $form['ranking'][$i]['decrease_every'] = array(
        '#type' => 'select',
        '#title' => '投稿日からこれだけ経過ごとに',
        '#options' => $decrease_every_options,
        '#default_value' => $options[$i]['decrease_every'],
      );
      $form['ranking'][$i]['decrease_rate'] = array(
        '#type' => 'select',
        '#title' => 'スコアをn%に減らす',
        '#options' => $decrease_rate_options,
        '#default_value' => $options[$i]['decrease_rate'],
      );
    }
  }
  $form['#submit'][] = 'bancho_gaapi_form_alter_submit';
}
/**
 * called from bancho_gaapi_form_alter_submit()
 */
function bancho_gaapi_form_alter_submit_main($form, &$form_state) {
  $options = array();
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    $options[$i] = bancho_gaapi_diminishing_ranking_default_array();
  }
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    if (preg_match('/^bancho_gaapi\|/', $form_state['values']['ranking'][$i]['method'])) {
      foreach (array('decrease_if_old_enough', 'decrease_every', 'decrease_rate') as $key) {
        if (!empty($form_state['values']['ranking'][$i][$key])) {
          $options[$i][$key] =  $form_state['values']['ranking'][$i][$key];
        }
      }
    }
  }
  variable_set('bancho_gaapi_ranking_options', $options);
}

/* support function */
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_map_assoc_createarray() {
  $a = array();
  for ($i = 100; $i > 0; $i--) {
    $a[] = $i;
  }
  return $a;
}
/* support function */
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_map_assoc_percentage($val) {
  return $val . '%';
}
