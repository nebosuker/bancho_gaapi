<?php
/**
 * Implements hook_views_data()().
 * (see Views module)
 * http://views.doc.logrus.com/
 */
function bancho_gaapi_views_data() {
  $ranking_methods = array();
  foreach (module_implements('gaapi_ranking') as $module) {
    $list = module_invoke($module, 'gaapi_ranking', 'list');
    if (count($list) > 0) {
      foreach ($list as $key => $value) {
        $ranking_methods["{$module}|{$key}"] = $value;
      }
    }
  }
  $ranking_settings = array();
  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    $ranking_settings[$i] = array(
      'run_at' => '',
      'method' => '',
    );
  }
  $ranking_settings = variable_get('bancho_gaapi_ranking_settings', $ranking_settings);
  foreach ($ranking_settings as $key => $ranking_array) {
    $method_name = 'なし';
    if (isset($ranking_array['method']) && !empty($ranking_array['method']) && isset($ranking_methods[$ranking_array['method']])) {
      $method_name = $ranking_methods[$ranking_array['method']];
    }
    $ranking_settings[$key]['name'] = $method_name;
  }

  $data = array();

  $data['bancho_gaapi']['table']['group'] = 'Google Analytics data';

//  $data['bancho_gaapi']['table']['base'] = array(
//    'field' => 'nid',
//    'title' => 'Google Analytics data',
//    'help' => 'Google Analytics data',
//    'weight' => 0,
//  );

  $data['bancho_gaapi']['table']['join'] = array(
    'node' => array(
      'left_field' => 'nid',
      'field' => 'nid',
    ),
  );

  $data['bancho_gaapi']['nid'] = array(
    'title' => 'nid',
    'help' => 'nid',
    'relationship' => array(
      'base' => 'node',
      'field' => 'nid',
      'handler' => 'views_handler_relationship',
      'label' => 'nid',
    ),
  );
  $data['bancho_gaapi']['total_pv'] = array(
    'title' => 'GA API: total_pv',
    'help' => 'overall numbers of pageviews',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_last1day_pv'] = array(
    'title' => 'GA API: last1day_pv',
    'help' => 'pageviews of yesterday',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_last1day_pv_delta'] = array(
    'title' => 'GA API: last1day_pv_delta',
    'help' => '(pageviews of yesterday) - (pageviews of the day before yesterday)',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_last7days_pv'] = array(
    'title' => 'GA API: last7days_pv',
    'help' => 'pageviews of last 7 days',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_last7day_pv_delta'] = array(
    'title' => 'GA API: last7day_pv_delta',
    'help' => '(pageviews of last 7 days) - (pageviews of 7 days before last 7 days)',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_last30days_pv'] = array(
    'title' => 'GA API: last30days_pv',
    'help' => 'pageviews of last 30 days',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_last30day_pv_delta'] = array(
    'title' => 'GA API: last30day_pv_delta',
    'help' => '(pageviews of last 30 days) - (pageviews of 30 days before last 30 days)',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_entrances'] = array(
    'title' => 'GA API: total_entrances',
    'help' => 'overall numbers of entrances',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_exits'] = array(
    'title' => 'GA API: total_exits',
    'help' => 'overall numbers of exists',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_exit_rate'] = array(
    'title' => 'GA API: total_exit_rate',
    'help' => 'overall exist rates',
    'field' => array(
      'handler' => 'views_handler_field_bancho_gaapi_rates',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_bounces'] = array(
    'title' => 'GA API: total_bounces',
    'help' => 'overall numbers of bounces',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_bounce_rate'] = array(
    'title' => 'GA API: total_bounce_rate',
    'help' => 'overall bounce rates',
    'field' => array(
      'handler' => 'views_handler_field_bancho_gaapi_rates',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_day1_pv'] = array(
    'title' => 'GA API: day1_pv',
    'help' => 'pageviews of the day node is published',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_1week_pv'] = array(
    'title' => 'GA API: 1week_pv',
    'help' => 'pageviews of 7 days from the day the node is published',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_2week_pv'] = array(
    'title' => 'GA API: 2week_pv',
    'help' => 'pageviews of 14 days from the day the node is published',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_1month_pv'] = array(
    'title' => 'GA API: 1month_pv',
    'help' => 'pageviews of 30 days from the day the node is published',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );
  $data['bancho_gaapi']['total_3month_pv'] = array(
    'title' => 'GA API: 3month_pv',
    'help' => 'pageviews of 90 days from the day the node is published',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );

  for ($i = 1; $i <= BANCHO_GAAPI_NUM_RANKINGS; $i++) {
    if (isset($ranking_settings[$i]['method']) && $ranking_settings[$i]['method']) {
      $data['bancho_gaapi']["score{$i}"] = array(
        'title' => "GA API: ranking{$i} " . check_plain($ranking_settings[$i]['name']),
        'help' => "ranking score $i -&gt; " . check_plain($ranking_settings[$i]['name']),
        'field' => array(
          'handler' => 'views_handler_field_numeric',
          'click sortable' => TRUE,
        ),
        'sort' => array(
          'handler' => 'views_handler_sort',
        ),
        'filter' => array(
          'handler' => 'views_handler_filter_numeric',
        ),
      );
      // ランキング用ブロックなどで全件を対象にクエリするとコストが高いので
      // あらかじめcronでのランキングデータ処理時に平均値を出しておいて
      // bancho_gaapi_ranking{$i}_average変数に保存しておき、
      // その平均値より大きいデータだけを対象とするフィルタを定義
      $data['bancho_gaapi']["score{$i}_average"] = array(
        'title' => "GA API: average filter {$i} " . check_plain($ranking_settings[$i]['name']),
        'help' => "ranking score $i -&gt; " . check_plain($ranking_settings[$i]['name'] . ". For filtering with average value (stored in variable bancho_gaapi_ranking{$i}_average) to reduce querytime."),
        'filter' => array(
          'handler' => 'views_handler_filter_gaapi_above_average',
          'additional fields' => array(
            "score{$i}" => "score{$i}",
          ),
          'options' => array(
            'bancho_gaapi_ranking_id' => $i,
          ),
        ),
      );
    }
  }

  if (db_table_exists('bancho_sbm_count')) {

    $data['bancho_sbm_count']['table']['group'] = 'bancho_sbm_count';
//    $data['bancho_sbm_count']['table']['base'] = array(
//      'field' => 'nid',
//      'title' => 'Social reaction data',
//      'help' => 'Social reaction data',
//      'weight' => 0,
//    );

    $data['bancho_sbm_count']['table']['join'] = array(
      'node' => array(
        'left_field' => 'nid',
        'field' => 'nid',
      ),
    );
    $data['bancho_sbm_count']['nid'] = array(
      'title' => 'nid',
      'help' => 'nid',
      'field' => array(
        'handler' => 'views_handler_field',
        'click sortable' => TRUE,
      ),
      'relationship' => array(
        'base' => 'node',
        'field' => 'nid',
        'handler' => 'views_handler_relationship',
        'label' => 'nid',
      ),
    );
    $data['bancho_sbm_count']['service'] = array(
      'title' => 'SBM: service',
      'help' => 'SBM: service name',
      'field' => array(
        'handler' => 'views_handler_field',
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_sbm_cache_service',
      ),
    );
    $data['bancho_sbm_count']['count'] = array(
      'title' => 'SBM: count',
      'help' => 'SBM: count',
      'field' => array(
        'handler' => 'views_handler_field_numeric',
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_numeric',
        'option' => 'integer',
      ),
    );
  }

  return $data;
}



/**
 * Implements hook_views_handlers().().
 */
function bancho_gaapi_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'bancho_gaapi') . '/views',
    ),
    'handlers' => array(
      'views_handler_field_bancho_gaapi_rates' => array(
        'parent' => 'views_handler_field',
      ),
      'views_handler_filter_gaapi_above_average' => array(
        'parent' => 'views_handler_filter_numeric',
      ),
      'views_handler_filter_sbm_cache_service' => array(
        'parent' => 'views_handler_filter_in_operator',
      ),
    ),
  );
}




