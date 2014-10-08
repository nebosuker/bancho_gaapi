<?php
/**
 * @file
 * GA APIの処理の本体ファイル
 */
define("GAAPI_ACCOUNT_FEED_URL", 'https://www.googleapis.com/analytics/v3/management/accounts');
define("GAAPI_REPORT_DATA_URL", 'https://www.googleapis.com/analytics/v3/data/ga');
define("GAAPI_USERAGENT_GZIP", 'Drupal (+http://drupal.org/) (gzip)');
define("GAAPI_OLDEST_DATE_LIMIT", 1104505200);


/**
 * 各種共通設定情報
 *@return 設定の入った配列
 */
function bancho_gaapi_get_common_data() {

  $status = variable_get("bancho_gaapi_profile_status", array());

  // max_resultsは0だとdivide by zeroが出る
  $max_results = bancho_gaapi_setting_info('max_results');
  if ($max_results == 0) {
    watchdog(
      'bancho_critical', 
      '(gaapi)  bancho_gaapi_max_results reverted from ZERO to 5000', 
      array(),  
      WATCHDOG_WARNING 
    );
    $max_results = 5000;
    bancho_gaapi_setting_info('max_results', $max_results);
    watchdog('bancho_gaapi', '$max results was ZERO, set to ' . $max_results);
  }

  $profiles = array();
  $all_profiles = bancho_gaapi_form_strings();
  // プロファイルの名前（current|base1|base2）が入ってる
  foreach (array_keys($all_profiles) as $profile) {
    // GAのプロファイルIDが指定されてなかったらスキップ
    $pid = bancho_gaapi_profile_info($profile, 'id');
    if ($pid) {
      // GAのプロファイルIDが指定されているけど
      // refresh_tokenが指定されてなかったら
      // スキップしたうえで、警告出しておく
      if (bancho_gaapi_profile_info($profile, 'refresh_token')) {
        $profiles[$profile] = $pid;
      }
      else {
        watchdog('bancho_gaapi', sprintf('NEED TO RE-AUTHORIZE : %s for Google Analytics data. Clear profile data for %s at %s and re-authorize.', $all_profiles[$profile]['title'], $all_profiles[$profile]['title'], l('admin/config/bancho/bancho_gaapi', 'admin/config/bancho/bancho_gaapi')));
      }
    }
  }

  // latest系の処理実行時間帯
  $default_runats = array(4, 5, 6);
  $run_ats = variable_get('bancho_gaapi_runats', array());

  return array(
    // この上でまとめたプロファイル情報
    'profiles' => $profiles,
    // 各種処理状況を保存しておく変数
    // →bancho_gaapi.installでvariable_setしているのでそちらを参照
    'status' => $status,
    // スロットル関連の設定
    'limit' => array(
      'chunk' => bancho_gaapi_setting_info('chunk'),
      'loop' => ceil(200000 / $max_results),
    ),
    // 処理ルール。キーが処理内容で値は時間関連のオフセット
    // All datetime sent with daet("Y-m-d", t) when calling API
    //
    // total:
    //  whole period
    // +-------------------------------------------------+
    // |<----------------------------------------------->|
    // 2005-01-01                                        time()
    //
    // last1day: (=yesterday)
    // -------------------------------------------------+--
    //                                                  ^
    //                                                  time() - 86400
    //
    // last1day_prev: (=the day before yesterday)
    // ----------------------------------------------+-----
    //                                               ^
    //                                               time() - 86400*2
    //
    // last7days/last30days:
    //  last 7/30 DAYS from now
    // ------------------------------+---------------------+
    //                               |<------------------->|
    //                               time() - x            time()
    //
    // last7days_prev/last30days_prev:
    //  last 7/30 DAYS from the day 7/30 days ago
    // -----+---------------------+-------------------------
    //      |<------------------->|
    //      time() - 2x - 86400   time() - x - 86400
    //
    // day1:
    //  THE DAY the node was created
    // -----+---------------------------------------------
    //      ^
    //      node->created
    //
    // 1week/2week/1month/3month:
    //  1week/2week/1month/3month since the node was created
    // -----+--------------------------------------------+-----
    //      |<------------------------------------------>|
    //      node->created                                node->created + x
    //
    'todo' => array(
      // 累積数
      'total' => 86400,
      'last1day_prev' => 86400,
      'last1day' => 86400, // last1day_prevより後
      'last7days_prev' => 518400,
      'last7days' => 518400, // last_7days_prevより後
      'last30days_prev' => 2505600,
      'last30days' => 2505600, // last_30days_prevより後
      // 特定期間
      'day1' => 0,
      '1week' => 518400,
      '2week' => 1123200,
      '1month' => 2505600,
      '3month' => 7689600,
    ),
    // 最新日時に対する処理（chunk数繰り返さず1回のみ）のリスト
    // キーが$taskに使う文字列（todoのkey）、
    // 値は何時台なら更新するかの配列（in_array(date("H"), *)で判定）
    // ただし前回その$taskの更新から'default_interval'秒経過していなければ
    // パスする
    // 
    'latest' => array(
      'total' => (isset($run_ats['total']) && is_array($run_ats['total'])) ? $run_ats['total'] : $default_runats,
      'last1day_prev' => (isset($run_ats['last1day_prev']) && is_array($run_ats['last1day_prev'])) ? $run_ats['last1day_prev'] : $default_runats,
      'last1day' => (isset($run_ats['last1day']) && is_array($run_ats['last1day'])) ? $run_ats['last1day'] : $default_runats,
      'last7days_prev' => (isset($run_ats['last7days_prev']) && is_array($run_ats['last7days_prev'])) ? $run_ats['last7days_prev'] : $default_runats,
      'last7days' => (isset($run_ats['last7days']) && is_array($run_ats['last7days'])) ? $run_ats['last7days'] : $default_runats,
      'last30days_prev' => (isset($run_ats['last30days_prev']) && is_array($run_ats['last30days_prev'])) ? $run_ats['last30days_prev'] : $default_runats,
      'last30days' => (isset($run_ats['last30days']) && is_array($run_ats['last30days'])) ? $run_ats['last30days'] : $default_runats,
    ),
    // デフォルトの「前回からこれだけ経ってなきゃパス」秒数（2時間）
    'default_interval' => 7200,
  );
}





/**
 * ランキング情報をアップデート
 */
function bancho_gaapi_do_rankingupdate($force = FALSE) {
  bancho_profiler(1, 'all');
  $ranking_settings = variable_get('bancho_gaapi_ranking_settings', array());
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    $method = $ranking_settings[$i]['method'];
    $run_at = $ranking_settings[$i]['run_at'];
    // $method指定なし＝エラー
    if (!$method) {
      continue;
    }
    // run_atはarray
    if (!is_array($run_at)) {
      $run_at = array($run_at);
    }
    // 指定時間ないでなければパス（手動アップデート時を除く）
    if (!in_array(date("H"), $run_at) && !$force) {
      continue;
    }
    // $methodは"module_name|method_name"にしている
    if (!preg_match('/^([^|]+)\|(..*)$/', $method, $matches)) {
      continue;
    }
    $result = module_invoke($matches[1], 'gaapi_ranking', 'update', $matches[2], $i);
    if (!$result) {
      watchdog(
        'bancho_gaapi', 
        sprintf(
          '(gaapi) %s_gaapi_ranking("update", %s, %d) returned %s. (no need to take action if you repeated ranking update, because DB reaturns 0 as affected rows if there is no rows to be updated (same value))', 
          check_plain($matches[1]), 
          check_plain($matches[2]), 
          $i, 
          ($result === FALSE ? 'FALSE' : check_plain($result === 0 ? '0' : $result))
        ), 
        array(),  
        WATCHDOG_NOTICE 
      );
    }
  }
  bancho_profiler(2, 'all');
}





/**
 * GAからデータを取得して処理
 * bancho_gaapi_reget_ga_data()をマージしたので
 * ・cron実行または手動実行
 * ・指定日公開記事の情報を再取得
 * で処理が少し違う
 *
 * @param $mode
 *   infoupdate  cron実行または手動実行の通常処理
 *   date        指定日公開記事の情報を再取得
 *
 * @param $for_date timestamp
 *   1日目|1週間目などの情報を取得したいノードの公開日
 *   mktime(0,0,0,"m","d","Y")レベル
 *   $mode=="date" のときに必須。
 *
 * @param $force 
 *   variableで保存している「この時刻に更新」を無視して強制処理する場合はTRUE
 *
 */
