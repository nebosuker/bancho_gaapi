<?php


/**
 * called from bancho_gaapi_gaapi_ranking();
 *@param $action
 * list|update
 *@param $name
 * name of the function
 *@param $option
 * which score to update
 *@return
 *
 */
function bancho_gaapi_gaapi_ranking_main($action = 'list', $name = FALSE, $option = FALSE) {

  switch ($action) {

    case 'list':
      $list = array();
      $list['pv_last1day'] = 'PV数（昨日）';
      $list['pv_last7days'] = 'PV数（過去7日間）';
      $list['pv_last30days'] = 'PV数（過去30日間）';
      $list['pv_overall'] = 'PV数（全期間）';

      $list['pv_last1day_diminishing'] = 'PV数（昨日、投稿日時ベース漸減スコア）';
      $list['pv_last7days_diminishing'] = 'PV数（過去7日間、投稿日時ベース漸減スコア）';
      $list['pv_last30days_diminishing'] = 'PV数（過去30日間、投稿日時ベース漸減スコア）';
      $list['pv_overall_diminishing'] = 'PV数（全期間、投稿日時ベース漸減スコア）';

      $list['pv_day1'] = 'PV数（投稿初日）';
      $list['pv_1week'] = 'PV数（投稿後1週間）';
      $list['pv_2week'] = 'PV数（投稿後2週間）';
      $list['pv_1month'] = 'PV数（投稿後30日間）';
      $list['pv_3month'] = 'PV数（投稿後90日間）';

      $list['pv_day1_diminishing'] = 'PV数（投稿初日、投稿日時ベース漸減スコア）';
      $list['pv_1week_diminishing'] = 'PV数（投稿後1週間、投稿日時ベース漸減スコア）';
      $list['pv_2week_diminishing'] = 'PV数（投稿後2週間、投稿日時ベース漸減スコア）';
      $list['pv_1month_diminishing'] = 'PV数（投稿後30日間、投稿日時ベース漸減スコア）';
      $list['pv_3month_diminishing'] = 'PV数（投稿後90日間、投稿日時ベース漸減スコア）';

      $list['pv_delta_last1days'] = 'PV数増加数（昨日）';
      $list['pv_delta_last7days'] = 'PV数増加数（過去7日間）';
      $list['pv_delta_last30days'] = 'PV数増加数（過去30日間）';

      $list['pv_delta_last1days_diminishing'] = 'PV数増加数（昨日、投稿日時ベース漸減スコア）';
      $list['pv_delta_last7days_diminishing'] = 'PV数増加数（過去7日間、投稿日時ベース漸減スコア）';
      $list['pv_delta_last30days_diminishing'] = 'PV数増加数（過去30日間、投稿日時ベース漸減スコア）';
      return $list;
      break;


    case 'update':
      if (!$option || !$name) {
        return FALSE;
      }

      switch ($name) {
        case 'pv_last1day':
          $field = 'total_last1day_pv';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_last7days':
          $field = 'total_last7days_pv';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_last30days':
          $field = 'total_last30days_pv';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_overall':
          $field = 'total_pv';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;

        case 'pv_last1day_diminishing':
          $field = 'total_last1day_pv';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_last7days_diminishing':
          $field = 'total_last7days_pv';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_last30days_diminishing':
          $field = 'total_last30days_pv';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_overall_diminishing':
          $field = 'total_pv';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;

        case 'pv_day1':
          $field = 'total_day1_pv';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_1week':
          $field = 'total_1week_pv';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_2week':
          $field = 'total_2week_pv';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_1month':
          $field = 'total_1month_pv';
          bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_3month':
          $field = 'total_3month_pv';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;

        case 'pv_day1_diminishing':
          $field = 'total_day1_pv';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_1week_diminishing':
          $field = 'total_1week_pv';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_2week_diminishing':
          $field = 'total_2week_pv';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_1month_diminishing':
          $field = 'total_1month_pv';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_3month_diminishing':
          $field = 'total_3month_pv';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;

        case 'pv_delta_last1days':
          $field = 'total_last1day_pv_delta';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_delta_last7days':
          $field = 'total_last7days_pv_delta';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_delta_last30days':
          $field = 'total_last30days_pv_delta';
          $affected = bancho_gaapi_update_rankingscore($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;

        case 'pv_delta_last1days_diminishing':
          $field = 'total_last1day_pv_delta';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_delta_last7days_diminishing':
          $field = 'total_last7days_pv_delta';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;
        case 'pv_delta_last30days_diminishing':
          $field = 'total_last30days_pv_delta';
          $affected = bancho_gaapi_update_rankingscore_diminish($field, $option);
          bancho_gaapi_update_rankingscore_avarage_vars($field, $option);
          break;

        default:
          watchdog('bancho_gaapi', sprintf('FAILED to update ranking score%d - %s (affected: %d rows)', $option, check_plain($name), $affected));
          return FALSE;
          break;
      }

      watchdog('bancho_gaapi', sprintf('ranking score%d updated - %s (affected: %d rows)', $option, check_plain($name), $affected));

      return $affected; // 値を返すこと

      break;
  }
}

/**
 * ランキングの平均値データ更新
 * このデータは、views_handler_filter_gaapi_above_averageで使ってる
 * （views/views_handler_filter_gaapi_above_average.inc内）
 */
function bancho_gaapi_update_rankingscore_avarage_vars($field, $option) {
  variable_set(
    sprintf('bancho_gaapi_ranking%d_average', $option),  
    db_query(sprintf('SELECT AVG(score%d) FROM {bancho_gaapi}', $option))->fetchField()
  );
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_update_rankingscore($field, $option) {
  $db_res = db_query(sprintf(
    'UPDATE {bancho_gaapi} SET score%d = `%s`',
    $option,
    $field
  ));
  $affected_rows = $db_res->rowCount();
  return $affected_rows;
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_update_rankingscore_diminish($field, $option) {
  $options = array();
  $affected = 0;
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    $options[$i] = bancho_gaapi_diminishing_ranking_default_array();
  }
  $options = variable_get('bancho_gaapi_ranking_options', $options);

  $db_res = db_query(sprintf('SELECT SQL_NO_CACHE ga.nid, ga.%s pv, n.created FROM {bancho_gaapi} ga LEFT JOIN {node} n ON ga.nid=n.nid', $field));
  // 投稿日時からdecrease_if_old_enoughより多く経過していたら
  // 投稿日時からdecrease_every秒経過するごとに
  // PV数評価が1*(decrease_rate/100)になる
  // int(10)なのでとりあえず100倍
  foreach ($db_res as $row) {
    $time_passed = (REQUEST_TIME - $row->created);
    $time_passed = $time_passed > 0 ? $time_passed : 0;
    $diminish_multiplier = 1;
    if ($time_passed > $options[$option]['decrease_if_old_enough']) {
      $multiplier_index = $time_passed / $options[$option]['decrease_every'];
      if ($multiplier_index > 1) {
        $diminish_multiplier = $options[$option]['decrease_rate'] / 100;
        $diminish_multiplier = pow($diminish_multiplier, floor($multiplier_index)) * (1 - (1 -$diminish_multiplier) * ($multiplier_index - floor($multiplier_index)));
      }
      elseif ($multiplier_index > 0) {
        $diminish_multiplier = $options[$option]['decrease_rate'] / 100;
        $diminish_multiplier = 1 - (1 -$diminish_multiplier) * ($multiplier_index - floor($multiplier_index));
      }
    }
    $score = ceil($row->pv * $diminish_multiplier) * 100;
    /* db_query(
     'UPDATE {bancho_gaapi} SET score%d=%d WHERE nid=%d', $option, $score, $row->nid) */
    $affected += db_update('bancho_gaapi', array())
      ->fields(array(
        sprintf('score%d',$option) => $score,
      ))
      ->condition('nid', $row->nid)
      ->execute();
  }
  return $affected;
}


