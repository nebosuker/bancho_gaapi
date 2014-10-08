<?php
/**
 * @file
 * Yahoo!リスティング広告のマイクロサイトのレポートデータ引き出し用スクリプト
 */


// GA API用コード読み込み
module_load_include('inc', 'bancho_gaapi', 'gaapi_core');



/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_tokasiki() {
  return drupal_get_form('bancho_gaapi_tokasiki_rankingform');
}


/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_tokasiki_rankingform($form, &$form_state) {
  ini_set('memory_limit', "512M");

  // textareaのCSS
  $form['#attached']['css']['textarea_border'] = array(
    'data' => 'textarea{border: 2px inset threedface !important}',
    'type' => 'inline',
  );

  $to = mktime(0, 0, 0, date("n"), 1, date("Y")) -1;
  $from = mktime(0, 0, 0, date("n", $to), 1, date("Y", $to));
  $form['from'] = array(
    '#type' => 'textfield',
    '#title' => '開始日',
    '#default_value' => date("Y-m-d", $from),
    '#description' => 'yyyy-mm-dd形式',
    '#size' => 15,
  );
  $form['to'] = array(
    '#type' => 'textfield',
    '#title' => '開始日',
    '#default_value' => date("Y-m-d", $to),
    '#description' => 'yyyy-mm-dd形式',
    '#size' => 15,
  );
  $form['num'] = array(
    '#type' => 'select',
    '#title' => '対象件数',
    '#options' => drupal_map_assoc(array(1, 3, 5, 10, 20, 30, 50, 100, 1000, 5000)),
    '#default_value' => 100,
  );
  
  $profiles = array();
  $form_info = bancho_gaapi_form_strings();
  foreach ($form_info as $profile => $info) {
    if (bancho_gaapi_profile_info($profile, 'id')) {
      $profiles[$profile] = bancho_gaapi_profile_info($profile, 'name');
    }
  }
  $form['profile'] = array(
    '#type' => 'select',
    '#title' => '対象プロファイル',
    '#options' => $profiles,
    '#default_value' => 'current',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => '処理を開始',
  );

  $form['warning'] = array(
    '#type' => 'markup',
    '#markup' => '<p style="color: #ff0000; font-weight: bold; font-size: 1.1em">※対象日数が長ければ長いほど負荷がかかり処理時間が長くなります（標準の30日分処理で約5分）。</p>',
  );
  $form['scr'] = array(
    '#type' => 'markup',
    '#markup' => '
<script type="text/javascript">
$(document).ready(function(){
  $("#edit-submit")
    .removeAttr("disabled")
    .attr("value", "処理を開始")
    .click(function(event){
      if (confirm("対象1日分あたり約10秒（30日分で約5分）かかります。開始していいですか？")){
        $(this)
          .attr("value","処理中……")
          .attr("disabled","disabled");
        this.form.submit(); // chromeだとreturn trueでうまくフォームが動かない
        return true;  // 不要？
      }
      else{
        return false;
      }
    });
});
</script>',
  );

  return $form;

}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_tokasiki_rankingform_validate($form, &$form_state) {
  if (!strtotime($form_state['values']['from'])) {
    form_set_error('from', '日付設定が不正です');
  }
  if (!strtotime($form_state['values']['to'])) {
    form_set_error('to', '日付設定が不正です');
  }
  if (strtotime($form_state['values']['from']) > strtotime($form_state['values']['to'])) {
    form_set_error('from', '日付設定が不正です（開始と終了が逆？）');
  }
}


/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_tokasiki_rankingform_submit($form, &$form_state) {


  // 処理時間が長くなるとブラウザによってはサーバーからのレスポンスを
  // うまく処理してくれない現象が起きるかも

  $batch = array(
    'title' => 'Googleアナリティクスデータを処理中……',
    'operations' => array(
      array('bancho_gaapi_tokasiki_batch', array($_POST)),
    ),
    'finished' => 'bancho_gaapi_tokasiki_batch_finished',
    'file' => drupal_get_path('module', 'bancho_gaapi_tokasiki') . '/bancho_gaapi_tokasiki.inc',
  );

  batch_set($batch);
//  batch_process('admin/reports/bancho_gaapi/tokasiki');
  batch_process();

}