function bancho_gaapi_do_infoupdate($mode='infoupdate', $for_date=FALSE, $force = FALSE) {
  ini_set('memory_limit', "256M");
  bancho_profiler(1, 'all');

  // $mode=="date"指定の場合は$date必須
  if ($mode == 'date' && !$for_date) {
    watchdog('bancho_critical', '(gaapi) bancho_gaapi_do_infoupdate called with $mode=="date", but without $for_date', array(), WATCHDOG_WARNING );
    return FALSE;
  }

  // 共通基本設定を取得
  $common_data = bancho_gaapi_get_common_data();

  // スロットル関連の設定
  $limit = $common_data['limit'];

  // 処理ルール。キーが処理内容で値は時間関連のオフセット
  $todo = $common_data['todo'];


  // $mode=="date"指定の場合はlatest系の処理は不要
  if ($mode == 'date') {
    foreach (array_keys($common_data['latest']) as $val) {
      unset($todo[$val]);
    }
  }


  // 各種処理状況を保存しておく変数
  // →bancho_gaapi.installでvariable_setしているのでそちらを参照
  $status = $common_data['status'];

  // 実際に実行したAPIリクエスト数をログするため
  $request_done = 0;
  $log_start = REQUEST_TIME;


  // プロファイルごとに処理していく
  foreach ($common_data['profiles'] as $profile => $pid) {

    // total|day1|1weekなどのタスクごとに処理
    foreach ($todo as $task => $offset) {

      // latest系の処理は1回だけなのでループ変数を上限に上げておく
      // （$mode=="date"では関係なし）
      if (in_array($task, array_keys($common_data['latest']))) {
        $i = $limit['chunk'];
      }
      // 期間ごとのpv数の処理は$limit['chunk']指定回数まで繰り返す
      else {
        $i = 1;
      }

      for ($i; $i <= $limit['chunk']; $i++) {

        // bancho_gaapi_prepare_apicallに渡す日付情報を指定
        
        // $mode=="date"なら引数で指定された日時
        if ($mode == 'date' ) {
          $date = $for_date;
        }
        // latest系の処理では最終更新日時を渡す
        elseif (in_array($task, array_keys($common_data['latest']))) {
          $date = $status[$profile][$task];
        }
        // latest系以外では対象日付なので最終処理日時を1日分前に進めて渡す
        else {
          $date = $status[$profile][$task] + 86400;
        }

        // bancho_gaapi_prepare_apicallでデータ取得＆処理
        $done = bancho_gaapi_prepare_apicall(
          $profile, 
          $task, 
          $date, 
          $offset, 
          $force
        );

        // エラー
        if ($done === FALSE) {
          watchdog('bancho_critical', "(gaapi) bancho_gaapi_prepare_apicall returned FALSE: " . check_plain($profile) . " " . check_plain($task) . date("Y-m-d", $date), array(), WATCHDOG_WARNING );
        }
        // 実行されなかった
        elseif ($done === 0) {
          // たぶん情報更新の必要がないので$status関連は更新しない
        }
        // 実行＆成功
        else {
          $request_done += $done;
          if (in_array($task, array_keys($common_data['latest']))) {
            // 現在の日付を入れる
            $status[$profile][$task] = REQUEST_TIME;
          }
          else {
            // 処理対象日時を入れる
            $status[$profile][$task] = $date;
          }
          // $statusを保存
          // （日時指定の場合は処理内容をvariable系に保存しない）
          if ($mode !== 'date') {
            variable_set("bancho_gaapi_profile_status", $status);
          }
        }

      } // for($i; $i <= $limit['chunk']; $i++){

      // latest系で大量のentry数を処理した場合はすでに時間がかかってるので
      // このプロファイルの逐日処理はせずに次のプロファイルに移る
      // じゃないとcronがタイムアウトするから。たぶん。
      // $mode=="date"では関係ない
      if (!$force && in_array($task, array_keys($common_data['latest'])) && $done && bancho_profiler(0, 'entries') > 20000) {
        break;
      }

    } // foreach($todo as $task=>$offset){

  } // foreach($common_data['profiles'] as $profile=>$pid){

  bancho_profiler(2, 'all');
  if ($request_done) {
    watchdog(
      'bancho_gaapi', 
      sprintf(
        '<pre>%d requests (%d path) done in %s sec %s<br />total %s updates / %s entries, max-results=%d, chunk=%d, loop=%d, <br />peakmem=%sMB, <br />processed=%s<br />%s</pre>', 
        $request_done, 
        $limit['chunk'], 
        (time() - $log_start), 
        (
          $mode == 'date' ?
          'for ' . date("Y-m-d", $for_date) :
          (arg(5) == 'repeat' && is_numeric(arg(6)) && arg(6) > 1) ? sprintf('(%d of %d batch) ', arg(7), arg(6)) : ''
        ), 
        number_format(bancho_profiler(0, 'db_update_num')), 
        number_format(bancho_profiler(0, 'entries')), 
        bancho_gaapi_setting_info('max_results'), 
        $limit['chunk'], 
        $limit['loop'], 
        round(memory_get_peak_usage(TRUE) / 1048576, 1), 
        check_plain(trim(bancho_profiler(0, 'tasks_done'), ', ')), 
        check_plain(print_r(bancho_profiler(0), TRUE))
      )
    );
  }

  // （他の場合はcronとかの可能性があるのでメッセージは出さない）
  // $mode=="date"の場合はメッセージで結果を示す
  // （他の場合はcronとかの可能性があるのでメッセージは出さない）
  if ($mode == 'date') {
    if ($request_done) {
      drupal_set_message(sprintf('%s の記事の逐日アクセス情報を更新しました。', date("Y-m-d", $date)));
    }
    else  {
      drupal_set_message(sprintf('%s の記事の逐日アクセス情報を更新しようとしましたが、何らかの理由でリクエストが実行されませんでした。%s', date("Y-m-d", $date), l('ログを見る', 'admin/reports/watchdog')));
    }
  }
}





/**
 * APIコールの準備をして、
 * 実際にAPIコールする関数を必要な回数呼び出し、
 * 結果がNGなら再実行するなどする
 *
 *@param $profile
 * current|base1|base2...
 * どのプロファイルの処理をしているか
 *
 *@param $task
 * total|day1|1week|2week|1month|3month などが入る
 *
 *@param $date
 * 処理対象日時（$profile/$task値によって動作が変わる）
 *
 *@param $offset
 * 処理時に$dateに加算/減算する値
 *
 *@return
 * エラーが発生したらFALSE
 * 成功したら実行したリクエスト数（何も実行しなければ0）
 *
 */
function bancho_gaapi_prepare_apicall($profile, $task, $date, $offset, $force = FALSE) {
  // ログ用
  $num_db_updates = bancho_profiler(0, 'db_update_num');

  // 共通基本設定を取得
  static $common_data;
  if (!isset($common_data)) {
    $common_data = bancho_gaapi_get_common_data();
  }


  // staticキャッシュをクリア
  bancho_gaapi_after_apicall_reset();

  // $request_done
  $request_done = 0;

  // 処理対象日時の不正値チェック
  // おかしい値が入ってたらリセットする
  if (!is_numeric($date) || $date < 0) {
    if (in_array($task, array_keys($common_data['latest']))) {
      $date = 0;
    }
    else {
      $date = bancho_gaapi_get_oldest_timestamp($profile);
    }
  }
  // 将来の日付は今日に戻す
  elseif ($date > REQUEST_TIME) {
    $date = mktime(0, 0, 0, date("m"), date("d"), date("Y"));
  }

//watchdog('bancho_gaapi', "START $profile $task \$date=" . date("Y-m-d", $date));
//watchdog('bancho_gaapi', "START $profile $task \$date=" . date("Y-m-d", $date) . ((in_array($task, array_keys($common_data['latest']))) ? '<br />in_array(date("H"), $common_data[\'latest\'][$task])? ' . (in_array(date("H"), $common_data['latest'][$task]) ? 'TRUE' : 'FALSE') : '') . '<br />$date < (time() - $common_data[\'default_interval\']) ? ' . ($date < (time() - $common_data['default_interval']) ? ' TRUE' : ' FALSE'));


  // start-date/end-date/metricsを設定して実行するかどうかチェック

  // totalまたはlatest系
  if ($task == 'total' || in_array($task, array_keys($common_data['latest']))) {
    // total用パラメータ設定
    if ($task == 'total') {
      // データ設定
      // totalでは、$statusの該当profileの['oldest']から現在
      // ページ別の場合はga:visitsじゃなくてga:uniquePageviewsが正しい模様
      // DBのフィールド名はvisitsのままなので注意
      // ga:visitsなしにga:uniquePageviewsが含まれているとサンプリングされず
      // dimensionがpagePathの場合大変なデータ数になるので
      // filtersでゴミを回避すること
      $metrics = 'ga:pageviews,ga:bounces,ga:entrances,ga:exits,ga:uniquePageviews';
      if (isset($common_data['status'][$profile]['oldest'])){
        $startdate = $common_data['status'][$profile]['oldest'];
      }
      else {
        $startdate = bancho_get_ga_oldest_latest_date($profile, 'latest');
      }
//    $startdate = 1104505200;
      $enddate = REQUEST_TIME;
    }
    // latest系用パラメータ設定
    else {
      // データ設定
      // metricsは全部同じ
      // ページ別の場合はga:visitsじゃなくてga:uniquePageviewsが正しい模様
      // DBのフィールド名はvisitsのままなので注意
      // ga:visitsなしにga:uniquePageviewsが含まれているとサンプリングされず
      // dimensionがpagePathの場合大変なデータ数になるので
      // filtersでゴミを回避すること
      $metrics = 'ga:pageviews,ga:uniquePageviews';
      // start-dateとend-dateをそれぞれ設定
      switch ($task) {
        case 'last1day':
          $startdate = REQUEST_TIME - $offset;
          $enddate = REQUEST_TIME - $offset;
          break;

        case 'last1day_prev':
          $startdate = REQUEST_TIME - $offset - 86400;
          $enddate = REQUEST_TIME - $offset - 86400;
          break;

        case 'last7days':
        case 'last30days':
          $startdate = REQUEST_TIME - $offset;
          $enddate = REQUEST_TIME;
          break;

        case 'last7days_prev':
        case 'last30days_prev':
          $startdate = REQUEST_TIME - (2 * $offset) - 86400;
          $enddate = REQUEST_TIME - $offset - 86400;
          break;

        default:
          watchdog('bancho_critical', "(gaapi) UNKNOWN \$task value: " . check_plain($task), array(), WATCHDOG_ERROR);
          return $request_done; // 0
      }
    }

    // 実行判定

    // $force（管理画面から実行）なら必ず実行
    if ($force) {
      // 実行
    }
    // $date=0（初期状態）か最終処理が現在より既定秒数以上前なら更新
    elseif (in_array(date("H"), $common_data['latest'][$task]) && (($date < (REQUEST_TIME - $common_data['default_interval'])) || ($date == 0))) {
      // そのプロファイルの最新データ日時を取得
      $ga_latest_date = bancho_get_ga_oldest_latest_date($profile, 'latest');

      /*
       print "profile={$profile}<br />\n";
       print "latest:" . $ga_latest_date . "/" . date("Y-m-d H:i", $ga_latest_date) ."<br />\n";
       print "status:" . $common_data['status'][$profile][$task] . "/" . date("Y-m-d H:i",$common_data['status'][$profile][$task]) . "<br />\n";
       print "status前日:" . (mktime(0,0,0,date("m",$common_data['status'][$profile][$task]),date("d",$common_data['status'][$profile][$task]),date("y",$common_data['status'][$profile][$task]))-86400) . "/" . date("Y-m-d H:i",mktime(0,0,0,date("m",$common_data['status'][$profile][$task]),date("d",$common_data['status'][$profile][$task]),date("y",$common_data['status'][$profile][$task]))-86400) . "<br />\n";
       print "suspend? : " . (($ga_latest_date && $ga_latest_date < mktime(0,0,0,date("m",$common_data['status'][$profile][$task]),date("d",$common_data['status'][$profile][$task]),date("y",$common_data['status'][$profile][$task]))-86400) ? 'TRUE' : 'FALSE') . "<br />\n";
       print "common_data:<pre>" . nl2br(check_plain(print_r($common_data,true)))."</pre>";
       exit();
       */
      // GAの最新データが最後に処理した日の前日より前なら実行しない
      if ($ga_latest_date && $ga_latest_date < mktime(0, 0, 0, date("m", $common_data['status'][$profile][$task]), date("d", $common_data['status'][$profile][$task]), date("y", $common_data['status'][$profile][$task])) -86400) {
        //watchdog('bancho_gaapi', "total_suspend={$profile}-{$task}, ga_latest=" . date("Y-m-d H:i", $ga_latest_date) . ", status=" . date("Y-m-d H:i",$common_data['status'][$profile][$task]));
        return $request_done; // 0＝最終処理日時を更新しない
      }
      else {
        // 実行
      }
    }
    else {
      return $request_done; // 0＝最終処理日時を更新しない
    }
  }


  // total/latest系以外（day1、1week、2week、1month、3monthなど）
  // TODO: ここもga:uniquePageviews使ってサンプリング回避する？
  else {
    // データ設定
    $startdate = $date;
    $enddate = $startdate + $offset;
    $metrics = 'ga:pageviews';

    // 実行判定
    // end-dateが今日の前々日より前じゃないと処理しない
    // （GAのデータは中1～2日空けないと安定しないから）
    if ($enddate > (mktime(0, 0, 0, date("m"), date("d"), date("Y")) - 172801)) {
      return $request_done; // 0
    }
    // start-dateが2005-01-01より前になってたらGAがエラー出すからパス
    elseif ($startdate < 1104505200) {
      return $request_done; // 0
    }
    else {
      // 実行
    }
  }

  //watchdog('bancho_gaapi', "$profile $task " . date("Y-m-d",$startdate) . "-" . date("Y-m-d",$enddate) . " call test"); 
  //return 0;


  // API実行のパラメータを設定
  //（idsとmax-resultsはbancho_gaapi_do_apicall()が自動設定）
  $params = array(
//    'ids'         => 'ga:' . $common_data['profiles'][$profile],
    'metrics'     => $metrics,
    'dimensions'  => 'ga:pagePath',
    'sort'        => 'ga:pagePath',
    'start-date'  => date("Y-m-d", $startdate),
    'end-date'    => date("Y-m-d", $enddate),
//    'max-results' => bancho_gaapi_setting_info('max_results'),
//    'prettyPrint'=>true, // for debug
  );
  // ga:visitsなしにga:uniquePageviewsが含まれているとサンプリングされず
  // dimensionがpagePathの場合大変なデータ数になるので
  // filtersでゴミを回避すること（一部のフィルタ対象URLはWeb担特有かも）
  // TODO：これ管理画面で指定できるほうがいいかも
  if (strstr($metrics, 'uniquePageviews') !== FALSE) {
//    array_push($params, 'filters=ga:uniquePageviews>5;ga:pagePath!~^/((aggregator|comment|l|list|popular|r|tags)/|search\\?)');
//    $params['filters']='ga:uniquePageviews%3E2;ga:pagePath!~%5E/((aggregator%7Ccomment%7Cl%7Clist%7Cpopular%7Cr%7Ctags)/%7Csearch%5C%5C?)';
    $params['filters']='ga:uniquePageviews>5;ga:pagePath!~^/((aggregator|comment|l|list|popular|r|tags)/|search\\?)';
  }

  // start-indexに使う
  $startindex = 1;

  $request_done = bancho_gaapi_do_apicall(
    $profile,
    $params,
    'bancho_gaapi_update_table',
    'bancho_gaapi_after_apicall_reset',
    $task
  );
  return $request_done;

}


