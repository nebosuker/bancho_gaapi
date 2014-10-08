<?php
/**
 * Filter by bancho_gaapi score%
 * 平均値より上かどうかでとりあえずクエリを軽くするフィルタ
 *
 * カテゴリなどで絞り込むと平均以下のも含めなきゃいけなくなるので
 * 係数をテキストボックスなどで指定できるように変えるべきかも
 * →よく考えるとカテゴリで絞り込めば母数が減ってるのでこのフィルタは指定不要
 */
class views_handler_filter_gaapi_above_average extends views_handler_filter_numeric {
  function get_value_options() {
    if (!isset($this->value_options)) {
      $this->value_options = array();
    }
  }

  function option_definition() {
    $options = array();
    return $options;
  }

  function operators() {
    $operators = array(
      '< avg' => array(
        'title' => 'Is less than average',
        'method' => 'op_bancho_gaapi_simple_avg',
        'short' => '<avg',
        'values' => 0,
      ),
      '> avg' => array(
        'title' => 'Is greater than average',
        'method' => 'op_bancho_gaapi_simple_avg',
        'short' => '>avg',
        'values' => 0,
      ),
    );
    return $operators;
  }

  function op_bancho_gaapi_simple_avg($field = NULL) {
//    $this->ensure_my_table();
    // ここもっときれいにフィールド名を受け渡せるといいな
    $ranking_id = preg_replace('/score([0-9]+)_average/', '$1', $this->field);
    $this->query->add_where($this->options['group'], sprintf('bancho_gaapi.score%s', $ranking_id), variable_get("bancho_gaapi_ranking{$ranking_id}_average", 0), ($this->operator == '< avg' ? '<' : '>'));
  }

}