/**
 * Implementation of hook_views_style_plugins().
 * Adds view types to views UI interface.
 */

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_views_plugins() {
  return array(
    'module' => 'bancho_gaapi', // This just tells our themes are elsewhere.
    'style' => array(
      'bancho_gaapi_ranking' => array(
        'title' => t('Bancho GAAPI Ranking'),
        'help' => 'bancho_gaapi_ranking style list',
        'parent' => 'list',
        'handler' => 'views_plugin_style_bancho_gaapi_ranking',
        'path' => drupal_get_path('module', 'bancho_gaapi') . '/views',
        'theme' => 'views_view_bancho_gaapi_ranking',
        'theme file' => 'theme.inc',
        'theme path' => drupal_get_path('module', 'bancho_gaapi') . '/views',
        'uses row plugin' => TRUE,
        'uses options' => TRUE,
        'uses grouping' => FALSE,
        'type' => 'normal',
        'help topic' => 'style-list',
      ),
    ),
    'row' => array(
      'bancho_gaapi_ranking_row_fields' => array(
        'title' => t('Bancho GAAPI Ranking Fields'),
        'help' => 'bancho_gaapi_ranking style list',
        'parent' => 'fields',
        'handler' => 'views_plugin_row_bancho_gaapi_ranking',
        'path' => drupal_get_path('module', 'bancho_gaapi') . '/views',
        'theme' => 'views_view_bancho_gaapi_ranking_row_fields',
        'theme file' => 'theme.inc',
        'theme path' => drupal_get_path('module', 'bancho_gaapi') . '/views',
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
        'help topic' => 'style-row-fields',
      ),
    ),
  );
}