/**
 * GAのコールが終わってHTTPレスポンスコードでエラーが出てなかったとき
 * （転送途中のリセットとか）、start-indexを1に戻してやり直すので、
 * そのときに実行する関数（それまでの処理をリセットするため）
 */
function bancho_gaapi_after_apicall_reset(){
  // staticキャッシュをクリア
  static $false_val = FALSE;  // bancho_gaapi_update_table参照渡しのため
  bancho_gaapi_update_table($false_val, FALSE, FALSE, FALSE, TRUE);
}




/**
 *
 *@param $profile string
 * current|base1|base2...
 * どのプロファイルの処理をしているか
 *
 *@param $params array
 * GAのAPIに渡すパラメータを配列で
 * ただし、以下の情報は自動指定されるので渡す必要なし
 *   ids => $profileを元に自動設定
 *   max-results => bancho_gaapi_setting_info('max_results')を元に自動設定
 *
 *@param $callback string
 * 1回のGAのコールが終わるたびに結果を渡すコールバック関数名
 * コールバックに渡る引数は次のとおり
 * $entry, $profile, $task, $params, FALSE
 *
 *@param $resetcallback string
 * GAのコールが終わってHTTPレスポンスコードでエラーが出てなかったとき
 * （転送途中のリセットとか）に実行する関数名
 * コールバックに渡る引数は次のとおり
 *  $profile, $params, $callback, $task
 *
 *@param $task string
 * タスク名（文字列、オプション）
 *
 *@return
 * エラーが発生したらFALSE
 * 成功したら実行したリクエスト数（何も実行しなければ0）
 *
 */
function bancho_gaapi_do_apicall($profile, &$params, $callback=NULL, $resetcallback=NULL, $task='' ) {

  ini_set('memory_limit', "256M");

//watchdog('bancho_gaapi', "$profile/$task" . ' do_apicall with params: ' . print_r($params,TRUE));


  $request_done = 0;
  $startindex = 1;
  
  // 共通基本設定を取得
  static $common_data;
  if (!isset($common_data)) {
    $common_data = bancho_gaapi_get_common_data();
  }
  // スロットル関連の設定
  $limit = $common_data['limit'];
  

  // GAに渡すparamのidsを$profileを元に指定
  $params['ids'] = 'ga:' . $common_data['profiles'][$profile];
  $params['max-results'] = bancho_gaapi_setting_info('max_results');

  // コールバック指定がなければやる意味ない
  if (!$callback) {
    watchdog('bancho_critical', '(gaapi) bancho_gaapi_do_apicall called with no $callback for ' . check_plain($profile) . '/' . check_plain($task), array(), WATCHDOG_ERROR );
    return FALSE;
  }

  // そのプロファイルでアカウント認証しておく
  $auth = bancho_gaapi_get_auth_header($profile);
  // 認証失敗したらエラー（FALSE）を返す
  if (!$auth) {
    watchdog('bancho_critical', "(gaapi) Failed authentication for " . check_plain($profile) . " username: " . check_plain(bancho_gaapi_profile_info($profile, 'username')), array(), WATCHDOG_WARNING );
    return FALSE;
  }


  // 全部処理できるまでmax_results件取得→処理を何回か回す
  $counter = 0;
  $result = FALSE;
  while (1) {
    // failsafe
    if (++$counter > $limit['loop']) {
      // うーん
      watchdog('bancho_critical', "(gaapi) could not get whole dataset for " . check_plain($profile) . " " . check_plain($task) . " (loop counter exceeded limitation of " . $limit['loop'] . ")" . ($result['totalResults'] ? check_plain(" totalResults=" . $result['totalResults'] . ", but stopped while trying {$startindex}-" . ($startindex + bancho_gaapi_setting_info('max_results') -1)) : check_plain('$result=' . print_r($result, TRUE)) ), array(), WATCHDOG_ERROR);
      return FALSE;
    }
    // ログ用
    ++$request_done;
    // 良くないんだけどね。。。
    drupal_set_time_limit(60);
//bancho_profiler(3,'set_time_limit_num',1);
//watchdog('bancho_gaapi', "API call: $profile $task {$startindex}-" . ($startindex + bancho_gaapi_setting_info('max_results')-1) . " " . $params['start-date']);


    // データ取得して、$callbackで指定された処理を実行する
    $result = bancho_gaapi_get_data(
      $auth, 
      $profile, 
      $params, 
      $startindex, 
      $callback,
      $task
    );

//watchdog('bancho_debug', 'exit'); exit();

    // エラーが返ったら
    if (!is_array($result)) {

      // リクエスト関連のエラーじゃなきゃリトライ（転送途中のリセットがある）
      if ($result == FALSE) {
        watchdog('bancho_critical', "(gaapi) bancho_gaapi_get_data failed, retrying : " . check_plain($profile) . " " . check_plain($task) . " " . check_plain($startindex) . "-" . check_plain($startindex + bancho_gaapi_setting_info('max_results') -1) . " " . $params['start-date'], array(), WATCHDOG_WARNING );
        // 微妙に追加スリープ
        usleep(100000);
        // リセット時のコールバックを実行
        if (!empty($resetcallback) && function_exists($resetcallback)) {
          $resetcallback($profile, $params, $callback, $task);
        }
        // $startindexをリセットしてwhileを1からやり直す
        $startindex = 1;
      }

      // レスポンスコードが503だったらquota exceed
      elseif ($result == 503) {
        watchdog('bancho_critical', "(gaapi) bancho_gaapi_get_data failed, QUOTA EXCEEDED : " . check_plain($profile) . " " . check_plain($task) . " " . check_plain($startindex) . "-" . check_plain($startindex + bancho_gaapi_setting_info('max_results') -1) . " " . $params['start-date'], array(), WATCHDOG_WARNING );
        return FALSE;
      }

      // 401だったら認証期限切れ。再認証
      elseif ($result == 401) {
        $auth = bancho_gaapi_get_auth_header($profile, TRUE);
      }

    }


    // 処理OK
    else {
      // このアクセスで全部の処理が完結したら
      if ((int) $result['totalResults'] <= ((int) $result['itemsPerPage'] + (int) $result['startIndex'] - 1)) {
//watchdog('bancho_gaapi', (bancho_profiler(0,'db_update_num') - $num_db_updates) . " updates / " . $result['totalResults'] . " pagePath: $profile $task {$startindex}-" . ($startindex + bancho_gaapi_setting_info('max_results')-1) . " " .$params['start-date']);
        // 処理済み項目をログ
        bancho_profiler(4, 'tasks_done', sprintf('%s-%s, ', $profile, $task));
        // whileを抜ける
        break;
      }

      // まだ残ってる場合は次のstartIndexを設定してwhileを続行
      else {
//watchdog('bancho_gaapi', (bancho_profiler(0,'db_update_num') - $num_db_updates) . " updates / " . $result['totalResults'] . " pagePath: $profile $task {$startindex}-" . ($startindex + bancho_gaapi_setting_info('max_results')-1) . " processed : " .$params['start-date'] . ", going next: " . ($result['itemsPerPage'] + $result['startIndex']));
        $startindex = (int) $result['itemsPerPage'] + (int) $result['startIndex'];
      }
    }
    // 10秒間に100アクセスまでのスロットルQuotaがある
    usleep(100000);
  }

  return $request_done;
}






