<?php

function webform_stats_render_query($type_id,$node = FALSE,$date_range = FALSE, $aggregate = FALSE){
	$metrics = webform_stats_get_type_metrics($type_id);
	//Loop through the stat grouping type's metrics
	foreach($metrics as $i => $metric){
		//TODO: Add admin controls to display / hide certain fields from view. Exclude hidden ones from this query
		$j = $i+1;
		$aggregate_function = 'SUM';
		if($aggregate){
			//Add metric to query column fields with aggregate function
			$fields[] = $aggregate_function."(m$j.data) ".$metric->name;
		}
		else{
			//Add metric to query column fields
			$fields[] = "m$j.data ".$metric->name;
		}	
		//Add table join for particular report's data for metric
		$joins[] = 
			" INNER JOIN {webform_stats_report_data} m$j ON " .
			"(wsr.report_id = m$j.report_id AND m$j.metric_id = " .
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
	$vars[] = $type_id;
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
		$vars[] = $aggregate;
	}
	else{
		$group_criteria = "";	
	}
	
	$date_fields = ($aggregate) ? "MIN(wsr.report_period_start), MAX(wsr.report_period_end)," : "wsr.report_period_start, wsr.report_period_end,";

	$sql = 
		"SELECT wsr.nid, " .
		$date_fields .
		implode(', ',$fields) .
		" FROM {webform_stats_reports wsr} " .
		implode(" ",$joins) .
		" WHERE wsr.type_id = %d" .
		$node_criteria . 
		$date_criteria .
		$group_criteria;
}