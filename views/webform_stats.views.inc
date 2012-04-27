<?php
/**
 * @file
 * Views hooks implemented for the Webform Stats module.
 */

function webform_stats_views_data_alter(&$data) {
//Link to report
$data['node']['webform_stats_node_report_link_handler'] = array(
    'field' => array(
      'title' => t('Webform Stats Report'),
      'help' => t('A link to the node report overview.'),
      'handler' => 'webform_stats_node_report_link_handler',
    ),
  );
}

/**
 * Implements hook_views_handlers().
 */
function webform_stats_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'webform_stats') . '/views',
    ),
    'handlers' => array(
      'webform_stats_node_report_link_handler' => array(
        'parent' => 'views_handler_field_node_link',
        'file' => 'webform_stats_node_report_link_handler.inc',
    ),
    ),
  );
}