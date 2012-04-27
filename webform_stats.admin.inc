<?php

function webform_stats_admin(){
  $output = "hello world";
  $output .= drupal_get_form('webform_stats_settings_form');
  return $output;
}

function webform_stats_admin_type_add(){
  $output = drupal_get_form('webform_stats_types_add_form');
  return $output;
}

function webform_stats_admin_type($type_id){
  $output = drupal_get_form('webform_stats_types_manage_form',$type_id);
  return $output;
}
function webform_stats_admin_type_nodes($type_id){
  $output = drupal_get_form('webform_stats_types_edit_nodes_form',$type_id);
  return $output;
}
function webform_stats_admin_type_delete($type_id){
  $output = drupal_get_form('webform_stats_types_edit_nodes_form',$type_id);
  return $output;
}
function webform_stats_admin_type_metrics($type_id){
  $output = webform_stats_metric_table(webform_stats_get_type_metrics($type_id));
  $output .= "<h2>Add new Metric</h2>";
  $output .= drupal_get_form('webform_stats_types_metrics_form',$type_id);
  return $output;
}

function webform_stats_settings_form(){

  $form['webform_stats_debug'] = array(
    '#type' => 'checkbox',
     '#title' => t('Debug mode?'),
    '#description' => t('Write detailed logs to watchdog table.'),
    '#return_value' => TRUE,
    '#default_value' => variable_get('webform_stats_debug',FALSE),
    '#required' => TRUE,
  );

  if(module_exists('data')){
    $form['webform_stats_data'] = array(
      '#type' => 'checkbox',
       '#title' => t('Make stats available to Data module?'),
      '#description' => t('This will allow for stats to be used in Views.'),
      '#return_value' => TRUE,
      '#default_value' => variable_get('webform_stats_data',FALSE),
      '#required' => TRUE,
    );
  }

  return system_settings_form($form);
}

function webform_stats_type_form_options(&$form, $type_id = false){
  if($type_id){
    $type = webform_stats_get_type_details($type_id);
  }
  else {
    $type = false;
  }
  $hours = array();
  for ($i=0; $i < 24; $i++) {
    $hours[$i] = $i.":00";
  }
  $intervals = array(
      604800 => t('Weekly'),
      1209600 => t('Biweekly'),
      2419200 => t('Every 4 weeks'),
    );
  $weekdays = webform_stats_weekdays();

  $form['name'] = array(
     '#type' => 'textfield',
     '#title' => t('Statistical Group Name'),
     '#size' => 24,
     '#maxlength' => 255,
    '#required' => TRUE,
   );
  $form['day'] = array(
    '#type' => 'select',
     '#title' => t('What day of the week should report intervals begin on?'),
    '#description' => t('This cannot be altered once submitted.'),
    '#options' => $weekdays,
    '#default_value' => 0,
    '#required' => TRUE,
  );
  $form['interval'] = array(
    '#type' => 'select',
     '#title' => t('What is the length of the report interval?'),
    '#description' => t('This cannot be altered once submitted.'),
    '#options' => $intervals,
    '#default_value' => 604800,
  );
  $form['cron'] = array(
    '#title' => t('Build reports on cron runs (according to schedule)'),
    '#type' => 'radios',
    '#default_value' => 0,
    '#options' => array('yes','no'),
    '#required' => TRUE,
  );
  $form['scheduling'] = array(
    '#type' => 'fieldset',
    '#title' => t('Scheduling options'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['scheduling']['scheduling_day'] = array(
    '#type' => 'select',
    '#title' => t('What day of the week should reports be generated?'),
    '#options' => $weekdays,
    '#default_value' => 0,
  );
  $form['scheduling']['scheduling_hour'] = array(
    '#type' => 'select',
     '#title' => t('What hour of the day should the reports start generating after?'),
    '#options' => $hours,
    '#default_value' => 1,
  );
  $form['scheduling']['scheduling_window'] = array(
    '#type' => 'select',
    '#title' => t('Length of time within which cron runs may generate reports'),
    "#options" => $hours,
    '#default_value' => 2,
  );
  if($type){

    $form['name']['#default_value'] = $type->name;
    $day = date('l', $type->base_date);
    $form['day'] = array(
      '#type' => 'textfield',
       '#title' => t('What day of the week should report intervals begin on?'),
      '#default_value' => $day,
      '#disabled' => TRUE,
      '#size' => 10,
    );
    $form['interval'] = array(
      '#type' => 'textfield',
       '#title' => t('What is the length of the report interval?'),
      '#default_value' => $intervals[$type->report_interval],
      '#disabled' => TRUE,
      '#size' => 10,
    );
    $form['cron']['#default_value'] = $type->cron;
    $form['scheduling']['scheduling_day']['#default_value'] = $type->scheduling_day;
    $form['scheduling']['scheduling_hour']['#default_value'] = $type->scheduling_hour;
    $form['scheduling']['scheduling_window']['#default_value'] = $type->scheduling_window;
  }
}

function webform_stats_types_add_form(&$form_state){
  $form = array();
  webform_stats_type_form_options($form);
  $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Create Statistical Group'),
    );
  return $form;
}

function webform_stats_types_add_form_submit($form, &$form_state) {
  $type = new Stdclass;
  $type->name = $form_state['values']['name'];
  $type->report_interval = $form_state['values']['interval'];
  $type->base_date = webform_stats_build_base_date($form_state['values']['day']);
  if(!$type->base_date){
    return false;
  }
  $type->cron = $form_state['values']['cron'];
  $type->scheduling_day = $form_state['values']['scheduling_day'];
  $type->scheduling_hour = $form_state['values']['scheduling_hour'];
  $type->scheduling_window = $form_state['values']['scheduling_window'];
  drupal_write_record('webform_stats_report_types',$type);
  drupal_set_message("New Statistical Group Created");
  menu_rebuild();
}

function webform_stats_types_manage_form(&$form_state, $type = false){
  $form = array();
  $form['type'] = array('#value' => $type, '#type' => 'hidden');

  webform_stats_type_form_options($form,$type);

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Modify Group Settings'),
  );
  return $form;
}

