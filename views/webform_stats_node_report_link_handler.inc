<?php

/**
 * @file
 * Views handler to display a link to the email submission lookup form
 */

/**
 * Field handler to present a link to the email submission lookup form
 */
class webform_stats_node_report_link_handler extends views_handler_field_node_link {
  function construct() {
    parent::construct();
  }

  function render($values) {
    // ensure user has access to edit this node.
    $node = new stdClass();
    $node->nid = $values->{$this->aliases['nid']};
		//EDIT FOR proper permissions
    if (!user_access('view webform stats reports')) {
      return;
    }

		$path = webform_stats_get_node_path($node->nid);
		if(!$path){
			return t('--');
		}
    $text = !empty($this->options['text']) ? $this->options['text'] : t('Report');
    return l($text, $path);
  }
}
