<?php

function webform_stats_display_index(){
	//Get all statistical groupings
	$types = webform_stats_get_types();
	$output = "yo";
	return $output;
}

function webform_stats_display_type($type_id){
	//$date_range = array('min' => time()-2592000, 'max' => time());
	
	if(isset($_GET['grouping']) && isset($_GET['start_date']) && isset($_GET['end_date']) && isset($_GET['d']) ){
		$params['grouping'] = $_GET['grouping'];
		$params['start_date'] = $_GET['start_date'];
		$params['end_date'] = $_GET['end_date'];
		$params['d'] = $_GET['d'];
	}
	else{
		$params['grouping'] = 'nid';
		$params['start_date'] = time()-9676800;
		$params['end_date'] = time();
		$params['d'] = '4m';
	}
	
	$date_range = array(
		'min' => $params['start_date'],
		'max' => $params['end_date'],
	);
	
	$output = drupal_get_form('webform_stats_date_form',$type_id,FALSE,$params);
	$output_data =  webform_stats_report_overview($type_id,FALSE,$date_range,$params['grouping']);
	
	if(!empty($output_data['extra'])){
		foreach($output_data['extra'] as $i => $extra){
			$output .= $extra;
		}
	}
	
	$output .= $output_data['default'];
	return $output;
}

function webform_stats_display_node_overview($type_id,$nid){
	// if(isset($_GET['grouping']) && isset($_GET['start_date']) && isset($_GET['end_date']) && isset($_GET['d']) ){
	// 	$params['grouping'] = $_GET['grouping'];
	// 	$params['start_date'] = $_GET['start_date'];
	// 	$params['end_date'] = $_GET['end_date'];
	// 	$params['d'] = $_GET['d'];
	// }
	// else{
	// 	$params['grouping'] = 'nid';
	// 	$params['start_date'] = time()-9676800;
	// 	$params['end_date'] = time();
	// 	$params['d'] = '4m';
	// }
	// 
	// $date_range = array(
	// 	'min' => $params['start_date'],
	// 	'max' => $params['end_date'],
	// );
	
	//$output = drupal_get_form('webform_stats_date_form',$type_id,$nid,$params);
	
	$output_data = webform_stats_report_overview($type_id,$nid,FALSE,FALSE);
		
		if(!empty($output_data['extra'])){
			foreach($output_data['extra'] as $i => $extra){
				$output .= $extra;
			}
		}
		
		$output .= $output_data['default'];
	return $output;
}

function webform_stats_report_overview($type_id,$nid = FALSE, $date_range = FALSE, $aggregate = FALSE){
	if($aggregate == 'nid'){
		$nodes = webform_stats_get_type_webforms($type_id);
		$row_count = count($nodes);
	}
	else{
		$row_count = FALSE;
	}
	$metrics = webform_stats_get_type_metrics($type_id);

	$header = array();
	
	if(!$nid && ($aggregate !== "report_period_start")){
		$header['nid'] = array(
			'data' => t('Nid'),
			'field' => 'nid',
		);
		$header['title'] =	array(
			'data' => t('Title'),
			'field' => 'title',
		);
	}
		$header['mindate'] = array(
			'data' => t('Start'),
			'field' => 'report_period_start',
		);
		$header['maxdate'] = array(
			'data' => t('End'),
			'field' => 'report_period_end',
		);
	//Add type's metrics to header
	foreach($metrics as $i => $metric){
		$header["m".$metric->metric_id."_data"] = array(
			'data' => t($metric->name),
			'field' => "m".$metric->metric_id."_data",
		);
	}
	
	//Add default sorting params based on node specificity and aggregates
	if(!$nid && $aggregate !== "report_period_start"){
		$header["m".$metrics[0]->metric_id."_data"]['sort'] = 'desc';
	}
	else{
		$header['mindate']['sort'] = "asc";
	}
						
	$query = webform_stats_render_query_builder($type_id,$nid,$date_range, $aggregate);
	$sql = $query['query'];
	$sql .= tablesort_sql($header);
	$result = pager_query($sql, 30, 0,$row_count,$query['vars']);	
	
	$rows = array();
	
	while ($node = db_fetch_object($result)){
		$row_data = array();
		if(!$nid && ($aggregate !== "report_period_start")){
			$row_data[] = $node->nid;
			$row_data[] = l($node->title,"webform_stats/$type_id/$node->nid");
		}
			$row_data[] = date('M j, Y',$node->report_period_start);
			$row_data[] = date('M j, Y',$node->report_period_end);
		foreach($metrics as $i => $metric){
			$data_field = "m".$metric->metric_id."_data";
			$row_data[] = number_format($node->$data_field);
		}
		$rows[] = $row_data;
		$query_data[] = $node;
	}
	$output = theme('table', $header, $rows);
	$output .= theme('pager', NULL, 30, 0);
	
	//Allow other modules to utilize this data

	$additional_output = module_invoke_all('webform_stats_display_data',$query_data,$type_id,$metrics, $date_range, $aggregate);

	
	return array(
		"default" => $output,
		"extra" => $additional_output,
	);
}

