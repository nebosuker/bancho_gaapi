<?php

/**
 * Field handler to translate a language into its readable form.
 */
class views_handler_field_bancho_gaapi_rates extends views_handler_field {

  function render($values) {
    $value = $values->{$this->field_alias};
    if ($value == 10000) {
      $s = '100';
    }
    elseif ($value == 0) {
      $s == '0';
    }
    elseif ($value < 1000) {
      $s = sprintf('%01d.%02d', floor($value / 100), $value % 100);
    }
    else {
      $s = sprintf('%02d.%02d', floor($value / 100), $value % 100);
    }
    return check_plain($s . '%');
  }
}