/**
 * 実際にAPIを呼び出してデータを取得してパースして
 * 必要に応じてDB書き込みの関数を呼び出す。
 *
 *@param $auth
 * authトークン（http header用配列）
 *
 *@param $params
 * データ取得条件の配列。
 * キー名がmetricsやdimensionsなどのキー名で、
 * 値がそれぞれの設定値
 *
 *@param $profile
 * current|base1|base2...
 * どのプロファイルの処理をしているか
 *
 *
 *@param $callback string
 * 1回のGAのコールが終わるたびに結果を渡すコールバック関数名
 *
 *@param $task
 * total|day1|1week|2week|1month|3month などが入る
 * この関数では利用せず、bancho_gaapi_update_tableで使う
 * （文字列、オプション）
 *
 *@return
 * 処理が完了したら、冒頭で定義している$resultの中身が入った配列
 * エラー時には非array：
 *   リクエストでエラーが発生したら、httpレスポンスコード
 *   その他のエラー発生時にはFALSE
 */
function bancho_gaapi_get_data($auth, $profile, &$params, $startindex = 1, $callback=NULL, $task='') {

  // このアクセスで処理が完結するかどうかのチェック用データ
  $result = array(
    'totalResults' => 0,
    'startIndex' => 1,
    'itemsPerPage' => 10000,
  );

  // DB接続失敗時などの繰り返しtry回数
  static $common_data;
  if (!isset($common_data)) {
    $common_data = bancho_gaapi_get_common_data();
  }
  $limit = $common_data['limit'];

  // $paramを念のためにチェック
  if (!is_array($params)) {
    watchdog('bancho_critical', '(gaapi) bancho_gaapi_get_data called with wrong $parameter.', array(), WATCHDOG_ERROR);
    return FALSE;
  }


  // DATA APIで情報を取得
  $gzip = TRUE;
  bancho_profiler(4, 'gzip', ($gzip ? 1 : 0));


  // drupal_http_build_queryにしちゃダメだったはず
  $apicall_url = GAAPI_REPORT_DATA_URL . '?' . http_build_query($params, '', '&') . '&start-index=' . $startindex;

  $response = gaapi_http_request(
    $apicall_url,
    $auth,
    'GET',
    NULL,
    $gzip
  );


// debug
//watchdog('bancho_debug', '<pre>debug header:' . check_plain(print_r(array('request'=>$response['request'], 'response_header'=>$response['header']),TRUE))); $bakfile = "/tmp/gadata_{$profile}_{$task}"; ; if (!in_array($task, array_keys($common_data['latest']))){ $bakfile .= "_" . $params['start-date'];} $bakfile .= "_{$startindex}"; file_put_contents("{$bakfile}.json", $response['content']); chmod("$bakfile.json", 0777); // watchdog('bancho_gaapi', "bakfile = {$bakfile}"); $fh = fopen("{$bakfile}.json", 'w');
//$data = json_decode($response['content']); watchdog('bancho_debug', 'memory_get_peak_usage=' . memory_get_peak_usage(TRUE));



  if ($response['error'] || $response['http_code'] > 200) {
    return $response['http_code']; // エラー時は非arrayを返す
  }


  // jsonデータをまるっと読み込み
  $data = json_decode($response['content']);
  // json読み込み失敗チェック
  if (! isset($data->kind)) {
    watchdog('bancho_critical', '(gaapi) bancho_gaapi_get_data failed to decode json : <pre>' . check_plain(print_r(array('request' => $response['request'], 'response_header' => $response['header']), TRUE)) . '</pre>', array(), WATCHDOG_WARNING);
    return FALSE;
  }

  // レスポンスデータをunsetしてメモリ節約
  unset($response);


  // $result関連の情報（totalResults|startIndex|itemsPerPage）を処理
  $result['totalResults'] = $data->totalResults;
  $result['itemsPerPage'] = $data->itemsPerPage;
  $result['startIndex'] = $data->query->{'start-index'}; // 微妙に違うので注意
  $result['nextLink'] = isset($data->nextLink) ? $data->nextLink : FALSE;

  bancho_profiler(4, 'containsSampledData', ($data->containsSampledData ? 1 : 0));
  // columnHeadersを処理
  // この配列が各row内に何のデータがどの順で入っているかの情報になる
  // columnHeadersの並びはdimension→metricsで、
  // それぞれqueryで指定した順なので決め打ちOK。
  $cols = array();
  foreach ($data->columnHeaders as $col) {
    $cols[] = array(
      'type' => strtolower($col->columnType),
      'name' => $col->name,
    );
  }
  // 各rowを処理
  if (!isset($data->rows) || count($data->rows) < 1) {
    // データに対象行がなかったら、問題なしとして進めるけど記録しておく
    // 過去データのプロファイルに対しては、しばしば発生するので
    // 記録は不要かも
    watchdog('bancho_gaapi', 'bancho_gaapi_get_data retrieved NO rows for ' . check_plain($profile) . ' ' . check_plain($task) . ' startindex=' . check_plain($startindex) . ' : <pre>' . check_plain(print_r($data, TRUE)) . '</pre>');
    return $result;
  }
  else {
    foreach ($data->rows as $row) {
      $entry = array();
      foreach ($row as $key => $val) {
        $entry[$cols[$key]['type']][$cols[$key]['name']] = $val;
      }
      // 処理entry数カウント
      bancho_profiler(3, 'entries', 1);

      // DB処理失敗時のfailsafe（$limit['loop']に根拠はない。適当）
      $no_db_error = FALSE;
      for ($i = 0; $i < $limit['loop']; $i++) {

        // $callbackで指定された処理を実行
//        if (!bancho_gaapi_update_table($entry, $profile, $task, $startdate, FALSE)) {
        drupal_set_time_limit(60);
        $res = function_exists($callback) ? $callback($entry, $profile, $task, $params, FALSE) : FALSE;
        // 処理結果
        if (!$res) {

          // 書き込み失敗したら0.1秒wait
          usleep(100000);
          watchdog('bancho_critical', '(gaapi) callback func from bancho_gaapi_get_data() (name is ' . check_plain($callback) . ') returned FALSE', array(), WATCHDOG_WARNING);
        }
        else {
          $no_db_error = TRUE;
          $i = $limit['loop'];
        }
      }
      if (! $no_db_error) {
        watchdog('bancho_critical', '(gaapi) callback func from bancho_gaapi_get_data() (name is ' . check_plain($callback) . ') returned FALSE repeatedly. There should be some error there : <pre>' . check_plain(print_r($data, TRUE)) . '</pre>', array(), WATCHDOG_WARNING);
        return FALSE;
      }
    }
  }

//watchdog('bancho_gaapi', 'result=<pre>' . check_plain(print_r($result, TRUE)) . '</pre><br>startindex=' . $startindex);
  return $result;
}



/**
 * Data APIから取得したデータのパース結果1entryの情報をデータベースに反映させる
 *
 *@param $entry
 * bancho_gaapi_get_data()でデータをパースして作った1エントリ分の配列
 *
 *@param $profile
 * current|base1|base2...
 * どのプロファイルの処理をしているか
 *
 *@param $task
 * total|day1|1week|2week|1month|3month などが入る
 *
 *@param $params
 * GAの呼び出しに使ったパラメータ
 *
 *@param $reset
 * staticのキャッシュをクリア
 *
 *@return
 * 問題なければFALSE以外, 問題が発生した場合はFALSE
 *
 */