function bancho_gaapi_tokasiki_batch($values,  &$context) {


//watchdog('bancho_debug', 'bancho_gaapi_tokasiki_batch');

  if (!isset($context['sandbox']['progress'])) {
//watchdog('bancho_debug', 'batch initialized');
    $context['sandbox']['progress'] = 0;
    if (empty($context['results'])) {
      $context['sandbox']['body'] = '';
      $context['sandbox']['values'] = $values;
      $context['results'] = array();
    }
  }

//watchdog('bancho_debug', 'progress = ' . $context['sandbox']['progress']);

  switch ($context['sandbox']['progress']) {


  case 0:
//watchdog('bancho_debug', 'switch 0');
    $context['message'] = 'URL別データ処理中……';
    $context['sandbox']['progress'] = 1;
    $context['finished'] = 0.1;
    break;



  case 1:
//watchdog('bancho_debug', 'switch 1');
    // まずURL別統計
    if (!bancho_gaapi_tokasiki_get_ranking_data($values)) {
      $body_html = '<p style="color: #ff0000; font-size: 2em">URL別統計の処理でエラーが発生しました。再度試してみて同じ問題が発生するようなら安田に連絡してください</p>';
    }
    else {
//watchdog('bancho_debug', 'OK: bancho_gaapi_tokasiki_get_ranking_data');
      $nodes = bancho_gaapi_tokasiki_entry(FALSE);
//  print '<pre style="width: 12em; float: left;">' . nl2br(check_plain(print_r($nodes, true))) . "</pre>";
      // ランキングのnidを処理してテンプレート変数を作る
      $startdate = strtotime($context['sandbox']['values']['from']);
      $enddate = strtotime($context['sandbox']['values']['to']);
      $rep = array(
        'RANKING_BODY1' => '',
        'RANKING_PERIOD' => date("Y年m月d日", $startdate) . '～' . date("Y年m月d日", $enddate),
      );
      if (date("Y", $startdate) != date("Y", $enddate)) {
        $rep['RANKING_PERIOD_SHORT'] = date("Y-n-j", $startdate) . '～' . date("Y-n-j", $enddate);
      }
      else {
        $rep['RANKING_PERIOD_SHORT'] = date("Y-n-j", $startdate) . '～' . date("n-j", $enddate);
      }
      $i = 0;
      foreach ($nodes as $nid => $stat) {
        if (++$i > $context['sandbox']['values']['num']) {
          break;
        }
        $str_template = "\"%s\"\t%d\t\"%s\"\t\"%s\"\t\"%s\"\t%d\t\"%s\"\t%s\t%s\t%s";
        if ($nid == 'overture') {
          $str = sprintf(
            $str_template, 
            "―", 
            $i, 
            "トップページ（overture）", 
            "―", 
            url('overture', array('absolute' => TRUE)), 
            $stat['ga:pageviews'], 
            floor($stat['ga:timeOnPage'] / 3600) . ':' . floor(($stat['ga:timeOnPage'] % 3600) / 60) . ':' . ($stat['ga:timeOnPage'] % 60),
            '-',
            '-',
            '-'
          );
        }
        elseif ($nid == 'ylisting') {
          $str = sprintf(
            $str_template, 
            "―", 
            $i, 
            "トップページ（ylisting）", 
            "―", 
            url('ylisting', array('absolute' => TRUE)), 
            $stat['ga:pageviews'], 
            floor($stat['ga:timeOnPage'] / 3600) . ':' . floor(($stat['ga:timeOnPage'] % 3600) / 60) . ':' . ($stat['ga:timeOnPage'] % 60),
            '-',
            '-',
            '-'
          );
        }
        elseif ($nid == 'yahooads') {
          $str = sprintf(
            $str_template, 
            "―", 
            $i, 
            "トップページ（yahooads）", 
            "―", 
            url('yahooads', array('absolute' => TRUE)), 
            $stat['ga:pageviews'], 
            floor($stat['ga:timeOnPage'] / 3600) . ':' . floor(($stat['ga:timeOnPage'] % 3600) / 60) . ':' . ($stat['ga:timeOnPage'] % 60),
            '-',
            '-',
            '-'
          );
        }
        else {
          $corner_name = db_query('SELECT SQL_NO_CACHE t.name FROM {taxonomy_term_data} t INNER JOIN {taxonomy_index} r ON r.tid = t.tid WHERE t.vid = :vid AND r.nid = :node_nid LIMIT 0,1', array(':vid' => variable_get('bancho_vid_corner', 4), ':node_nid' => $nid))->fetchField();
          $db_res = db_query('SELECT SQL_NO_CACHE nid, type, title, created FROM {node} WHERE nid = :nid', array(':nid' => $nid))->fetchObject();
          $sbms = array();
          foreach (array('facebook', 'twitter', 'hatena') as $service) {
            $sbms[$service] = db_query('SELECT SQL_NO_CACHE count FROM {bancho_sbm_count} WHERE nid = :nid AND service LIKE :service', array(':nid'=>$nid, ':service'=>$service))->fetchField();
          }
          $str = sprintf(
            $str_template, 
            date("Y/m/d", $db_res->created), 
            $i, 
            trim(preg_replace("/(?<!\\\\)\"/", '\\"', preg_replace('/[\r\n]/', ' ', check_plain($db_res->title)))), 
            ($corner_name ? trim(preg_replace("/(?<!\\\\)\"/", '\\"', preg_replace('/[\r\n]/', ' ', check_plain($corner_name)))) : ($db_res->type == 'story_news' ? 'ニュース' : '―')), 
            url('node/' . $nid, array('absolute' => TRUE)), 
            $stat['ga:pageviews'], 
            floor($stat['ga:timeOnPage'] / 3600) . ':' . floor(($stat['ga:timeOnPage'] % 3600) / 60) . ':' . ($stat['ga:timeOnPage'] % 60),
            $sbms['facebook'],
            $sbms['twitter'],
            $sbms['hatena']
          );
        }
//watchdog('bancho_debug', 'str=' . $str);
//        $rep['RANKING_BODY1'] .= trim(preg_replace("/(?<!\\\\)\"/", '\\"', preg_replace('/[\r\n]/', ' ', check_plain($db_res->title)))) . "\n";
        $rep['RANKING_BODY1'] .= $str . "\n";
      }
      // HTMLテンプレートにデータを埋める
      $body_html = <<<RANKING_BODY_HTML
<h2>記事別データ</h2>
<textarea style="width: 100%; height: 16em" class="resizable">
<%RANKING_PERIOD%>

公開日\t順位\t記事タイトル\tコーナー\tURL\tPV数\t平均ページ滞在時間\tいいね！\tTwitter\tはてブ
<%RANKING_BODY1%>
</textarea>
<div>※「/ylisting/××」のURLで表示されている記事は、以前に「/overture/××」だった時のアクセス数も含めて表示しています</div>
RANKING_BODY_HTML;

      foreach ($rep as $key => $value) {
        $body_html = str_replace('<%' . $key . '%>', $value, $body_html);
      }
    }
//watchdog('bancho_debug', 'phage 1 DONE');

    $context['sandbox']['body'] .=  $body_html;
    $context['message'] = 'URL別処理完了、日別データ処理中……';
    $context['finished'] = 0.25;
    $context['sandbox']['progress'] = 2;
    break;



  case 2:
//watchdog('bancho_debug', 'switch 2');

    $body_html = '';

    if (!isset($context['sandbox']['target_date'])) {
      // 日別処理
      $context['sandbox']['from_date'] = strtotime($context['sandbox']['values']['from']);
      $context['sandbox']['target_date'] = strtotime($context['sandbox']['values']['from']);
      $context['sandbox']['to_date'] = strtotime($context['sandbox']['values']['to']);
      $context['sandbox']['dates_processed'] = 0;
      $context['sandbox']['body_html2a'] = '';
      $context['sandbox']['body_html2b'] = '';
//watchdog('bancho_debug', 'target_date initialized');
    }



    $count_dates_this_loop = 0;
    $values_day = array();
    $values_day['profile'] = $context['sandbox']['values']['profile'];
    $finished = TRUE;

    while ($context['sandbox']['target_date'] <= $context['sandbox']['to_date']) {
//watchdog('bancho_debug', 'day2day ' . date("Y-m-d", $context['sandbox']['target_date']));
      $values_day['from'] = date("Y-m-d", $context['sandbox']['target_date']);
      $values_day['to'] = $values_day['from'];
      if (!bancho_gaapi_tokasiki_get_ranking_data($values_day)) {
        $body_html .= '<p style="color: #ff0000; font-size: 2em">日別統計の処理でエラーが発生しました。再度試してみて同じ問題が発生するようなら安田に連絡してください</p>';

      }
      else {
//watchdog('bancho_debug', 'OK: dates: bancho_gaapi_tokasiki_get_ranking_data #' . ++$count_dates_this_loop);
        $nodes = bancho_gaapi_tokasiki_entry(FALSE);
        $pv = 0;
        foreach ($nodes as $node) {
          $pv += $node['ga:pageviews'];
        }
        $context['sandbox']['body_html2a'] .= sprintf(
          "%s\t%d\n", 
          date("Y年n月j日", $context['sandbox']['target_date']), 
          $pv
        );
        $context['sandbox']['body_html2b'] .= $pv . "\n";
      }
      $context['sandbox']['target_date'] += 86400;
      $context['sandbox']['dates_processed']++;
      $count_dates_this_loop++;
      if ($count_dates_this_loop > 2 && $context['sandbox']['target_date'] <= $context['sandbox']['to_date']) {
        $finished =  FALSE;
        break;
      }
    }
//watchdog('bancho_debug', 'switch 2 after while');

    if ($finished) {
      $context['results']['body'] = $context['sandbox']['body'] .  '<div class="clearfix"><div style="width: 40%; float: left;"><h2>日別データ（日付＋PV）</h2><textarea style="height: 16em;" class="resizable">' . $context['sandbox']['body_html2a'] . '</textarea></div>' . '<div style="width: 40%; float: left;"><h2>日別データ（PVのみ）</h2><textarea style="height: 16em;" class="resizable">' . $context['sandbox']['body_html2b'] . '</textarea></div></div>';
      $context['finished'] = 1;
//watchdog('bancho_debug', 'switch 2 finished ' . $context['finished']);
//watchdog('bancho_debug', 'fase 2 DONE');
    }
    else {
      $context['message'] = '日別データ処理中……（全 ' . (ceil(($context['sandbox']['to_date'] - $context['sandbox']['from_date'] ) / 86400) +1) . ' 日中 ' . $context['sandbox']['dates_processed'] . ' 日分完了）';
      $context['finished'] = 0.25 + ($context['sandbox']['dates_processed'] / ceil((($context['sandbox']['to_date'] - $context['sandbox']['from_date'] ) / 86400)+1) * 0.75);
//watchdog('bancho_debug', 'date ' . $context['sandbox']['dates_processed'] . ' DONE');
//watchdog('bancho_debug', 'switch 2 NOT finished ' . $context['finished']);

    }

    break;
  }


//watchdog('bancho_debug', 'outside switch');

//bancho_debug_to_file(array('finished'=>$context['finished'], 'context'=>$context), $context['message'], '/nfs/contents1/ird/web-tan.forum.impressrd.jp/yasuda-debug-batch.txt');

//watchdog('bancho_debug', 'end bancho_gaapi_tokasiki_batch');


}

