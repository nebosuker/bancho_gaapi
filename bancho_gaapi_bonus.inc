<?php


/**
 * Implements hook_gaapi_ranking
().
 *@param $action
 * list|update
 *@param $name
 * name of the function
 *@param $option
 * which score to update
 *@return
 *
 */
function bancho_gaapi_bonus_gaapi_ranking_main($action = 'list', $name = FALSE, $option = FALSE) {

  switch ($action) {

    case 'list':
      $list = array();
      $list['total_bounce_rate'] = '直帰率の低さ（PV数考慮）';
      $list['total_exit_rate'] = '離脱率の低さ（PV数考慮）';
      $list['total_bounce_rate_custom'] = '直帰率の低さ（PV数考慮）（カスタム）';
      $list['total_exit_rate_custom'] = '離脱率の低さ（PV数考慮）（カスタム）';
      return $list;
      break;


    case 'update':
      if (!$option || !$name) {
        return FALSE;
      }

      switch ($name) {
        case 'total_bounce_rate':
        case 'total_exit_rate':
        case 'total_bounce_rate_custom':
        case 'total_exit_rate_custom':
          // bancho_gaapi_bonus設定
          $default_array = array();
          for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
            $default_array[$i] = array(
              'decrease_if_old_enough' => 0,
              'decrease_every' => 86400,
              'decrease_rate' => 100,
            );
          }
          $options = variable_get('bancho_gaapi_bonus', $default_array);

          // クエリの準備
          switch ($name) {
            case 'total_bounce_rate':
            case 'total_bounce_rate_custom':
              $field = 'total_bounce_rate';
              break;
            case 'total_exit_rate':
            case 'total_exit_rate_custom':
              $field = 'total_exit_rate';
              break;
          }
          switch ($name) {
            case 'total_bounce_rate':
            case 'total_exit_rate':
              $sql = 'SELECT SQL_NO_CACHE ga.nid, ga.total_pv, ga.' . $field . ' FROM {bancho_gaapi} ga';
              break;
            case 'total_bounce_rate_custom':
            case 'total_exit_rate_custom':
              $sql = 'SELECT SQL_NO_CACHE ga.nid, ga.total_pv, ga.' . $field . ', n.created FROM {bancho_gaapi} ga LEFT JOIN {node} n ON ga.nid = n.nid';
              break;
          }

          // クエリ実行
          $db_res = db_query($sql);
          // pv数をどう操作すると良い値になるか（標本数操作）
          // pv数は絶対数が多いので、信頼性の調整だけだと微妙な感じになる
          $n_divider = 2500 / 375;
          foreach ($db_res as $row) {
            // スコア計算
            $score = bancho_gaapi_bonus_calcScore($row->$field / 10000, $row->total_pv / $n_divider);
            switch ($name) {
              case 'total_bounce_rate':
              case 'total_exit_rate':
                // do nothing
                break;
              case 'total_exit_rate_custom':
              case 'total_bounce_rate_custom':
                // 投稿日時からdecrease_if_old_enoughより多く経過していたら
                // 投稿日時からdecrease_every秒経過するごとに
                // PV数評価が1*(decrease_rate/100)になる
                $time_passed = (REQUEST_TIME - $row->created);
                $time_passed = $time_passed > 0 ? $time_passed : 0;
                if ($time_passed > $options[$option]['decrease_if_old_enough']) {
                  $multiplier_index = floor($time_passed / $options[$option]['decrease_every']);
                  if ($multiplier_index > 1) {
                    $diminish_multiplier = pow(
                $options[$option]['decrease_rate'] / 100, 
                $multiplier_index
              );
                  }
                  else {
                    $diminish_multiplier = 1;
                  }
                  $score = ceil($score * $diminish_multiplier);
                }
                break;
            }
            db_update('bancho_gaapi')
              ->fields(array(
                sprintf('score%d',$option) => $score,
              ))
              ->condition('nid', $row->nid)
              ->execute();
          }
          watchdog('bancho', sprintf('(gaapi) ranking score%d updated - %s', $option, check_plain($name)));
          return TRUE;
          break;
      }
  }
}