function webform_stats_types_manage_form_submit($form,&$form_state){
  $vars = array(
    $form_state['values']['name'],
    $form_state['values']['cron'],
    $form_state['values']['scheduling_day'],
    $form_state['values']['scheduling_hour'],
    $form_state['values']['scheduling_window'],
    $form_state['values']['type'],
  );

  $sql = "UPDATE {webform_stats_report_types} SET name = '%s', cron = %d, scheduling_day = %d, scheduling_hour =%d, scheduling_window=%d WHERE type_id = %d LIMIT 1";

  db_query($sql,$vars);
}

function webform_stats_types_edit_nodes_form(&$form_state, $type = false){

    $webforms = webform_stats_get_webforms();
    $form = array();

    $form['type'] = array('#value' => $type, '#type' => 'hidden');

    $form['nodes'] = array(
      '#type' => 'checkboxes',
      '#title' => 'Webforms in Reporting Group',
      '#description' => t('Select the webforms you would like to include in this group.'),
      '#options' => $webforms,
      '#default_value' => webform_stats_get_type_webforms($type,'min'),
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Assign to group'),
    );
    return $form;
}

function webform_stats_types_edit_nodes_form_submit($form, &$form_state){
  $type_nodes = webform_stats_get_type_webforms($form_state['values']['type'],'min');
  foreach($form_state['values']['nodes'] as $i => $node){
    if($node > 0){
      $nodes[$i] = $i;
    }
  }

  $add_nodes = array_diff($nodes,$type_nodes);
  $remove_nodes = array_diff($type_nodes,$nodes);

  //Remove old nodes from type
  $sql = 'DELETE FROM {webform_stats_nodes} WHERE type_id = %d AND (';
  $sql2 = '';
  $variables = array($form_state['values']['type']);
  foreach($remove_nodes as $i => $node){
    if(strlen($sql2))
    {
      $sql2 .= ' OR ';
    }
    $sql2 .= 'nid = %d';
    $variables[] = $i;
  }
  $sql2 .= ')';
  db_query($sql . $sql2, $variables);

  //Add new nodes to type
  $sql = 'INSERT INTO {webform_stats_nodes} (nid,type_id) VALUES ';
  $sql2 = '';
  $variables = array();
  foreach($add_nodes as $i => $node){
    if(strlen($sql2))
    {
      $sql2 .= ', ';
    }
    $sql2 .= '(%d, %d)';
    $variables[] = $i;
    $variables[] = $form_state['values']['type'];
  }
  db_query($sql . $sql2, $variables);
}

