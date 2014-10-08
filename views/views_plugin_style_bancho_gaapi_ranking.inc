<?php

/**
 * @file
 * Extends the view_plugin_style class to provide new ranking view style.
 *
 * sites/all/modules/views/includes/plugins.inc
 * と
 * sites/all/modules/views/plugins/views_plugin_style_list.inc
 * を参考にした
 *
 * optionのデフォルトをolにしてるだけ
 *
 */

class views_plugin_style_bancho_gaapi_ranking extends views_plugin_style_list {

  /**
   * Set default options
   */
  function option_definition() {
    $options = parent::option_definition();

    $options['type']['default'] = 'ol';

    return $options;
  }

  /**
   * Render the given style.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
  }

  /**
   * Perform any necessary changes to the form values prior to storage.
   * There is no need for this function to actually store the data.
   */
  function options_submit(&$form, &$form_state) { }
}

