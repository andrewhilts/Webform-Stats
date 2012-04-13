<?php

function webform_stats_render_query($type_id,$nid = FALSE,$aggregate = FALSE){
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
			$joins[] = "INNER JOIN {webform_stats_report_data} m$j ON (wsr.report_id = m$j.report_id AND m$j.metric_id = ".$metric->metric_id.")";
		}
		$node_criteria = ($nid) ? "AND wsr.nid = %d" : "";
		$group_criteria = ($aggregate) ? "GROUP BY wsr.$aggregate" : "";

		$sql = 
			"SELECT wsr.nid, wsr.report_period_start, wsr.report_period_end, " .
			implode(', ',$fields) .
			" FROM {webform_stats_reports wsr} " .
			implode(" ",$joins) .
			" WHERE wsr.type_id = %d" .
			$node_criteria . 
			$group_criteria;
}