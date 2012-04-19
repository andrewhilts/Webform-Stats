<?php

function webform_stats_display_index(){
	//Get all statistical groupings
	$types = webform_stats_get_types();
	$output = "yo";
	return $output;
}

function _webform_stats_display_params_build($default_aggregate){
	$display_args = array(
		'start_date' => time()-9676800,
		'end_date' => time(),
		'date_range' => '4m',
		'limit' => 10,
		'grouping' => $default_aggregate,
	);
	foreach($display_args as $key => $value){
		_webform_stats_display_params_assignment($display_args, $key, $value);
	}
	return $display_args;
}

function _webform_stats_display_params_assignment(&$display_args, $key, $default){
	$display_args[$key] = isset($_GET[$key]) ? $_GET[$key] : $default;
}

function webform_stats_display_type($type_id){
	//$date_range = array('min' => time()-2592000, 'max' => time());
	
	$display_args = _webform_stats_display_params_build('nid');
	
	$output = drupal_get_form('webform_stats_date_form',$type_id,FALSE,$display_args);
	
	$type = webform_stats_get_type_details($type_id);
	
	$output .= theme('webform_stats_header',$type->name,'h2');
	$output .= theme('webform_stats_header','From '.date('m/j/y',$display_args['start_date']).' to '.date('m/j/y',$display_args['end_date']),'h3');
	
	$output_data =  webform_stats_report_overview($type_id,FALSE,$display_args);
	
	if(!empty($output_data['extra'])){
		foreach($output_data['extra'] as $i => $extra){
			$output .= $extra;
		}
	}
	
	$output .= $output_data['default'];
	return $output;
}

function webform_stats_display_node_overview($type_id,$nid){
	
	$display_args = _webform_stats_display_params_build('report_period_start');
	
	$node = node_load($nid);
	
	$output .= drupal_get_form('webform_stats_date_form',$type_id,$nid,$display_args);
	
	$output .= theme('webform_stats_header',$node->title,'h2');
	$output .= theme('webform_stats_header','From '.date('m/j/y',$display_args['start_date']).' to '.date('m/j/y',$display_args['end_date']),'h3');

	$display_args['grouping'] = "report_period_start";
	$display_args['limit'] = 10;
	$output_data = webform_stats_report_overview($type_id,$nid,$display_args);
		
		if(!empty($output_data['extra'])){
			foreach($output_data['extra'] as $i => $extra){
				$output .= $extra;
			}
		}
		
		$output .= $output_data['default'];
	return $output;
}

function webform_stats_report_overview($type_id,$nid = FALSE, $display_args){
	$aggregate = $display_args['grouping'];

	$date_range = array(
		'min' => $display_args['start_date'],
		'max' => $display_args['end_date'],
	);

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
	$count_query = webform_stats_render_query_builder($type_id,$nid,$date_range, $aggregate,"count");
	$result = db_query($count_query['query'],$count_query['vars']);
	$sql = $query['query'];

	$sql .= tablesort_sql($header);
  $result = pager_query($sql, $display_args['limit'],0,$count_query['query'],$query['vars']);
	
	$rows = array();
	
	//Build a summary row to place at the end of the rows array
	$summary_row = array();
	
	//Do not add node details to summary when aggregating by time period
	if(!$nid && ($aggregate !== "report_period_start")){
		$summary_row['nid'] = "";
		$summary_row['title'] = "TOTAL:";
	}
	
	//Do not summarize the date columns
	$summary_row['startdate'] = "";
	$summary_row['enddate'] = "";
	
	while ($node = db_fetch_object($result)){
		$row_data = array();
		
		//Do not add node details when aggregating by time period
		if(!$nid && ($aggregate !== "report_period_start")){
			$row_data[] = $node->nid;
			$row_data[] = l($node->title,"webform_stats/$type_id/$node->nid");
		}
			$row_data[] = date('M j, Y',$node->report_period_start);
			$row_data[] = date('M j, Y',$node->report_period_end);
		foreach($metrics as $i => $metric){
			$data_field = "m".$metric->metric_id."_data";
			$row_data[] = number_format($node->$data_field);
			
			//Add an array item to the summary row for each metric
			$summary_row[$metric->metric_id] = $node->$data_field+$summary_row[$metric->metric_id];
			
		}
		$rows[] = $row_data;
		$query_data[] = $node;
	}
	foreach($summary_row as $i => $field){
		if(is_numeric($field)){
			//Format the summary rows for legibility
			$summary_row[$i] = number_format($field);
		}
	}
	//Add summary row, and "total" class (put in theme function)
	$rows[] = theme('webform_stats_table_summary_row',$summary_row);
	
	$themed_pager = theme('pager', NULL, $limit);
	
	$output = $themed_pager;
	$output .= theme('table', $header, $rows);
	$output .= $themed_pager;
	
	
	//Allow other modules to utilize this data
	$additional_output = module_invoke_all('webform_stats_display_data',$query_data,$type_id,$metrics, $date_range, $aggregate);

	return array(
		"default" => $output,
		"extra" => $additional_output,
	);
}

