<?php

function webform_stats_display_index(){
	//Get all statistical groupings
	$types = webform_stats_get_types();
	$output = "yo";
	return $output;
}

function webform_stats_display_type($type_id){
	//$date_range = array('min' => time()-2592000, 'max' => time());
	
	if(isset($_GET['grouping']) && isset($_GET['start_date']) && isset($_GET['end_date'])){
		$params['grouping'] = $_GET['grouping'];
		$params['start_date'] = $_GET['start_date'];
		$params['end_date'] = $_GET['end_date'];
	}
	else{
		$params['grouping'] = 'nid';
		$params['start_date'] = time()-2419200;
		$params['end_date'] = time();
	}
	
	$date_range = array(
		'min' => $params['start_date'],
		'max' => $params['end_date'],
	);
	
	$output = drupal_get_form('webform_stats_date_form',$type_id,FALSE,$params);
	$output .= webform_stats_report_overview($type_id,FALSE,$date_range,$params['grouping']);
	return $output;
}

function webform_stats_display_node_overview($type_id,$nid){
	return webform_stats_report_overview($type_id,$nid,FALSE,FALSE);
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
		$header['mindate']['sort'] = "desc";
	}
						
	$query = webform_stats_render_query_builder($type_id,$nid,$date_range, $aggregate);
	$sql = $query['query'];
	$sql .= tablesort_sql($header);
	$result = pager_query($sql, 30, 0,$row_count,$query['vars']);	
	
	$rows = array();

	while ($node = db_fetch_object($result)){
		$row_data = array();
		if(!$nid && ($aggregate !== "report_period_start")){$row_data[] = l($node->title,"webform_stats/$type_id/$node->nid");}
			$row_data[] = date('M j, Y',$node->report_period_start);
			$row_data[] = date('M j, Y',$node->report_period_end);
		foreach($metrics as $i => $metric){
			$data_field = "m".$metric->metric_id."_data";
			$row_data[] = number_format($node->$data_field);
		}
		$rows[] = $row_data;
	}
	$output = theme('table', $header, $rows);
	$output .= theme('pager', NULL, 30, 0);
	return $output;
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
	$wsr_fields = ($aggregate) ? "node.title as title, COUNT(wsr.report_period_start) reports, MIN(wsr.report_period_start) report_period_start, MAX(wsr.report_period_end) report_period_end, " : "node.title as title, wsr.report_period_start report_period_start, wsr.report_period_end report_period_end, ";

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


  $format = 'Y-m-d';

	$form['report_options'] = array(
		'#type' => 'fieldset',
		'#title' => 'Adjust the data display',
		'#collapsible' => TRUE,
		'#collapsed' => TRUE,
	);

  $form['report_options']['start'] = array(
     '#type' => 'date_popup',
     '#title' => 'Start Date',
     '#default_value' => $startdate, 
     '#date_format' => $format,
     '#date_label_position' => 'within'
  );

	$form['report_options']['end'] = array(
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
	
	$query = array(
		'grouping' => $form_state['values']['grouping'],
		'start_date' => strtotime($form_state['values']['start']),
		'end_date' => strtotime($form_state['values']['end']),
	);
		
	$form_state['redirect'] = array($form_state['values']['path'],$query);
}