function bancho_gaapi_update_table(&$entry, $profile, $task = FALSE, $params, $reset = FALSE) {

  // latest系リスト
  static $latest_list;
  if (!isset($latest_list)) {
    $common_data = bancho_gaapi_get_common_data();
    $latest_list = array_keys($common_data['latest']);
  }

  // ノードに対して複数のURLでアクセスされていたらGAのga:pagePathが別になって
  // 複数の行でレポートされるので、それを1つにまとめる必要がある
  // キーがnidで値が配列（キーが$task、値がそれまでに記録された数）
  static $nodes = array();
  // $reset指定時には$nodesだけクリア
  if ($reset) {
    $nodes = array();
  }

  // ノードのcreatedを保持する。キーがnidで値にはcreatedを入れる
  // （$reset指定時でもクリアしなくていいよね）
  static $createds;
  if (!isset($createds)) {
    //bancho_profiler(1,'select_query_createds');
    $createds = array();
    $db_res = db_query('SELECT SQL_NO_CACHE nid,created FROM {node}');
    foreach ($db_res as $row) {
      $createds[$row->nid] = $row->created;
    }
    //bancho_profiler(2,'select_query_createds');
  }

  // 現在時刻はいちいちtime()しない
  static $time;
  if (!isset($time)) {
    $time = REQUEST_TIME;
  }

  // $entryがなければ進む意味がないので戻る
  if (!$entry) {
    // $resetのときは問題ないけど、そうじゃなきゃ何かがおかしい
    if (!$reset) {
      watchdog('bancho_gaapi', 'bancho_gaapi_update_table called with no $entry and no $reset', array(), WATCHDOG_ERROR);
    }
    // TRUEを返さないとエラー扱いでリピートされる
    return TRUE;
  }

  // 処理対象URLをDrupalパスとして扱えるよう前処理＆非対象チェック
  $url = bancho_gaapi_gapath2drpath($entry['dimension']['ga:pagePath']);
  if (!$url) {
    return TRUE;
  }

  // node/\d+相当のURLかどうか調べる
  if (bancho_gaapi_is_this_node_url($url)) {
    $path = $url;
  }
  else {
    // 違うっぽかったらget_normal_pathして調べてみる
    bancho_profiler(1, 'drupal_get_normal_path_sec');
    $path = drupal_get_normal_path($url);
    bancho_profiler(2, 'drupal_get_normal_path_sec');
    bancho_profiler(3, 'drupal_get_normal_path_num', 1);

    if (!bancho_gaapi_is_this_node_url($path)) {
      // bancho_gaapi_is_this_node_urlがそれでも失敗したときに実行するhook
      static $is_this_node_url_fail;
      if (!isset($is_this_node_url_fail)) {
        $is_this_node_url_fail = module_implements('bancho_gaapi_is_this_node_url_fail');
      }
      elseif (!empty($is_this_node_url_fail)) {
        foreach ($is_this_node_url_fail as $module) {
          $func = $module . '_bancho_gaapi_is_this_node_url_fail';
          $func($path);
        }
      }
    }

  }

  // ノードでなければ戻る、ノードのページなら進む
  $nid = bancho_gaapi_is_this_node_url($path);
  // $nidがない＝ノードじゃない
  if ($nid == FALSE) {
    // TRUEを返さないとエラー扱いでリピートされる
    return TRUE;
  }
  // $createdsにその$nidのデータがない＝ノードが削除されたとかそんなん
  elseif (!isset($createds[$nid])) {
    // TRUEを返さないとエラー扱いでリピートされる
    return TRUE;
  }
  else {
    // 同一ノードに複数URLでのアクセスがあると
    // dimensions=ga:pagePath なので複数行になるので
    // $nodesをstatic変数にして保持しておいて加算していく
    // ページングがあるのでstatic変数のクリアは$taskごと
    // ページ別の場合はga:visitsじゃなくてga:uniquePageviewsが正しい模様
    // DBのフィールド名はvisitsのままなので注意
    static $metrics = array(
      'ga:pageviews',
      'ga:uniquePageviews',
      'ga:entrances',
      'ga:exits',
      'ga:bounces',
    );
    if (isset($nodes[$nid])) {
      foreach ($metrics as $metric) {
        // uniquePageviewsの場合のみ累積じゃなくてstatic変数記録を含めての最大値だけ採用
        // ページ別の場合はga:visitsじゃなくてga:uniquePageviewsが正しい模様
        // DBのフィールド名はvisitsのままなので注意
        if ($metric == 'ga:uniquePageviews') {
          if (isset($nodes[$nid][$metric])) {
            if ((int) $nodes[$nid][$metric] < (int) $entry['metric'][$metric]) {
              $nodes[$nid][$metric] = (int) $entry['metric'][$metric];
            }
          }
        }
        else {
          if (isset($nodes[$nid][$metric])) {
            $nodes[$nid][$metric] = (int) $nodes[$nid][$metric] + (int) $entry['metric'][$metric];
          }
        }
      }
    }
    else {
      $nodes[$nid] = array();
      foreach ($metrics as $metric) {
        if (isset($entry['metric'][$metric])) {
          $nodes[$nid][$metric] = $entry['metric'][$metric];
        }
      }
    }


    // 既存のbancho_gaapiデータ行があれば取得しておく
    //bancho_profiler(1,'bancho_gaapi_table_data_GET');
    $data = bancho_gaapi_table_data($nid);
    //bancho_profiler(2,'bancho_gaapi_table_data_GET');

    // 期間全体の累積値の処理
    if ($task == 'total') {

      // 処理対象フィールド
      $fields = array(
        "{$profile}_pv",
        "{$profile}_visits",
        "{$profile}_entrances",
        "{$profile}_exits",
        "{$profile}_bounces",
        "total_pv",
        "total_visits",
        "total_entrances",
        "total_exits",
        "total_exit_rate",
        "total_bounces",
        "total_bounce_rate",
        "total_delta",
      );

      // total系を再計算
      // ページ別の場合はga:visitsじゃなくてga:uniquePageviewsが正しい模様
      // DBのフィールド名はvisitsのままなので注意
      $nodes[$nid]['total_pv'] = $data['total_pv'] - $data["{$profile}_pv"] + $nodes[$nid]['ga:pageviews'];
      $nodes[$nid]['total_visits'] = $data['total_visits'] - $data["{$profile}_visits"] + $nodes[$nid]['ga:uniquePageviews'];
      $nodes[$nid]['total_entrances'] = $data['total_entrances'] - $data["{$profile}_entrances"] + $nodes[$nid]['ga:entrances'];
      $nodes[$nid]['total_exits'] = $data['total_exits'] - $data["{$profile}_exits"] + $nodes[$nid]['ga:exits'];
      $nodes[$nid]['total_exit_rate'] = $nodes[$nid]['total_pv'] > 0 ? round($nodes[$nid]['total_exits'] / $nodes[$nid]['total_pv'] * 10000) : 0;
      $nodes[$nid]['total_bounces'] = $data['total_bounces'] - $data["{$profile}_bounces"] + $nodes[$nid]['ga:bounces'];
      $nodes[$nid]['total_bounce_rate'] = $nodes[$nid]['total_entrances'] > 0 ? round($nodes[$nid]['total_bounces'] / $nodes[$nid]['total_entrances'] * 10000) : 0;

      // bancho_gaapi_table_dataに戻すデータ
      // ページ別の場合はga:visitsじゃなくてga:uniquePageviewsが正しい模様
      // DBのフィールド名はvisitsのままなので注意
      $data["{$profile}_pv"] = $nodes[$nid]['ga:pageviews'];
      $data["{$profile}_visits"] = $nodes[$nid]['ga:uniquePageviews'];
      $data["{$profile}_entrances"] = $nodes[$nid]['ga:entrances'];
      $data["{$profile}_exits"] = $nodes[$nid]['ga:exits'];
      $data["{$profile}_bounces"] = $nodes[$nid]['ga:bounces'];
      $data["total_pv"] = $nodes[$nid]['total_pv'];
      $data["total_visits"] = $nodes[$nid]['total_visits'];
      $data["total_entrances"] = $nodes[$nid]['total_entrances'];
      $data["total_exits"] = $nodes[$nid]['total_exits'];
      $data["total_exit_rate"] = $nodes[$nid]['total_exit_rate'];
      $data["total_bounces"] = $nodes[$nid]['total_bounces'];
      $data["total_bounce_rate"] = $nodes[$nid]['total_bounce_rate'];

      // 15日目以降7日あたりの増分をtotal_deltaに入れる
      $node_age = $time - $createds[$nid];
      if ($node_age < 1296000) {
        // 15日未満だと増分は0
        $data["total_delta"] = 0;
      }
      elseif ($data["total_pv"] < $data["total_2week_pv"]) {
        // 増分マイナスは避ける
        $data["total_delta"] = 0;
      }
      else {
        $data["total_delta"] = round(
          (
            ($data["total_pv"] - $data["total_2week_pv"])
             / 
             (($node_age - 1209600) / 86400)
          )
          * 7
        );
      }

    }


    // total以外のlatest系
    elseif (in_array($task, $latest_list)) {

      // 処理対象フィールド
      $fields = array(
        "{$profile}_{$task}_pv",
        "total_{$task}_pv",
      );

      $data["{$profile}_{$task}_pv"] = $nodes[$nid]['ga:pageviews'];
      $data["total_{$task}_pv"] = $data["current_{$task}_pv"] + $data["base1_{$task}_pv"] + $data["base2_{$task}_pv"] - $data["{$profile}_{$task}_pv"] + $nodes[$nid]['ga:pageviews'];

      // last1day/last7days/last30daysの場合はdeltaも出す。
      // *_prevを先に処理しているので、ちゃんと値が出るはず
      switch ($task) {
        case 'last1day':
        case 'last7days':
        case 'last30days':
          $fields[] = "total_{$task}_pv_delta";
          // ここおかしいので直したけど、これでいいのかな
//          $delta = $total - $data["total_{$task}_prev_pv"];
          $delta = $data["total_{$task}_pv"] - $data["total_{$task}_prev_pv"];
          // とりえあずdeltaはpositiveだけ
          $data["total_{$task}_pv_delta"] = ($delta >= 0 ? $delta : 0);
          break;
      }


      // GAでは、最近の短期間でアクセス数が急上昇した場合でも
      // 期間を長くとってデータ取得すると少なく表示されるので
      // total_pvとtotal_visitsはlatest系も含めた最大値を採用する
      if ($nodes[$nid]['ga:pageviews'] > $data["{$profile}_pv"]) {
        $fields[] = "total_pv";
        $data["total_pv"] = $data['total_pv'] - $data["{$profile}_pv"] + $nodes[$nid]['ga:pageviews'];
        $fields[] = "{$profile}_pv";
        $data["{$profile}_pv"] = $nodes[$nid]['ga:pageviews'];
      }
      // ページ別の場合はga:visitsじゃなくてga:uniquePageviewsが正しい模様
      // DBのフィールド名はvisitsのままなので注意
      if ($nodes[$nid]['ga:uniquePageviews'] > $data["{$profile}_visits"]) {
        $fields[] = "total_visits";
        $data["total_visits"] = $data['total_visits'] - $data["{$profile}_visits"] + $nodes[$nid]['ga:uniquePageviews'];
        $fields[] = "{$profile}_visits";
        $data["{$profile}_visits"] = $nodes[$nid]['ga:uniquePageviews'];
      }

    }


    // ここに来るということは、totalでもなくlatestでもない
    // 特定期間の処理（$task = day1|1week|2week|1month|3month）なので
    // start-dateと同じ日がcreatedなnodeだけを対象にする
    elseif (isset($createds[$nid]) && date("Y-m-d", $createds[$nid]) === $params['start-date']) {

      // 処理対象フィールド
      $fields = array(
        "{$profile}_{$task}_pv",
        "total_{$task}_pv",
      );

      $data["{$profile}_{$task}_pv"] = $nodes[$nid]['ga:pageviews'];
      $data["total_{$task}_pv"] = $data["current_{$task}_pv"] + $data["base1_{$task}_pv"] + $data["base2_{$task}_pv"] - $data["{$profile}_{$task}_pv"] + $nodes[$nid]['ga:pageviews'];

    }


    // ここに来るのは$task = day1|1week|2week|1month|3monthだけど
    // 対象ノードがそのターゲット期間に該当しない場合
    else {
      // パスして問題なし
      return TRUE;
    }



    // 「x日目までpv」が経過していなければ累計を入れておく
    // そのノードの公開日時を日付レベルでUNIXタイムスタンプ化
    $node_created = mktime(
      0, 
      0, 
      0, 
      date("m", $createds[$nid]), 
      date("d", $createds[$nid]), 
      date("Y", $createds[$nid])
    );
    // 今日の日付を日付レベルでUNIXタイムスタンプ化
    static $date_today;
    if (!isset($date_today)) {
      $date_today = mktime(0, 0, 0, date("m"), date("d"), date("Y"));
    }

//      // 初日
//      if ($node_created == $date_today){
//        $fields[] = "{$profile}_day1_pv";
//        $fields[] = "total_day1_pv";
//        $data["{$profile}_day1_pv"] = $data["{$profile}_pv"];
//        $data["total_day1_pv"] = $data["total_pv"];
//      }

    // 7日
    if ($date_today -(86400 * 6) < $node_created ) {
      $fields[] = "{$profile}_1week_pv";
      $fields[] = "total_1week_pv";
      $data["{$profile}_1week_pv"] = $data["{$profile}_pv"];
      $data["total_1week_pv"] = $data["total_pv"];
    }

    // 14日
    if ($date_today -(86400 * 13) < $node_created) {
      $fields[] = "{$profile}_2week_pv";
      $fields[] = "total_2week_pv";
      $data["{$profile}_2week_pv"] = $data["{$profile}_pv"];
      $data["total_2week_pv"] = $data["total_pv"];
    }

    // 30日
    if ($date_today -(86400 * 29) < $node_created) {
      $fields[] = "{$profile}_1month_pv";
      $fields[] = "total_1month_pv";
      $data["{$profile}_1month_pv"] = $data["{$profile}_pv"];
      $data["total_1month_pv"] = $data["total_pv"];
    }

    // 90日
    if ($date_today -(86400 * 89) < $node_created) {
      $fields[] = "{$profile}_3month_pv";
      $fields[] = "total_3month_pv";
      $data["{$profile}_3month_pv"] = $data["{$profile}_pv"];
      $data["total_3month_pv"] = $data["total_pv"];
    }

    // 更新
    //bancho_profiler(1,'bancho_gaapi_table_data_SET');
    $result = bancho_gaapi_table_data($nid, 'SET', $data, $fields);
    //bancho_profiler(2,'bancho_gaapi_table_data_SET');
  }

  return $result;
}