function webform_stats_render_query_builder($type_id,$node = FALSE,$date_range = FALSE, $aggregate = FALSE, $counter = FALSE){
	$vars[] = $type_id;
	$metrics = webform_stats_get_type_metrics($type_id);
	$joins[] = "INNER JOIN {node} as node ON (wsr.nid = node.nid)";
	//Loop through the stat grouping type's metrics
	foreach($metrics as $i => $metric){
		//TODO: Add admin controls to display / hide certain fields from view. Exclude hidden ones from this query
		$aggregate_function = 'SUM';
		if($aggregate){
			//Add metric to query column fields with aggregate function
			$fields[] = $aggregate_function."(m".$metric->metric_id.".data) as m".$metric->metric_id."_data";
		}
		else{
			//Add metric to query column fields
			$fields[] = "m".$metric->metric_id.".data as m".$metric->metric_id."_data";
		}	
		//Add table join for particular report's data for metric
		$joins[] = 
			"LEFT OUTER JOIN {webform_stats_report_data} as m".$metric->metric_id." ON " .
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
	$wsr_fields = ($aggregate == 'nid') ? "node.title as title, COUNT(wsr.report_period_start) as reports, MIN(wsr.report_period_start) as report_period_start, MAX(wsr.report_period_end) as report_period_end, " : "node.title as title, wsr.report_period_start as report_period_start, wsr.report_period_end as report_period_end, ";

	$sql = 
		"SELECT wsr.nid, " .
		$wsr_fields .
		implode(', ',$fields) .
		" FROM {webform_stats_reports} wsr " .
		implode(" ",$joins) .
		" WHERE wsr.type_id = %d" .
		$node_criteria . 
		$date_criteria .
		$group_criteria;
		
	if($counter){
		$sql = 
		"SELECT COUNT(DISTINCT wsr.$aggregate)" .
		" FROM {webform_stats_reports} wsr " .
		" WHERE wsr.type_id = %d" .
		$node_criteria . 
		$date_criteria;
	}

	//Return array of sql and vars for use in other functions.	
	return array(
		"query" => $sql,
		"vars" => $vars,
	);
}

function webform_stats_date_form(&$form,$type_id,$nid = FALSE,$display_args){
	$form = array();

 	$startdate = date('Y-m-d',$display_args['start_date']);
	$enddate = date('Y-m-d',$display_args['end_date']);

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

  $format = 'Y-m-d';

	$form['report_options'] = array(
		'#type' => 'fieldset',
		'#title' => 'Adjust the data display',
		'#collapsible' => TRUE,
		'#collapsed' => TRUE,
	);
	
	$form['report_options']['start_radio'] = array(
     '#type' => 'select',
     '#title' => 'Date Range',
		 '#options' => $time_periods,
     '#default_value' => $display_args['date_range'], 
		 '#description' => t('Ensure that custom date range is not selected.'),
  );
	
	$form['report_options']['custom_range'] = array(
		'#type' => 'fieldset',
		'#title' => 'Custom Date Range',
		'#collapsible' => TRUE,
		'#collapsed' => TRUE,
	);
	if($display_args['date_range'] == 1){
		$form['report_options']['custom_range']["#collapsed"] = FALSE;
	}
	
	$custom_range = ($display_args['date_range'] == 1) ? 1 : 0;
	
	$form['report_options']['custom_range']['custom_range_enabled'] = array(
		'#type' => 'checkbox',
		'#title' => t('Use custom date range'),
		'#return_value' => 1,
		'#default_value' => $custom_range,
	);
	
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
	if(!$nid){
		$form['report_options']['grouping'] = array(
			'#type' => "radios",
			'#title' => "Aggregate data",
			'#options' => $groupings,
			'#default_value' => $display_args['grouping'],
		);
	}
	
	$form['report_options']['limit'] = array(
		'#type' => "select",
		'#title' => "Results to display at a time",
		"#options" => array(5 => 5,10 => 10,15 => 15,20 => 20,30 => 30,50 => 50,100 =>100),
		"#default_value" => $display_args['limit'],
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

	if($form_state['values']['custom_range_enabled'] !== 1){
		$date_range = $form_state['values']['start_radio'];
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
		$date_range = '1';
	}
	$query = array(
		'start_date' => $start_date,
		'end_date' => $end_date,
		'limit' => $form_state['values']['limit'],
		'date_range' => $date_range,
	);
	
	if(isset($form_state['values']['grouping'])){
		$query['grouping'] = $form_state['values']['grouping'];
	}

	$form_state['redirect'] = array($form_state['values']['path'],$query);
}

function theme_webform_stats_table_summary_row($summary_row){
	if(isset($summary_row['title'])){
		$summary_row['title'] = t("TOTAL:");
	}
	else{
		$summary_row['startdate'] = t("TOTAL:");
	}
	return array('data' => $summary_row, 'class' => 'total');
}
function theme_webform_stats_header($text, $level){
	switch($level){
		case "h1":
			$tag = "h1";
			break;
		case "h2":
			$tag = "h2";
			break;
		case "h3":
			$tag = "h3";
			break;
		case "h4":
			$tag = "h4";
			break;
		case "h5":
			$tag = "h5";
			break;
		default:
			$tag = "h2";
			break;
	}
	return "<$tag>".$text."</$tag>\n";
}	