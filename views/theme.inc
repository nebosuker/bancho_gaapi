<?php

/**
 * @file
 * View theme functions for bancho_gaapi_ranking
 */

/**
 * Template preprocessor for views-view-bancho-gaapi-ranking.tpl.php.
 */
function template_preprocess_views_view_bancho_gaapi_ranking(&$vars) {
  //drupal_set_message('template_preprocess_views_view_bancho_gaapi_ranking');
//  template_preprocess_views_view_list($vars);

  //bancho_debug_to_file(NULL, NULL, NULL, TRUE);
//bancho_debug_to_file($vars, '$vars', '/nfs/contents1/ibc/netshop.impress.co.jp/_OLD/yasuda-debug.txt');

  // .item-list ol.gaapi_rankingにする（tpl.phpで対応が必要）
  $vars['options']['ul_classname'] = 'gaapi_ranking';

  $total_rows = $vars['view']->total_rows;
  $items_per_page = $vars['view']->items_per_page || 999;
  $current_page = $vars['view']->current_page || 0;

  $vars['li_classes'] = array();
  $vars['li_values'] = array();
  foreach ($vars['rows'] as $id => $row) {
    // li.gaapi_ranking_1のクラス名を用意していく（2ページ目以外）
    if (! $current_page) {
      $vars['li_classes'][$id] = sprintf('gaapi_ranking_%d', $id + 1);
    }
    // liのvalue入れていく
    $vars['li_values'][$id] = check_plain($id + 1 + ($items_per_page * (($current_page * $items_per_page) <= $total_rows ? $current_page : floor($total_rows / $items_per_page))));

  }


}

/**
 * Template preprocessor for views-view-bancho-gaapi-ranking-row_fields.tpl.php.
 */
function template_preprocess_views_view_bancho_gaapi_ranking_row_fields(&$vars) {
  // parent指定してるから自動でやるかと思ったら違うっぽい
  template_preprocess_views_view_fields($vars);
  //bancho_debug_to_file(NULL, NULL, NULL, TRUE);
  //bancho_debug_to_file($vars, 'template_preprocess_views_view_bancho_gaapi_ranking_row_fields');
}