/**
 * 参考：
 *  http://d.hatena.ne.jp/makiyamakoji/20090512/p1
 *    http://www.evanmiller.org/how-not-to-sort-by-average-rating.html
 *    http://blog.s21g.com/articles/1467
 * @param $rate exit_rateやbounce_rateのパーセンテージ（100%なら1、50%なら0.5）
 * @param $n pvやvisitなどの母数
 * @param $revert $rateが低いほうが良い場合はTRUE、低いほうが良い場合はFALSE
 * @param $norma 信頼性
 * @return スコア（高いほうが良い値に修正済み）
 */
function bancho_gaapi_bonus_calcScore($rate = FALSE, $n = FALSE, $norma = 0.95, $revert = TRUE) {

  static $z_list;
  if (!isset($z_list[$norma])) {
    $z_list[$norma] = (double) bancho_gaapi_calcNormalDistPercentile($norma);
  }

  if (!$rate || !$n) {
    return 0;
  }

  $p = (double) ($revert ? (1 - $rate) : $rate);
  $z = (double) $z_list[$norma];
  $z2 = (double) pow($z, 2);

  $temp = (double) ($p * (1 - $p) + $z2 / (4 * $n));
  $numerator = (double) ($p + ($z2 / (2 * $n)) - $z * sqrt($temp / $n));
  $denominator = (double) (1 + $z2 / $n);

  return ceil(($numerator / $denominator) * 1000000);
}
/**
 * 参考：
 *  http://d.hatena.ne.jp/makiyamakoji/20090512/p1
 */
function bancho_gaapi_calcNormalDistPercentile($percent) {
  if ($percent <= 0 || 1 <= $percent) {
    $percent = 0.95;
  }
  if (percent == 0.5) {
    return 0;
  }
  $b = array(
    1.570796288,
    0.03706987906,
    -0.8364353589e-3,
    -0.2250947176e-3,
    0.6841218299e-5,
    0.5824238515e-5,
    -0.104527497e-5,
    0.8360937017e-7,
    -0.3231081277e-8,
    0.3657763036e-10,
    0.6936233982e-12,
  );

  $w3 = (double) -(log(4 * $percent * (1 - $percent)));
  $w1 = $b[0];
  for ($i = 1; $i < count($b); $i++) {
    $w1 += $b[i] * pow($w3, $i);
  }
  if ($percent > 0.5) {
    return sqrt($w1 * $w3);
  }
  else {
    return -sqrt($w1 * $w3);
  }
}


/**
 * Implements hook_form_alter
().
 */
function bancho_gaapi_bonus_form_alter_main(&$form, &$form_state, $form_id) {
  $default_array = array(
    'decrease_if_old_enough' => 1209600,
    'decrease_every' => 1209600,
    'decrease_rate' => 40,
  );
  $options = variable_get('bancho_gaapi_bonus', array(1 => $default_array, 2 => $default_array, 3 => $default_array));

  $ranking_settings = variable_get('bancho_gaapi_ranking_settings', array());
  $decrease_if_old_enough_options = drupal_map_assoc(array(86400, 172800, 345600, 604800, 1209600, 2419200, 4838400, 9676800, 31536000, 63072000), 'format_interval');
  $decrease_every_options = drupal_map_assoc(array(86400, 172800, 345600, 604800, 1209600, 2419200, 4838400, 9676800, 31536000), 'format_interval');
  $decrease_rate_options = drupal_map_assoc(bancho_gaapi_map_assoc_createarray(), 'bancho_gaapi_map_assoc_percentage');
  for ($i = 1; $i < 4; $i++) {
    $method = $ranking_settings[$i]['method'];
    switch ($method) {
      case 'bancho_gaapi_bonus|total_bounce_rate_custom':
      case 'bancho_gaapi_bonus|total_exit_rate_custom':
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
      default:
    }
  }
  $form['#submit'][] = 'bancho_gaapi_bonus_form_alter_submit';
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_bonus_form_alter_submit_main($form, &$form_state) {
  $options = array(
    1 => array(),
    2 => array(),
    3 => array(),
  );
  for ($i = 1; $i < 4; $i++) {
    if (preg_match('#^bancho_gaapi_bonus\|#', $form_state['values']['ranking'][$i]['method'])) {
      $options[$i]['decrease_if_old_enough'] = $form_state['values']['ranking'][$i]['decrease_if_old_enough'];
      $options[$i]['decrease_every'] = $form_state['values']['ranking'][$i]['decrease_every'];
      $options[$i]['decrease_rate'] = $form_state['values']['ranking'][$i]['decrease_rate'];
    }
  }
  variable_set('bancho_gaapi_bonus', $options);
}