function bancho_gaapi_tokasiki_batch_finished($success, $results, $operations) {

//dpm(array('success'=>$success, 'results'=>$results, 'operations'=>$operations));
  drupal_set_message($results['body']);
}



/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_tokasiki_get_ranking_data($values) {
  ini_set('memory_limit', "512M");

//watchdog('bancho_debug', '<pre>' . check_plain(print_r($values,TRUE)) . '</pre>');

  $profile = $values['profile'];

  // API実行のパラメータを設定
  // （idsとmax-resultsはbancho_gaapi_do_apicall()が自動設定）
  $params = array(
    'metrics' => 'ga:pageviews,ga:timeOnPage,ga:exits',
    'dimensions' => 'ga:pagePath',
    'sort' => 'ga:pagePath',
    'start-date' => date("Y-m-d", strtotime($values['from'])),
    'end-date' => date("Y-m-d", strtotime($values['to']))
  );

  // まずはstaticキャッシュをクリア
  bancho_gaapi_tokasiki_entry_reset();

  $requests_done = bancho_gaapi_do_apicall(
    $profile,
    $params,
    'bancho_gaapi_tokasiki_entry',  //$callback
    'bancho_gaapi_tokasiki_entry_reset',  //$resetcallback
    'bancho_gaapi_tokasiki_entry' //$task
  );


//watchdog('bancho_debug', "\$requests_done=<pre>" . check_plain(print_r($requests_done,TRUE)). '</pre>');

  // staticキャッシュに貯めたデータを取得
  $nodes = bancho_gaapi_tokasiki_entry(FALSE);

//watchdog('bancho_debug', "\$nodes=<pre>" . check_plain(print_r($nodes,TRUE)). '</pre>');

  arsort($nodes);
//dpm($nodes);

  if ($nodes) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}