/**
 * {bancho_gaapi}テーブルとのインターフェイス
 * UPDATE/INSERT時に既存データを参照する必要があり
 * ノード数が多くなるとDB参照の時間がかかるため
 * ローカルにデータを保持
 *@param $nid
 * nid
 *@param $action
 * GET|SET
 * SETを指定するとローカルのデータを更新して、
 * $fieldsで指定されたフィールドをDB更新する
 *@param $data
 * $action=SET時の更新データ（$action=GET時に返されるのと同じ形式の配列）
 *@param $data
 * $action=SET時の更新対象フィールド名の配列
 *@return
 * $action=GET時
 *  {bancho_gaapi}テーブルのそのnidのレコードを配列で返す
 * $action=SET時
 *  更新に失敗するとFALSE
 */
function bancho_gaapi_table_data(&$nid, $action = 'GET', &$data = NULL, &$fields = NULL) {
  // staticキャッシュは、メモリ消費を抑えるために
  // 各レコードをTSV値にして1レコード1値にする
  // （配列でやると1000ノードあたり10MBとかになる）
  static $nodes;
  // $nodesのTSV値とDB上のフィールドのマップ（nidを除く）
  static $map = array(
    'total_pv',
    'total_visits',
    'total_entrances',
    'total_exits',
    'total_bounces',
    'total_last1day_pv',
    'total_last1day_prev_pv',
    'total_last7days_pv',
    'total_last7days_prev_pv',
    'total_last30days_pv',
    'total_last30days_prev_pv',
    'total_day1_pv',
    'total_1week_pv',
    'total_2week_pv',
    'total_1month_pv',
    'total_3month_pv',
    'total_last1day_pv_delta',
    'total_last7days_pv_delta',
    'total_last30days_pv_delta',
    'total_exit_rate',
    'total_bounce_rate',
    'total_delta',
    'current_pv',
    'current_visits',
    'current_entrances',
    'current_exits',
    'current_bounces',
    'current_last1day_pv',
    'current_last1day_prev_pv',
    'current_last7days_pv',
    'current_last7days_prev_pv',
    'current_last30days_pv',
    'current_last30days_prev_pv',
    'current_day1_pv',
    'current_1week_pv',
    'current_2week_pv',
    'current_1month_pv',
    'current_3month_pv',
    'base1_pv',
    'base1_visits',
    'base1_entrances',
    'base1_exits',
    'base1_bounces',
    'base1_last1day_pv',
    'base1_last1day_prev_pv',
    'base1_last7days_pv',
    'base1_last30days_pv',
    'base1_last7days_prev_pv',
    'base1_last30days_prev_pv',
    'base1_day1_pv',
    'base1_1week_pv',
    'base1_2week_pv',
    'base1_1month_pv',
    'base1_3month_pv',
    'base2_pv',
    'base2_visits',
    'base2_entrances',
    'base2_exits',
    'base2_bounces',
    'base2_last1day_pv',
    'base2_last1day_prev_pv',
    'base2_last7days_pv',
    'base2_last7days_prev_pv',
    'base2_last30days_pv',
    'base2_last30days_prev_pv',
    'base2_day1_pv',
    'base2_1week_pv',
    'base2_2week_pv',
    'base2_1month_pv',
    'base2_3month_pv',
  );
  if (!isset($nodes)) {
    //bancho_profiler(1,'bancho_gaapi_table_data_init');
    //watchdog('bancho_gaapi', 'mem before $nodes = ' . number_format(memory_get_usage(TRUE)/1024, 0) . 'KB');
    // データを全部引き出す
    // メモリ節約のためにオブジェクトとか二次元配列じゃなくて
    // タブ区切りの配列にする
    $db_res = db_query(
      sprintf(
        'SELECT SQL_NO_CACHE nid,%s FROM {bancho_gaapi}',
        implode(',', $map)
      ), 
      array(),
      array('fetch'=>PDO::FETCH_ASSOC)
    );
    $nodes = array();
    foreach ($db_res as $row) {
      $row_nid = $row['nid'];
      unset($row['nid']);
      // キーがnid、値がTSV値
      $nodes[$row_nid] = implode("\t", $row);
    }
    //watchdog('bancho_gaapi', 'mem after $nodes = ' . number_format(memory_get_usage(TRUE)/1024, 0) . 'KB<br />, num = ' . count($nodes));
    //bancho_profiler(2,'bancho_gaapi_table_data_init');
  }

  // 不正値チェック
  if (!$nid || !is_numeric($nid)) {
    watchdog('bancho_critical', 'bancho_gaapi_table_data called with invalid $nid : "' . check_plain($nid) . '"', array(), WATCHDOG_ERROR);
    return FALSE;
  }


  switch ($action) {

    // 保存
    case 'SET':

      // 不正値チェック
      if (!is_array($data) || !is_array($fields)) {
        watchdog('bancho_critical', 'bancho_gaapi_table_data called with invalid $data', array(), WATCHDOG_ERROR);
        return FALSE;
      }

      // $dataをTSV値にしてstaticキャッシュを更新
      $nodes[$nid] = implode("\t", $data);


      // ここからDBを更新関連

      // 不正値チェック
      foreach ($fields as $field) {
        if (!is_numeric($data[$field])) {
          watchdog('bancho_critical', 'bancho_gaapi_table_data received non-numeric data when updating', array(), WATCHDOG_ERROR);
          return FALSE;
        }
      }


      // DB更新
      bancho_profiler(1, 'db_update_sec');
      db_merge('bancho_gaapi')
        ->key(array('nid' => $nid))
        ->fields($data) // これ$dataでいいのかな
        ->execute();
      bancho_profiler(2, 'db_update_sec');

      // DB挿入数をトラック
      bancho_profiler(3, 'db_update_num', 1);

      return TRUE;
      break;


      // 読み取り
    case 'GET':
    default:
      // staticキャッシュのTSV値を配列に戻す
      if (isset($nodes[$nid])) {
        $cached_data = explode("\t", $nodes[$nid]);
      }
      else {
        $cached_data = array();
      }
      // 返すための配列
      $retval = array();
      // $mapを使ってキーを、キャッシュ値から値を、それぞれ設定
      foreach ($map as $key => $value) {
        $retval[$value] = (isset($cached_data[$key]) ? $cached_data[$key] : 0);
      }
      return $retval;
      break;
  }
}


/**
 * ノードページかどうかの判断
 *@param $path
 * 調べたいパス
 *
 *@return
 * ノードのページ（node/[0-9]+）ならnid
 * そうでなければbooleanのFALSE
 */
function bancho_gaapi_is_this_node_url(&$path) {
  //bancho_profiler(1,'bancho_gaapi_is_this_node_url');

  // drupal_get_normal_pathの回数を減らすために
  // node/\d+のパスをまとめて取得しておく
  // キーがdst(alias)、値がnidの配列
  static $url_aliases;
  if (!isset($url_aliases)) {
    //bancho_profiler(1,'gather_url_alias_info');
    //watchdog('bancho_gaapi', 'mem before url aliases = ' . round(memory_get_usage(TRUE)/1024, 1) . 'KB');
    $url_aliases = array();
    $db_res = db_query("SELECT SQL_NO_CACHE alias, source FROM {url_alias} WHERE source LIKE :ua", array(':ua'=>'%node/%'));
    foreach ($db_res as $row) {
      if (preg_match('_^node/(\d+)$_', $row->source, $matches)) {
        $url_aliases[$row->alias] = $matches[1];
      }
    }
    //watchdog('bancho_gaapi', 'mem after url aliases = ' . round(memory_get_usage(TRUE)/1024, 1) . 'KB, num = ' . count($url_aliases));
    //bancho_profiler(2,'gather_url_alias_info');

    // preprocessのhook処理
    if ($hook_preprocess_gapath = module_implements('bancho_gaapi_preprocess_url_aliases')) {
      foreach ($hook_preprocess_gapath as $module) {
        $func = $module . '_bancho_gaapi_preprocess_url_aliases';
        $func($url_aliases);
      }
    }

  }


  if (isset($url_aliases[$path])) {
    return $url_aliases[$path];
    bancho_profiler(3, 'url_aliases_cache_hit', 1);
  }
  else {
    $parts = explode('/', $path);
    if ($parts[0] == 'node' && isset($parts[1]) && is_numeric($parts[1])) {
      return $parts[1];
    }
    else {
      return FALSE;
    }
  }
  //bancho_profiler(2,'bancho_gaapi_is_this_node_url');
  return FALSE;
}






