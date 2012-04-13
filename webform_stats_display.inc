<?php

function webform_stats_render_query($type_id,$node = FALSE,$date_range = FALSE, $aggregate = FALSE){
	$metrics = webform_stats_get_type_metrics($type_id);
	foreach($metrics as $i => $metric){
		$j = $i+1;
		$aggregate_function = 'SUM';
		if($aggregate){
			$fields[] = $aggregate_function."(m$j.data) ".$metric->name;
		}
		else{
			$fields[] = "m$j.data ".$metric->name;
		}	
		$joins[] = " INNER JOIN {webform_stats_report_data} m$j ON (wsr.report_id = m$j.report_id AND m$j.metric_id = ".$metric->metric_id.")";
	}
	if($node){
		if(is_array($node)){
			foreach($node as $i => $nid){
			    $node_criteria .= " AND wsr.nid=%d";
				$vars[] = $nid;
			}
		}
		elseif(is_numeric($node)){
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
	$vars[] = $type_id;
	if($date_range){
		if(is_array($date_range)){
			$date_criteria = 
				" AND wsr.report_period_start >= %d".
				"  AND wsr.report_period_end <= %d";
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
		$group_criteria = " GROUP BY wsr.$aggregate";
		$vars[] = $aggregate;
	}
	else{
		$group_criteria = "";	
	}

	$sql = 
		"SELECT wsr.nid, wsr.report_period_start, wsr.report_period_end, " .
		implode(', ',$fields) .
		" FROM {webform_stats_reports wsr} " .
		implode(" ",$joins) .
		" WHERE wsr.type_id = %d" .
		$node_criteria . 
		$date_criteria .
		$group_criteria;
}