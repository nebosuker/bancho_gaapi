<?php
/**
 * @file
 * Contains the base row style plugin.
 */

/**
 * The basic 'fields' row plugin
 *
 * This displays fields one after another, giving options for inline
 * or not.
 *
 * sites/all/modules/views/includes/plugins.inc
 * と
 * sites/all/modules/views/plugins/views_plugin_row_fields.inc
 * を参考にした
 *
 * 何もしてない定義だけ
 *
 * @ingroup views_row_plugins
 */
class views_plugin_row_bancho_gaapi_ranking extends views_plugin_row_fields {
  function option_definition() {
    return parent::option_definition();
  }

  /**
   * Provide a form for setting options.
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