/**
 * プロファイル情報を取得
 *@param $profile
 * current | base1 | base2
 *
 *@return
 * Management API 3.0の
 *  Accounts Collectionの戻り値そのままに加えて、
 *  各アカウントのchildLink->dataの下に、
 *    Webproperties Collectionの戻り値そのまま、さらに、
 *    各プロパティのchildLink->dataの下に、
 *      Profiles Collectionの戻り値そのまま。
 */
function bancho_gaapi_get_profiles($profile) {

  // todo： GET https://www.googleapis.com/analytics/v3/management/accounts/~all/webproperties/~all/profiles のように「~all」を使えばかなりリクエスト数を減らせるはず
  // 参考： https://developers.google.com/analytics/devguides/config/mgmt/v3/mgmtReference#collection_profiles

  // アカウント情報を保存しておく配列
  $account_info = NULL;

  // まずアカウント情報を取得
  $response = gaapi_http_request(
    GAAPI_ACCOUNT_FEED_URL, 
    bancho_gaapi_get_auth_header($profile)
  );
//dpm(array('profile http request $response', bancho_print_r($response)));
//dpm(array('profile http request $response content', bancho_print_r($response['content'])));

  if ($response['error']) {
    drupal_set_message(
      'retrieving account feed failed.<br>response=<pre>' . check_plain(print_r($response,TRUE)) . '</pre>', 
      'error'
    );
    return FALSE;
  }
  $account_info = json_decode($response['content']);


  // 各アカウントのプロパティ情報を取得
  foreach ($account_info->items as $account) {
    if ($account->childLink->type == 'analytics#webproperties') {
      $response = gaapi_http_request(
        $account->childLink->href, 
        bancho_gaapi_get_auth_header($profile)
      );
      if ($response['error']) {
        drupal_set_message(
          'retrieving web propaties feed failed', 
          'error'
        );
        return FALSE;
      }
      $account->childLink->data = json_decode($response['content']);

      // 各プロパティのプロファイルの情報を取得
      foreach ($account->childLink->data->items as $property) {
        if ($property->childLink->type == 'analytics#profiles') {
          $response = gaapi_http_request(
            $property->childLink->href, 
            bancho_gaapi_get_auth_header($profile)
          );
          if ($response['error']) {
            drupal_set_message(
              'retrieving web propaties feed failed', 
              'error'
            );
            return FALSE;
          }
          $property->childLink->data = json_decode($response['content']);
        }
      }
    }
  }

  unset($response);
  return $account_info;
}


/**
 * httpリクエストのラッパー
 */
function gaapi_http_request( $url, $headers = array(), $method = 'GET', $data = NULL, $gzip = TRUE ) {
  bancho_profiler(1, "http_request_{$method}");
  $headers = (array) $headers;
  if ($gzip) {
    $headers = array_merge(
      $headers, 
      array('User-Agent' => GAAPI_USERAGENT_GZIP, 'Accept-Encoding' => 'gzip')
    );
  }

  $result = drupal_http_request($url, array('headers' => $headers, 'method' => $method, 'data' => $data, 'timeout' => 70.0));

  if (isset($result->headers['content-encoding']) && $result->headers['content-encoding'] == 'gzip') {
    $result->data = gzdecode($result->data);
  }
//dpm(array('bancho_gaapi_http_request $result', $result));
  bancho_profiler(2, "http_request_{$method}");

//watchdog('bancho_gaapi', "dhr_result: <pre>" . check_plain(print_r($result,TRUE)) . "</pre>");

  if ((isset($result->error) && $result->error) || $result->code > 200) {
    if (isset($result->error) && $result->error == 'request timed out') {
      watchdog('bancho_critical', 'Google Data API timed out.<br /><pre>' . check_plain(print_r($result, TRUE)) . '</pre>', array(), WATCHDOG_ERROR);
    }
    else {
      watchdog('bancho_gaapi', 'Google Data API returned non-200 response.<br /><pre>' . check_plain(print_r($result, TRUE)) . '</pre>', array(), WATCHDOG_WARNING);
    }
  }

  // この変な処理はたぶんcurlからの移植の都合でこんなのになってる
  return array(
    'request' => $result->request,
    'http_code' => $result->code,
    'header' => $result->headers,
    'content' => $result->data,
    'error' => isset($result->error) ? $result->error : FALSE,
  );
}






/**
 * 各プロファイルの内部名リストと、
 * フォームに表示するための情報
 */
function bancho_gaapi_form_strings() {
  return array(
    'current' => array(
      'title' => '現行データ',
      'description' => '<p>このサイトで現在利用しているGoogleアナリティクスのプロファイルにアクセスできるアカウントでデータへの接続を許可</p>',
    ),
    'base1' => array(
      'title' => '過去データ1',
      'description' => '<p>過去に使っていた、このサイトのアクセス情報が保存されているGoogleアナリティクスのプロファイルにアクセスできるアカウントでデータへの接続を許可</p>',
    ),
    'base2' => array(
      'title' => '過去データ2',
      'description' => '<p>過去に使っていた、このサイトのアクセス情報が保存されているGoogleアナリティクスのプロファイルにアクセスできるアカウントでデータへの接続を許可</p>',
    ),
  );
}



/**
 * そのプロファイルの最古データ日時とサイトの最古ノードの日時を比較して
 * 新しいほうを処理開始日時に設定する
 *@param $profile
 * current|base1|base2...
 * どのプロファイルの処理をしているか
 *
 *@return
 * 最古タイムスタンプ
 */
function bancho_gaapi_get_oldest_timestamp($profile) {

//dpm('1$profile=' . $profile);

  // サイトの最古データ日時（念のために非公開ノードも含める）
  $site_oldest_data = db_query_range('SELECT SQL_NO_CACHE created FROM {node} ORDER BY created ASC', 0, 1)->fetchField();
  //watchdog('bancho_gaapi', "\$site_oldest_data: ".date("Y-m-d", $site_oldest_data));

  $ga_oldest_date = bancho_get_ga_oldest_latest_date($profile, 'oldest');

//dpm('2$ga_oldest_date=' . $ga_oldest_date);

  if (!$ga_oldest_date) {
    // GAのデータはシステムワイドで2005-01-01（1104505200）以降
    // これより前を指定するとエラーになる
    $ga_oldest_date = 1104505200;;
  }

  // どっちか新しいほうのoldestを採用
  $oldest_date = $site_oldest_data > $ga_oldest_date ? $site_oldest_data : $ga_oldest_date;

  // その1日前相当のタイムスタンプ形式で保存（次の日分から処理し始めるから）
  if ($oldest_date - 86400 >= GAAPI_OLDEST_DATE_LIMIT) {
    $oldest_date = $oldest_date - 86400;
  }

  return $oldest_date;
}




/**
 * GAの指定プロファイルの最古または最新のデータの日付を取得
 *@param $profile
 * プロファイルID
 *@param $mode
 * 'oldest' または 'latest'、失敗したらFALSE
 *@return
 * タイムスタンプ
 */
function bancho_get_ga_oldest_latest_date($profile, $mode = 'latest') {

//dpm('3$profile=' . $profile . ', $mode=' . $mode);

  static $oldest_latest = array();
  if (!isset($oldest_latest[$profile][$mode]) || $oldest_latest[$profile][$mode]) {

//dpm('4' . print_r($oldest_latest));

    $ga_date = FALSE;

    // 最古または最新のデータを1件だけ取得
    $params = array(
      'ids'         => 'ga:' . bancho_gaapi_profile_info($profile, 'id'),
      'filters'     => 'ga:visitors>0',
      'metrics'     => 'ga:visitors',
      'dimensions'  => 'ga:date',
      'start-date'  => '2005-01-01',
      'end-date'    => date("Y-m-d"),
      'max-results' => '1',
    );
    switch ($mode) {
      case 'oldest':
        $params['sort'] = 'ga:date';
        break;
      case 'latest':
      default:
        $params['sort'] = '-ga:date';
        break;
    }
    $response = gaapi_http_request(
      // drupal_http_build_queryにしちゃダメだったはず
      GAAPI_REPORT_DATA_URL . '?' . http_build_query($params, '', '&'), 
      bancho_gaapi_get_auth_header($profile)
    );
    //watchdog('bancho_gaapi', "bancho_get_ga_oldest_latest_date response: <pre>" . nl2br(check_plain(print_r($response,true))) . "</pre>");
    if ($response['error'] || $response['http_code'] > 200) {
      // エラーレポーティングはgaapi_http_requestでやってる
//dpm('5$response=' . print_r($response));
    }
    else {
//dpm('6$response=' . print_r($response));
      $data = json_decode($response['content']);
      // columnHeadersの並びはdimension→metricsで、
      // それぞれqueryで指定した順なので決め打ちOK。
      // GAのfeedではdimensionにga:dateが入る場合はY-m-dじゃなくYmd形式
      // なのでタイムスタンプに変換
      if (preg_match('_([0-9]{4})([0-9]{2})([0-9]{2})_', $data->rows[0][0], $matches)) {
//dpm('7$matches=' . print_r($matches));
        $oldest_latest[$profile][$mode] = mktime(0, 0, 0, $matches[2], $matches[3], $matches[1]);
      }
      else {
//dpm('8$data->rows[0][0]=' . print_r($data->rows[0][0]));
        watchdog('bancho_gaapi', 'Oldest report was not found in GA report: <pre>' . check_plain(print_r($data, TRUE)) . '</pre>');
      }
    }
  }
  return $oldest_latest[$profile][$mode];
}



/**
 * GAの返した['dimension']['ga:pagePath']を
 * Drupalのdrupal_get_normal_pathで扱えるようにする前処理
 *@param $gapath
 *  ['dimension']['ga:pagePath']
 *@return
 *  処理対象外ならFALSE
 *  それ以外ならbase_path()を除いたpath
 *
 * 
 */