function webform_stats_types_metrics_form (&$form_state, $type){
  $form = array();

  $form['type'] = array('#value' => $type, '#type' => 'hidden');

  webform_stats_metrics_form_options($form);

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add metric'),
  );
  return $form;
}

function webform_stats_types_metrics_form_submit ($form,&$form_state) {
  $metric = new Stdclass;
  $metric->name = $form_state['values']['name'];
  $metric->form_key = $form_state['values']['form_key'];
  $metric->component = $form_state['values']['component'];
  $metric->query_function = $form_state['values']['query_function'];
  $metric->form_key_fallback = $form_state['values']['form_key_fallback'];
  $metric->uniques = $form_state['values']['uniques'];
  $metric->type_id = $form_state['values']['type'];
  drupal_write_record('webform_stats_report_type_metrics',$metric);
  //Fire new metric hook
  module_invoke_all('webform_stats_type_metric_add',$metric->type_id);
}

/*
 * Implementation of hook_webform_stats_type_metric_add();
 */
function webform_stats_webform_stats_type_metric_add($type_id){
  //loop through all type nodes and mark as incomplete
  $nids = webform_stats_get_type_webforms($type_id);
  foreach($nids as $i => $nid){
    webform_stats_type_node_mark_complete($nid,$type_id,"incomplete");
  }
}

function webform_stats_metric_edit_form(&$form_state, $metric_id)
{
  $form['metric_id'] = array(
       '#type' => 'value',
       '#value' => $metric_id
   );
  if(isset($_GET['destination'])){
    $form['destination'] = array(
       '#type' => 'value',
       '#value' => $_GET['destination']
     );
  }

  webform_stats_metrics_form_options($form, $metric_id);

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Edit metric'),
  );

  return $form;
}

function webform_stats_metric_edit_form_submit($form,&$form_state){
  $vars = array(
    $form_state['values']['name'],
    $form_state['values']['query_function'],
    $form_state['values']['uniques'],
    $form_state['values']['component'],
    $form_state['values']['form_key'],
    $form_state['values']['form_key_fallback'],
    $form_state['values']['metric_id'],
  );

  $sql = "UPDATE {webform_stats_report_type_metrics} SET name = '%s', query_function = '%s', uniques = %d, component = '%s', form_key = '%s', form_key_fallback = %d WHERE metric_id = %d LIMIT 1";

  db_query($sql,$vars);

  if(isset($form_state['values']['destination'])){
    $form_state['redirect'] = $form_state['values']['destination'];
  }
  else{
    $form_state['redirect'] = "admin/settings/webform_stats";
  }
}

function webform_stats_metrics_form_options(&$form,$metric_id = FALSE){

  $component_types = webform_stats_get_component_types();

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Metric Name'),
    '#size' => 30,
    '#required' => TRUE,
  );
  $query_functions = array(
    'SUM' => 'SUM',
    'COUNT' => 'COUNT',
    'AVG' => 'AVG',
    'MAX' => 'MAX',
    'MIN' => 'MIN',
  );
  $form['query_function'] = array (
    '#type' => 'select',
     '#title' => t('Measurement Function'),
    '#options' => $query_functions,
    '#default_value' => 0,
    '#required' => TRUE,
  );
  $form['uniques'] = array (
    '#type' => 'radios',
     '#title' => t('Unique Values only?'),
    '#options' => array(t('No'),t('Yes'),),
    '#default_value' => 0,
    '#required' => TRUE,
  );
  $form['component'] = array(
    '#type' => 'select',
     '#title' => t('Component to measure'),
    '#options' => $component_types,
    '#default_value' => 0,
    '#required' => TRUE,
  );
  $form['form_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Form key to match'),
    '#size' => 30,
    '#required' => TRUE,
  );
  $form['form_key_fallback'] = array (
    '#type' => 'radios',
     '#title' => t('Fallback to first component of type if no matching key?'),
    '#options' => array(t('No'),t('Yes'),),
    '#default_value' => 1,
    '#required' => TRUE,
  );
  if($metric_id){
    $metric = webform_stats_get_metric($metric_id);
    $form['name']['#default_value'] = $metric->name;
    $form['query_function']['#default_value'] = $metric->query_function;
    $form['uniques']['#default_value'] = $metric->uniques;
    $form['component']['#default_value'] = $metric->component;
    $form['form_key']['#default_value'] = $metric->form_key;
    $form['form_key_fallback']['#default_value'] = $metric->form_key_fallback;
  }
}

