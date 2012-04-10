<?php

function webform_stats_admin(){
	$output = "hello world";
	$output .= drupal_get_form('webform_stats_types_add_form');
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
	dsm(webform_stats_get_type_metrics($type_id));
	$output = drupal_get_form('webform_stats_types_edit_metrics_form',$type_id);
	return $output;
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
    	'#default_value' => webform_stats_get_type_webforms($type),
		);
		$form['submit'] = array(
	    '#type' => 'submit',
	    '#value' => t('Assign to group'),
	  );
		return $form;
}

function webform_stats_types_edit_nodes_form_submit($form, &$form_state){
	$type_nodes = webform_stats_get_type_webforms($form_state['values']['type']);
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

function webform_stats_types_edit_metrics_form (&$form_state, $type){
	$form = array();
	
	$form['type'] = array('#value' => $type, '#type' => 'hidden');
	
	$component_types = webform_stats_get_component_types();
	
	$form['form_key'] = array(
		'#type' => 'textfield',
  	'#title' => t('Form key to match'),
		'#size' => 30,
		'#required' => TRUE,
	);
	$form['component'] = array(
		'#type' => 'select',
   	'#title' => t('Component to measure'),
		'#options' => $component_types,
		'#default_value' => 0,
		'#required' => TRUE,
	);
	$query_functions = array(
		'SUM','COUNT','AVG','MAX','MIN',
	);
	$form['query_function'] = array (
		'#type' => 'select',
   	'#title' => t('Measurement Function'),
		'#options' => $query_functions,
		'#default_value' => 0,
		'#required' => TRUE,
	);
	$form['form_key_fallback'] = array (
		'#type' => 'radios',
   	'#title' => t('Fallback to first component of type if no matching key?'),
		'#options' => array(t('No'),t('Yes'),),
		'#default_value' => 1,
		'#required' => TRUE,
	);
	$form['submit'] = array(
	  '#type' => 'submit',
		'#value' => t('Add metric'),
	);
	return $form;
}

function webform_stats_types_edit_metrics_form_submit ($form,&$form_state) {
	$metric = new Stdclass;
	$metric->form_key = $form_state['values']['form_key'];
	$metric->component = $form_state['values']['component'];
	$metric->query_function = $form_state['values']['query_function'];
	$metric->form_key_fallback = $form_state['values']['form_key_fallback'];
	$metric->type_id = $form_state['values']['type'];
	drupal_write_record('webform_stats_report_type_metrics',$metric);
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