function bancho_gaapi_gapath2drpath(&$gapath) {

  // preprocessのhook処理
  static $hook_preprocess_gapath;
  if (!isset($hook_preprocess_gapath)) {
    $hook_preprocess_gapath = module_implements('bancho_gaapi_preprocess_gapath');
  }
  elseif (!empty($hook_preprocess_gapath)) {
    foreach ($hook_preprocess_gapath as $module) {
      $func = $module . '_bancho_gaapi_preprocess_gapath';
      $func($gapath);
    }
  }


  // （まずURLデコード）
  $url = urldecode($gapath);

  // クエリパラメータやfragmentを除く
  static $fragment_keys = array('?', '#');
  foreach ($fragment_keys as $char) {
    if (strpos($url, $char) !== FALSE) {
      $url = substr($url, 0, strpos($url, $char));
    }
  }

  static $basepath_len;
  if (!isset($basepath_len)) {
    $basepath_len = strlen(base_path());
  }
  // base_pathを除く
  $url = substr($url, $basepath_len);

  // 明らかにノードでないものはスキップ
  // トップページ
  if ($url == '') {
    bancho_profiler(3, 'drupal_get_normal_path_skipped', 1);
    return FALSE;
  }
  // 処理対象外リスト
  static $excludes = array(
    'admin/',
    'logout',
    'user/',
    'node/add',
    'comment/',
    'search',
    'aggregator',
  );
  foreach ($excludes as $v) {
    if (substr($url, 0, strlen($v)) == $v) {
      bancho_profiler(3, 'drupal_get_normal_path_skipped', 1);
      return FALSE;
    }
  }

  return $url;
}



/**
 * プロファイル情報のvariable_(get|set)とのインターフェイス
 *@param $profile
 *  'current'|'base1'|'base2'
 *@param $part
 *  'id'|'name'
 *@param $data
 *  なしなら情報の取得、ありならそのデータを保存
 *@return
 *  $dataなしなら
 *    $partが指定されていればその情報を返す（リテラル）
 *    $partが指定されていなければプロファイル情報全部を返す（配列）
 */
function bancho_gaapi_profile_info($profile, $part = NULL, $data = NULL) {
  static $profile_info;
  
  // staticキャッシュの処理
  if (!isset($profile_info)) {
    $profile_info = array();
    foreach (bancho_gaapi_form_strings() as $key => $val) {
      $profile_info[$key] = NULL;
    }
  }

  // 指定された$profileのデータがなければとる
  if (is_null($profile_info[$profile])) {
    $profile_info[$profile] = variable_get(
      "bancho_gaapi_profile_info_{$profile}",  
      default_profile_info()
    );
  }
  if (is_null($data)) {
    if (is_null($part)) {
      return $profile_info[$profile];
    }
    elseif (isset($profile_info[$profile][$part])) {
      return $profile_info[$profile][$part];
    }
    else {
      return FALSE;
    }
  }
  else {
    if (is_null($part)) {
      $profile_info[$profile] = $data;
    }
    elseif (isset($profile_info[$profile][$part])) {
      $profile_info[$profile][$part] = $data;
    }
    variable_set("bancho_gaapi_profile_info_{$profile}", $profile_info[$profile]);
  }
}
function default_profile_info() {
  return array(
    'id' => '',
    'name' => '',
    'username' => '',
    'account_id' => '',
    'property_id' => '',
    'profile_id' => '', // idと同じだけどね
    'access_token' => '',
    'token_type' => '',
    'refresh_token' => '',
    'expires' => '',
  );
}


/**
 * bancho_gaapiの設定情報のvariable_(get|set)とのインターフェイス
 *@param $part
 *  'max_results'|'chunk'
 *@param $data
 *  なしなら情報の取得、ありならそのデータを保存
 *@return
 *  $dataなしなら
 *    $partが指定されていればその情報を返す（リテラル）
 *    $partが指定されていなければプロファイル情報全部を返す（配列）
 */
function bancho_gaapi_setting_info($part = NULL, $data = NULL) {
  static $setting_info;
  if (!isset($setting_info)) {
    $setting_info = variable_get('bancho_gaapi_setting_info', array('chunk' => 1, 'max_results' => 5000));
  }
  if (is_null($data)) {
    if (is_null($part)) {
      return $setting_info;
    }
    elseif (isset($setting_info[$part])) {
      return $setting_info[$part];
    }
    else {
      return FALSE;
    }
  }
  else {
    if (is_null($part)) {
      $setting_info = $data;
    }
    elseif (isset($setting_info[$part])) {
      $setting_info[$part] = $data;
    }
    variable_set('bancho_gaapi_setting_info', $setting_info);
  }

}




/**
 * @param $profile
 *  current | base1 | base2
 * APIアクセス用のAuthorizationヘッダーを返す
 * expireしてて更新できてなかったらFALSEを返す
 */
function bancho_gaapi_get_auth_header($profile, $force = FALSE) {
  // 必要に応じてtokenをリフレッシュ
  bancho_gaapi_oauth_refresh_token($profile, $force);
  $access_token = bancho_gaapi_profile_info($profile, 'access_token');
  $expires = bancho_gaapi_profile_info($profile, 'expires');
  if ($access_token && REQUEST_TIME < $expires) {
    return array('Authorization' => 'Bearer ' . $access_token);
  }
  else {
    return FALSE;
  }
}

/**
 * OAuthのtokenがexpireしてたら（またはexpire情報がなかったら）refresh
 * https://developers.google.com/accounts/docs/OAuth2WebServer#refresh
 * @param $profile
 *  current | base1 | base2
 */
function bancho_gaapi_oauth_refresh_token($profile, $force = FALSE) {
  $expires = bancho_gaapi_profile_info($profile, 'expires');
//dpm(array('bancho_gaapi_profile_info=',bancho_gaapi_profile_info($profile)));
  if (($force || !$expires || REQUEST_TIME > $expires) && bancho_gaapi_profile_info($profile, 'refresh_token')) {
    _bancho_gaapi_oauth_refresh_token($profile);
  }
}
/**
 * OAuthのtokenをrefresh
 * https://developers.google.com/accounts/docs/OAuth2WebServer#refresh
 * @param $profile
 *  current | base1 | base2
 * @param $initial
 *   最初のOAuth認証の場合にTRUE（指定しなければrefresh扱い）
 * @param $key
 *   $initial指定時にcodeを渡す必要がある
 */
function _bancho_gaapi_oauth_refresh_token($profile, $initial = FALSE, $code = '') {

  $clientID = variable_get('bancho_gaapi_clientID', NULL);
  $clientSecret = variable_get('bancho_gaapi_clientSecret', NULL);
  $profile_info = bancho_gaapi_profile_info($profile);
//dpm(array('$profile_info',$profile_info));

  $http_params = array(
    'client_id' => $clientID,
    'client_secret' => $clientSecret,
  );

  // 最初の取得とrefreshとでパラメータは違う
  if ($initial) {
    $http_params = array_merge($http_params, array(
      'redirect_uri' => url('admin/config/bancho/bancho_gaapi/oauth2callback', array('absolute' => TRUE)),
      'code' => $code,
      'grant_type' => 'authorization_code',
    ));
  }
  else {
    $http_params = array_merge($http_params, array(
      'refresh_token' => $profile_info['refresh_token'],
      'grant_type' => 'refresh_token',
    ));
  }

  // 実際にリクエスト送信
  $o = drupal_http_request(
    'https://accounts.google.com/o/oauth2/token', 
    array(
      // ここは/token
      'headers' => array(
        'Host' => 'accounts.google.com',
        'Content-Type' => 'application/x-www-form-urlencoded',
      ),
      'method' => 'POST',
      'data' => drupal_http_build_query($http_params, '', '&')
  ));

  if ($o->code == 200) {
    $data = json_decode($o->data);
    foreach ($data as $key => $val) {
      switch ($key) {
        case 'expires_in':
          // expiresは「何秒後に切れる」から「いつ切れる」に直しておく
          $profile_info['expires'] = REQUEST_TIME + $val;
          break;
        default:
          $profile_info[$key] = $val;
          break;
      }
    }

    // refresh_tokenは最初の回にしか返してくれないので、
    // 今回の取得で返されていなくて
    // 保存されていた$profile_infoにもなかったら問題発生
    if (!isset($data->refresh_token) && ! $profile_info['refresh_token']) {
      // 記録しておく
      watchdog('bancho_gaapi', 'ERROR: no refresh_token (retrieved) for Client ID: ' . check_plain($clientID) . "<br>profile info: <br><pre>" . check_plain(print_r($profile_info, TRUE)) . "</pre><br>http_result:<br><pre>" . check_plain(print_r($o, TRUE)) . "</pre><br>time: " . REQUEST_TIME);
    }

    // 保存
    bancho_gaapi_profile_info($profile, NULL, $profile_info);
//    watchdog('bancho_gaapi', sprintf('%s access token for %s. Expires in %d, that is %s.', ($initial ? 'Retrieved' : 'Refreshed'), $profile, $data->expires_in, strftime('%F %T', $profile_info['expires'])));
  }
  else {
    watchdog('bancho_gaapi', 'ERROR: cannot retrieve token for ' . check_plain($profile) . "<br>profile info: <br><pre>" . check_plain(print_r($profile_info, TRUE)) . "</pre><br>http_result:<br><pre>" . check_plain(print_r($o, TRUE)) . "</pre><br>time: " . REQUEST_TIME);
  }
}


/**
 * 処理時間計測用
 */
if (!function_exists('bancho_profiler')) {
  function bancho_profiler($action = 0, $id = FALSE, $data = FALSE) {

    static $profile_data = array();

    switch ($action) {
      case 0: // return data
        if ($id) {
          if (isset($profile_data[$id][0])) {
            return $profile_data[$id][0];
          }
        }
        else {
          $result = array();
          foreach ($profile_data as $key => $val) {
            $result[$key] = $profile_data[$key][0];
          }
          return $result;
        }
        break;

      case 1: // BEGIN
        $profile_data[$id][1] = microtime(TRUE);
        if (!isset($profile_data[$id][0])) {
          $profile_data[$id][0] = 0;
        }
        break;

      case 2: // STOP
        $profile_data[$id][0] = $profile_data[$id][0] + round(microtime(TRUE) - $profile_data[$id][1], 3);
        break;

      case 3: // add data
        if (!isset($profile_data[$id][0])) {
          $profile_data[$id][0] = 0;
        }
        $profile_data[$id][0] = $profile_data[$id][0] + $data;
        break;

      case 4: // append data
        if (!isset($profile_data[$id][0])) {
          $profile_data[$id][0] = '';
        }
        $profile_data[$id][0] = $profile_data[$id][0] . $data;
        break;
    }
  }
}



/**
 * http://www.php.net/manual/ja/function.gzdecode.php#84174
 */
if (!function_exists('gzdecode')) {
  function gzdecode($data) {
    $g = tempnam('/tmp', 'gadata');
    @file_put_contents($g, $data);
    ob_start();
    readgzfile($g);
    $d = ob_get_clean();
    unlink($g);
    return $d;
  }
}