/*
 * データをリセット（初期化時、またはデータ取得途中でエラー再試行の場合
 */
function bancho_gaapi_tokasiki_entry_reset() {
  bancho_gaapi_tokasiki_entry(FALSE, FALSE, FALSE, FALSE, TRUE);
}



/**
 * GAAPIで取得した各レコードに対して呼び出されるコールバック関数
 * 
 */
function bancho_gaapi_tokasiki_entry($entry, $profile, $task, $params, $reset = FALSE) {
  // ノードに対して複数のURLでアクセスされていたらGAのga:pagePathが別になって
  // 複数の行でレポートされるので、それを1つにまとめる必要がある
  // temptableで対処

  $table = 'TEMP_bancho_gaapi_tokasiki_entry';

  // $reset指定時には$nodesクリア
  if ($reset) {
    db_query("DROP TABLE IF EXISTS {{$table}}");
    db_query("CREATE TEMPORARY TABLE {{$table}} (
      nid varchar(255) NOT NULL,
      `ga:pageviews` INT NOT NULL default 0,
      `ga:timeOnPage` INT NOT NULL default 0,
      `ga:exits` INT NOT NULL default 0,
      PRIMARY KEY (nid),
      KEY pageviews (`ga:pageviews`)
    )");
  }

  if (!$entry) {
    // データを全部返す
    // ga:timeOnPageは 計算しないとダメ
    // ga:timeOnPage/(ga:pageviews - ga:exits)
    // http://code.google.com/intl/en/apis/analytics/docs/gdata/gdataReferenceCommonCalculations.html
    $db_res = db_query("SELECT * FROM {{$table}} ORDER BY `ga:pageviews` DESC");
    $nodes = array();
    while ($row = $db_res->fetchAssoc() ) {
      $row['ga:timeOnPage'] = (int)$row['ga:timeOnPage'] / ((int)$row['ga:pageviews'] - (int)$row['ga:exits']);
      $nodes[$row['nid']] = $row;
    }
    return $nodes;
//    return $db_res->fetchAllAssoc('nid');
  }

  static $bancho_gaapi_tokasiki_target_nids;
  // 対象nidリストを取得しておく
  if (!isset($bancho_gaapi_tokasiki_target_nids)) {
    $bancho_gaapi_tokasiki_target_nids = array();
    $bancho_gaapi_tokasiki_target_tids = array(
      5054, // 5054=マイクロサイト：特集：スポンサードサーチ徹底攻略!
      5700, // 5700 スポンサードサーチ
      5701, // 5701 インタレストマッチ
      5702, // 5702 SEM、その他のコンテンツ
      5703, // 5703 活用情報、効果アップ
      5704, // 5704 事例、インタビュー
      5705  // 5705 基本、スタートガイド
    );
    $sql = 'SELECT DISTINCT n.nid FROM {node} n LEFT JOIN {taxonomy_index} ti ON n.nid = ti.nid WHERE ti.tid IN (:tids)';
    $db_res = db_query($sql, array(':tids' => $bancho_gaapi_tokasiki_target_tids));
    foreach ($db_res as $row) {
      $bancho_gaapi_tokasiki_target_nids[] = $row->nid; // 念のため
    }

//watchdog('bancho_debug', '$bancho_gaapi_tokasiki_target_nids init');
  }

  // 処理対象URLをDrupalパスとして扱えるよう前処理＆非対象チェック
  $url = bancho_gaapi_gapath2drpath($entry['dimension']['ga:pagePath']);
  if (!$url) {
    return TRUE;
  }


//watchdog('bancho_debug', 'url=' . $url);

  // ノードのページなら進む、ノードでなければ戻る
  if ($url == 'overture') {
    //watchdog('bancho', "$url <pre>" . print_r($entry,TRUE) . "</pre>");
    $nid = 'overture';
  }
  elseif ($url == 'ylisting') {
    $nid = 'ylisting';
  }
  elseif ($url == 'yahooads') {
    $nid = 'yahooads';
  }
  else {
    $nid = bancho_gaapi_is_this_node_url(drupal_get_normal_path($url));
  }
  if ($nid == 'overture' || $nid == 'ylisting' || $nid == 'yahooads' || ($nid !== FALSE && in_array($nid, $bancho_gaapi_tokasiki_target_nids))) {

//watchdog('bancho_debug', 'target node');

    // DBからデータ取得
    $node = db_query("SELECT * FROM {{$table}} WHERE nid=:nid", array(':nid'=>$nid))->fetchAssoc();

    if (!$node) {
      $node = array(
        'nid' => $nid,
        'ga:pageviews' => 0,
        'ga:timeOnPage' => 0,
        'ga:exits' => 0,
      );
    }


    $metric = 'ga:pageviews';
    if (!empty($node[$metric])) {
      $node[$metric] = (int) $node[$metric] + (int) $entry['metric'][$metric];
    }
    else {
      $node[$metric] = $entry['metric'][$metric];
    }


    $metric = 'ga:exits';
    if (!empty($node[$metric])) {
      $node[$metric] = (int) $node[$metric] + (int) $entry['metric'][$metric];
    }
    else {
      $node[$metric] = $entry['metric'][$metric];
    }

    $metric = 'ga:timeOnPage';
    // ga:timeOnPageは 計算しないとダメ
    // ga:timeOnPage/(ga:pageviews - ga:exits)
    // http://code.google.com/intl/en/apis/analytics/docs/gdata/gdataReferenceCommonCalculations.html
    // →これは最後にデータを返すときにやる
    if (!empty($node[$metric])) {
      $node[$metric] = (int) $node[$metric] + (int) $entry['metric'][$metric];
    }
    else {
      $node[$metric] = $entry['metric'][$metric];
    }

    $query = db_merge($table)->key(array('nid' => $node['nid']))
      ->fields(array(
        'nid' => $node['nid'],
        '`ga:pageviews`' => $node['ga:pageviews'],
        '`ga:exits`' => $node['ga:exits'],
        '`ga:timeOnPage`' => $node['ga:timeOnPage']
      ))
      ->execute();
  }

//watchdog('bancho_debug', 'bancho_gaapi_tokasiki_entry done');

  return TRUE;
}




