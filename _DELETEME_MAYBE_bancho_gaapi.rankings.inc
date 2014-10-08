<?php
/**
 * @file
 * ランキング作成用のスクリプト
 */

module_load_include('inc', 'gaapi', 'gaapi_core');

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_ranking_main() {
  $args = array();
  for ($i = 0; $args[] = arg($i); $i++) {
  }
  unset($args[$i]);
  unset($args[0]);
  // TODO Please change this theme call as discussed at http://drupal.org/node/224333#theme_page.
  // echo theme('page', bancho_gaapi_ranking_core(implode('/', $args)));
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function theme_bancho_gaapi_ranking_block($path = FALSE) {
  // TODO: Should this theme bancho_gaapi_ranking_block be declared in hook_theme()?
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function theme_bancho_gaapi_ranking_block_content($content) {
  // TODO: Should this theme bancho_gaapi_ranking_block_content be declared in hook_theme()?
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function bancho_gaapi_ranking_core($path) { }