function webform_stats_render_query_builder($type_id,$node = FALSE,$date_range = FALSE, $aggregate = FALSE){
	$vars[] = $type_id;
	$metrics = webform_stats_get_type_metrics($type_id);
	$joins[] = "INNER JOIN {node} node ON (wsr.nid = node.nid) ";
	//Loop through the stat grouping type's metrics
	foreach($metrics as $i => $metric){
		//TODO: Add admin controls to display / hide certain fields from view. Exclude hidden ones from this query
		$aggregate_function = 'SUM';
		if($aggregate){
			//Add metric to query column fields with aggregate function
			$fields[] = $aggregate_function."(m".$metric->metric_id.".data) m".$metric->metric_id."_data";
		}
		else{
			//Add metric to query column fields
			$fields[] = "m".$metric->metric_id.".data m".$metric->metric_id."_data";
		}	
		//Add table join for particular report's data for metric
		$joins[] = 
			" INNER JOIN {webform_stats_report_data} m".$metric->metric_id." ON " .
			"(wsr.report_id = m".$metric->metric_id.".report_id AND m".$metric->metric_id.".metric_id = " .
			$metric->metric_id.")";
	}
	if($node){
		//Limit the query to a list of nodes
		if(is_array($node)){
			foreach($node as $i => $nid){
				if($i === 0){
			  	$node_criteria .= " AND (wsr.nid=%d\n";
			  }
			  else{
			  	$node_criteria .= " OR wsr.nid=%d";
		    }
				$vars[] = $nid;
			}
			$node_criteria .= ")";
		}
		elseif(is_numeric($node)){
			//Limit query to a single node only
			$node_criteria = " AND wsr.nid=%d";
			$vars[] = $node;
		}
		else{
			$node_criteria = "";
		}
	}
	else{
		$node_criteria = "";
	}

	if($date_range){
		//Limit the query to a particular date range
		if(is_array($date_range)){
			$date_criteria = 
				" AND wsr.report_period_start >= %d".
				" AND wsr.report_period_end <= %d";
			$vars[] = $date_range['min'];
			$vars[] = $date_range['max'];
		}
		else{
			$date_criteria = "";
		}
	}
	else{
		$date_criteria = "";
	}
	if($aggregate){
		//Group the returned results by a particular field from the report table (wsr)
		$group_criteria = " GROUP BY wsr.$aggregate";
	}
	else{
		$group_criteria = "";	
	}
	//Add COUNT and MAX/MIN Aggregate function to the report_period fields if aggregation is active
	$wsr_fields = ($aggregate) ? "node.nid, node.title as title, COUNT(wsr.report_period_start) reports, MIN(wsr.report_period_start) report_period_start, MAX(wsr.report_period_end) report_period_end, " : "node.nid, node.title as title, wsr.report_period_start report_period_start, wsr.report_period_end report_period_end, ";

	$sql = 
		"SELECT wsr.nid, " .
		$wsr_fields .
		implode(', ',$fields) .
		" FROM {webform_stats_reports wsr} " .
		implode(" ",$joins) .
		" WHERE wsr.type_id = %d" .
		$node_criteria . 
		$date_criteria .
		$group_criteria;
	
	//Return array of sql and vars for use in other functions.	
	return array(
		"query" => $sql,
		"vars" => $vars,
	);
}