/**
 * Build the delete confirmation form.
 *
 * A simple wrapper around Drupal's core confirm_form() function. Adds a value
 * field to store the ID of the record being deleted.
 *
 * @ingroup forms
 * @see ipad_redirect_manager_delete_confirm_submit()
 * @see confirm_form()
 */
function webform_stats_metric_delete_confirm_form(&$form_state, $metric_id)
{
    $form['metric_id'] = array(
        '#type' => 'value',
        '#value' => $metric_id
    );
    if(isset($_GET['destination'])){
      $form['destination'] = array(
        '#type' => 'value',
        '#value' => $_GET['destination']
      );
    }
    $metric = webform_stats_get_metric($metric_id);
    return confirm_form($form, t('Are you sure you want to delete metric:  '.$metric->name.'?', array(
        '%title' => $metric_id
    )), isset($_GET['destination']) ? $_GET['destination'] : 'admin/settings/webform_stats/', t('This action cannot be undone.'), t('Delete'), t('Cancel'));
}

function webform_stats_metric_delete_confirm_form_submit($form, &$form_state)
{
  if ($form_state['values']['confirm']) {
     webform_stats_metric_delete($form_state['values']['metric_id']);
    drupal_set_message(t('Your metric was deleted.'));
  }
  if(isset($form_state['values']['destination'])){
    $form_state['redirect'] = $form_state['values']['destination'];
  }
  else{
    $form_state['redirect'] = "admin/settings/webform_stats";
  }
}

function webform_stats_build_base_date($reporting_day){
  //Get base_date (earliest submission on record)
  $sql = "SELECT MIN(submitted) as min FROM {webform_submissions}";
  $result = db_query($sql);
  $date = db_fetch_object($result);

  if(!$date->min){
    drupal_set_message('No webform submissions yet. Need at least one.');
    return false;
  }

  $base_date = $date->min;

  //Change base date to particular day of week, so reporting interval is consistent
  //Make base_date closest to reporting date/time
  $weekdays = webform_stats_weekdays();
  $base_day_string = $weekdays[$reporting_day];

  $base_date = strtotime('next '.$base_day_string, $base_date)-604800;
  return $base_date;
}

function webform_stats_weekdays(){
  return array(
    t('Sunday'),
    t('Monday'),
    t('Tuesday'),
    t('Wednesday'),
    t('Thursday'),
    t('Friday'),
  );
}

function webform_stats_get_component_types(){
  $sql = "SELECT DISTINCT type from {webform_component}";
  $result = db_query($sql);
  $types = array();

  while($row = db_fetch_object($result)){
    $types[$row->type] = ucwords($row->type);
  }

  return $types;
}

function webform_stats_metric_table($metrics){
  $header = array(
    t('Name'),
    t('Measurement Function'),
    t('Uniques?'),
    t('Component'),
    t('Form Key'),
    t('Form Key Fallback'),
    t('Operations'),
  );
  $rows = array();
  $row_class = "webform_stats_metric";

  foreach($metrics as $i => $metric){
    $row_data = array(
      $metric->name,
      $metric->query_function,
      ($metric->uniques) ? t('Yes') : t('No'),
      $metric->component,
      $metric->form_key,
      ($metric->form_key_fallback) ? t('Yes') : t('No'),
      webform_stats_metric_links($metric->type_id,$metric->metric_id),
    );
    $rows[] = array('data' => $row_data, 'class' => $row_class);
  }
  return theme('table', $header, $rows, array('id' => 'webform_stats_metrics'));
}

function webform_stats_metric_links($type_id,$metric_id)
{
    $path            = drupal_get_path('module', 'webform_stats') . '/images/';
    $links['edit']   = array(
        'title' => theme('image', $path . 'text-editor.png', t('Edit')),
        'href' => "admin/settings/webform_stats/metric_edit/$metric_id",
        'html' => TRUE,
        'query' => drupal_get_destination()
    );
    $links['delete'] = array(
        'title' => theme('image', $path . 'edit-delete.png', t('Delete')),
        'href' => "admin/settings/webform_stats/metric_delete/$metric_id",
        'html' => TRUE,
        'query' => drupal_get_destination()
    );
    return theme('links', $links);
}

function webform_stats_metric_delete($metric_id){
  $sql = "DELETE FROM {webform_stats_report_type_metrics} WHERE metric_id = %d LIMIT 1";
  db_query($sql,$metric_id);
}