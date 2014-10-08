<?php

/**
 * bancho_gaapi_gapath2drpath()で処理するときに
 * カスタムの前処理を入れる
 * URL構造を変えたときとか用
 */
function hook_bancho_gaapi_preprocess_gapath(&$gapath) {

}



/**
 * bancho_gaapi_is_this_node_url()で処理するときに
 * 前もってDBから取得しておくurlエイリアスにカスタムの前処理を入れる
 * URL構造を変えたときとか用
 */
function hook_bancho_gaapi_preprocess_url_aliases(&$url_aliases) {
}


/**
 * bancho_gaapi_is_this_node_url()が失敗したときに
 * URL変更パターンとかを反映する用
 * preprocess_url_aliasesで対応するにはパターン数が多くなるときに使う
 */
function hook_bancho_gaapi_is_this_node_url_fail(&$path) {
}