function webform_stats_date_form(&$form,$type_id,$nid = FALSE,$params){
	$form = array();

 	$startdate = date('Y-m-d',$params['start_date']);
	$enddate = date('Y-m-d',$params['end_date']);

	//Get type
	$type = webform_stats_get_type_details($type_id);

	//Preset time ranges
	$time_periods = array(
		"1w" => "Last Reporting Period",
		"4w" => "Last Month",
		"4m" => "Last 16 Weeks",
		"ytd" => "Year-To-Date",
	);
	
	//Get all years of data
	//
	$min_year = date('Y',$type->base_date);
	$max_year = date('Y',time());
	
	//Add earlier years to form
	for($i = $max_year-1; $i >= $min_year; $i--){
		$time_periods[$i] = $i;
	}
	$time_periods["all"] = "Everything";
	$time_periods["custom"] = "Custom (Use calendar below)";

  $format = 'Y-m-d';

	$form['report_options'] = array(
		'#type' => 'fieldset',
		'#title' => 'Adjust the data display',
		'#collapsible' => TRUE,
		'#collapsed' => TRUE,
	);
	
	$form['report_options']['start_radio'] = array(
     '#type' => 'select',
     '#title' => 'Start Date',
		 '#options' => $time_periods,
     '#default_value' => $params['d'], 
  );
	if($params['d'] == "c"){
		$form['report_options']['start_radio']["#default_value"] = "custom";
	}
	
	$form['report_options']['custom_range'] = array(
		'#type' => 'fieldset',
		'#title' => 'Custom Date Range',
		'#collapsible' => TRUE,
		'#collapsed' => TRUE,
	);
	if($params['d'] == "c"){
		$form['report_options']['custom_range']["#collapsed"] = FALSE;
	}
	
  $form['report_options']['custom_range']['start'] = array(
     '#type' => 'date_popup',
     '#title' => 'Start Date',
     '#default_value' => $startdate, 
     '#date_format' => $format,
     '#date_label_position' => 'within'
  );

	$form['report_options']['custom_range']['end'] = array(
	    '#type' => 'date_popup',
     '#title' => 'End Date',
     '#default_value' => $enddate, 
     '#date_format' => $format,
     '#date_label_position' => 'within'
  );

	$groupings = array(
		"nid" => "By Node",
		"report_period_start" => "By Reporting Period",
	);

	$form['report_options']['grouping'] = array(
		'#type' => "radios",
		'#title' => "Aggregate data",
		'#options' => $groupings,
		'#default_value' => $params['grouping'],
	);
	
	$form['report_options']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Adjust data display'),
	);
	$form['type_id'] = array(
		'#type' => 'hidden',
		'#default_value' => $type_id,
	);
	$form['path'] = array(
		'#type' => 'hidden',
		'#default_value' => "webform_stats/$type_id",
	);
	if($nid){
		$form['path']['#default_value'] = "webform_stats/$type_id/$nid";
	}
  return $form;
}
function webform_stats_date_form_submit($form, &$form_state){
	
	$type = webform_stats_get_type_details(intval($form_state['values']['type_id']));

	if($form_state['values']['start_radio'] !== "custom"){
		$d = $form_state['values']['start_radio'];
		$the_time = time();
		$latest_report = webform_stats_base_date_factor($the_time,$type->base_date,$type->report_interval);
	switch($form_state['values']['start_radio']){
		case "1w":
			$start_time = $the_time-$type->report_interval;
			$start_date = webform_stats_base_date_factor($start_time,$type->base_date,$type->report_interval);
			$end_date = $latest_report;
			break;
		case "4w":
			$start_time = $the_time-2419200;
			$start_date = webform_stats_base_date_factor($start_time,$type->base_date,$type->report_interval);
			$end_date = $latest_report;
			break;
		case "4m":
			$start_time = $the_time-9676800;
			$start_date = webform_stats_base_date_factor($start_time,$type->base_date,$type->report_interval);
			$end_date = $latest_report;
			break;
		case "ytd":
			$start_date = strtotime(date('Y',time()).'-01-01');
			$end_date = $latest_report;
			break;
		case "all":
			$start_date = $type->base_date;
			$end_date = $latest_report;
			break;
		default:
			$start_date = strtotime($form_state['values']['start_radio']."-01-01");
			$year_after = intval($form_state['values']['start_radio'])+1;
			$end_date = strtotime($year_after."-01-01")-1;
			break;
	}
}
	else{
		$start_date = strtotime($form_state['values']['start']);
		$end_date = strtotime($form_state['values']['end']);
		$d = 'c';
	}
	$query = array(
		'grouping' => $form_state['values']['grouping'],
		'start_date' => $start_date,
		'end_date' => $end_date,
		'd' => $d,
	);

	$form_state['redirect'] = array($form_state['values']['path'],$query);
}