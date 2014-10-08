<?php
/**
 * Filter by node type
 */
class views_handler_filter_sbm_cache_service extends views_handler_filter_in_operator {
  function get_value_options() {
    if (!isset($this->value_options)) {
      $options = array();
      $this->value_title = 'SBM Service';
      $db_res = db_query('SELECT DISTINCT(service) FROM {bancho_sbm_count}');
      foreach ($db_res as $row) {
        $options[$row->service] = $row->service;
      }
      asort($options);
      $this->value_options = $options;
    }
  }
}
