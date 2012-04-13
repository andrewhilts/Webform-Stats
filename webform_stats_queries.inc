<?php

function _webform_stats_email_report($nid,$reporting_period,$metric,$cids,$type_id,$report_id){
	$cid_email = 9;
	
	if(!$metric->uniques){
		webform_stats_debug('webform_stats',"email report for node ".$nid." and r".$report_id." with min date: ".$reporting_period->min." and cid:".$cids[$metric->metric_id]);
		$sql = 
		"SELECT COUNT(wsd.data) FROM 
				{webform_submitted_data} wsd
		 INNER JOIN {webform_submissions} ws ON 
			 	(ws.nid = wsd.nid AND ws.sid = wsd.sid)
		 WHERE wsd.nid=%d 
				AND wsd.cid=%d
				AND ws.submitted >= %d
				AND ws.submitted <= %d";
			
		$result = db_query(
			$sql,
			$nid,
			$cids[$metric->metric_id],
			$reporting_period->min,
			$reporting_period->max
		);
	}
	else{
		webform_stats_debug('webform_stats',"Unique email report for node ".$nid." and r".$report_id." with min date: ".$reporting_period->min." and cid:".$cids[$metric->metric_id]);
		//Get previous unique entries only
		$sql_previous_u_email = 
		"CREATE TEMPORARY TABLE 
		webform_stats_temp_email
		(email varchar(255), PRIMARY KEY(email))
		SELECT DISTINCT wsd.data as email FROM 
				{webform_submitted_data} wsd 
		 INNER JOIN {webform_submissions} ws ON 
			 	(ws.nid = wsd.nid AND ws.sid = wsd.sid)
		 WHERE wsd.nid=%d
				AND wsd.cid=%d
			AND ws.submitted < %d
			GROUP BY wsd.data";
			
		db_query(
			$sql_previous_u_email,
			$nid,
			$cids[$metric->metric_id],
			$reporting_period->min
		);
		
	
		//Get this week's new unique emails			
		$sql_u_email = 
		"SELECT COUNT(DISTINCT wsd.data) FROM 
				{webform_submitted_data} wsd
		 INNER JOIN {webform_submissions} ws ON 
			 		(ws.nid = wsd.nid AND ws.sid = wsd.sid)
		 LEFT OUTER JOIN {webform_stats_temp_email} temp ON
			(temp.email = wsd.data)
		 WHERE wsd.nid=%d 
		 		AND wsd.cid=%d
				AND ws.submitted >= %d
				AND ws.submitted <= %d
				AND temp.email IS NULL
		 GROUP BY wsd.nid";
		
		$result = db_query(
						$sql_u_email,
						$nid,
						$cids[$metric->metric_id],
						$reporting_period->min,
						$reporting_period->max
						);
		
		//Drop temp table
		db_query("DROP TABLE {webform_stats_temp_email}");		
	}	
	$total = db_result($result);
	$total = intval($total);
	return $total;
}
function _webform_stats_select_report($nid,$reporting_period,$metric,$cids,$type_id,$report_id){
	$cid_email = 9;
	
	if(!$metric->uniques){
		webform_stats_debug('webform_stats',"select report for node ".$nid." and r".$report_id." with min date: ".$reporting_period->min." and cid:".$cids[$metric->metric_id]);
		$sql = 
		"SELECT SUM(wsd.data) FROM 
				{webform_submitted_data} wsd
		 INNER JOIN {webform_submissions} ws ON 
			 	(ws.nid = wsd.nid AND ws.sid = wsd.sid)
		 WHERE wsd.nid=%d 
				AND wsd.cid=%d
				AND ws.submitted >= %d
				AND ws.submitted <= %d";
			
		$result = db_query(
			$sql,
			$nid,
			$cids[$metric->metric_id],
			$reporting_period->min,
			$reporting_period->max
		);
	}
	else{
		webform_stats_debug('webform_stats',"Unique select report for node ".$nid." and r".$report_id." with min date: ".$reporting_period->min." and cid:".$cids[$metric->metric_id]);
		//Get previous unique selects only
		$sql_previous_u_select = 
		"CREATE TEMPORARY TABLE 
		{webform_stats_temp_select}
		(email varchar(255), PRIMARY KEY(email))
		SELECT DISTINCT wsd.data as email FROM 
				{webform_submitted_data} wsd 
			INNER JOIN {webform_submitted_data} wsd2 ON 
				(wsd.nid = wsd2.nid AND wsd.sid = wsd2.sid AND wsd2.cid=%d AND wsd2.data = 1) 
		 INNER JOIN {webform_submissions} ws ON 
			 	(ws.nid = wsd.nid AND ws.sid = wsd.sid)
		 WHERE wsd.nid=%d
				AND wsd.cid=%d
			AND ws.submitted < %d
			GROUP BY wsd.data";	
		db_query(
			$sql_previous_u_select,
			$cids[$metric->metric_id],
			$nid,
			$cid_email,
			$reporting_period->min
		);
	
		//Get this week's new unique selects only				
		$sql_u_select = 
	"SELECT COUNT(DISTINCT wsd.data) FROM 
			{webform_submitted_data} wsd 
	 INNER JOIN {webform_submitted_data} wsd2 ON 
			(wsd.nid = wsd2.nid AND wsd.sid = wsd2.sid) 
	 INNER JOIN {webform_submissions} ws ON 
		 	(ws.nid = wsd.nid AND ws.sid = wsd.sid)
	 LEFT OUTER JOIN {webform_stats_temp_select} temp ON
			(wsd.data = temp.email)
	 WHERE wsd.nid=%d 
			AND wsd.cid=%d 
			AND wsd2.cid=%d 
			AND wsd2.data = 1
			AND ws.submitted >= %d
			AND ws.submitted <= %d
			AND temp.email IS NULL
		GROUP BY wsd.nid";
									
		$result = db_query(
						$sql_u_select,
						$nid,
						$cid_email,
						$cids[$metric->metric_id],
						$reporting_period->min,
						$reporting_period->max
						);
		
		//Drop temp table
		db_query("DROP TABLE {webform_stats_temp_select}");		
	}
	$total = db_result($result);
	$total = intval($total);
	return $total;
}
function _webform_stats_mailchimp_report($nid,$reporting_period,$metric,$cids,$type_id,$report_id){
	$cid_email = 9;
	
	
	if(!$metric->uniques){
		webform_stats_debug('webform_stats',"mailchimp report for node ".$nid." and r".$report_id." with min date: ".$reporting_period->min." and cid:".$cids[$metric->metric_id]);
		$sql = 
		"SELECT COUNT(wsd.data) FROM 
				{webform_submitted_data} wsd
		 INNER JOIN {webform_submissions} ws ON 
			 	(ws.nid = wsd.nid AND ws.sid = wsd.sid)
		 WHERE wsd.nid=%d 
				AND wsd.cid=%d
				AND wsd.no='mailchimp_signup'
				AND wsd.data = 1
				AND ws.submitted >= %d
				AND ws.submitted <= %d";
		$result = db_query(
			$sql,
			$nid,
			$cids[$metric->metric_id],
			$reporting_period->min,
			$reporting_period->max
		);
	}
	else{
		webform_stats_debug('webform_stats',"Unique mailchimp report for node ".$nid." and r".$report_id." with min date: ".$reporting_period->min." and cid:".$cids[$metric->metric_id]);
		//Get previous unique mailchimps only
		$sql_previous_u_mailchimp = 
		"CREATE TEMPORARY TABLE 
		{webform_stats_temp_mailchimp}
		(email varchar(255), PRIMARY KEY(email))
		SELECT DISTINCT wsd.data as email FROM 
				{webform_submitted_data} wsd 
			INNER JOIN {webform_submitted_data} wsd2 ON 
				(wsd.nid = wsd2.nid AND wsd.sid = wsd2.sid AND wsd2.cid=%d AND wsd2.data = 1 AND wsd2.no = 'mailchimp_signup') 
		 INNER JOIN {webform_submissions} ws ON 
			 	(ws.nid = wsd.nid AND ws.sid = wsd.sid)
		 WHERE wsd.nid=%d
				AND wsd.cid=%d
			AND ws.submitted < %d
			GROUP BY wsd.data";
	
		db_query(
			$sql_previous_u_mailchimp,
			$cids[$metric->metric_id],
			$nid,
			$cid_email,
			$reporting_period->min
		);
	
		//Get this week's new unique mailchimp opt-ins only				
		$sql_u_mailchimp = 
	"SELECT COUNT(DISTINCT wsd.data) FROM 
			{webform_submitted_data} wsd 
	 INNER JOIN {webform_submitted_data} wsd2 ON 
			(wsd.nid = wsd2.nid AND wsd.sid = wsd2.sid) 
	 INNER JOIN {webform_submissions} ws ON 
		 	(ws.nid = wsd.nid AND ws.sid = wsd.sid)
	 LEFT OUTER JOIN {webform_stats_temp_mailchimp} temp ON
			(wsd.data = temp.email)
	 WHERE wsd.nid=%d 
			AND wsd.cid=%d 
			AND wsd2.cid=%d 
			AND wsd2.data = 1
			AND wsd2.no = 'mailchimp_signup'
			AND ws.submitted >= %d
			AND ws.submitted <= %d
			AND temp.email IS NULL
		GROUP BY wsd.nid";
		
		$result = db_query(
						$sql_u_mailchimp,
						$nid,
						$cid_email,
						$cids[$metric->metric_id],
						$reporting_period->min,
						$reporting_period->max
						);
		
		//Drop temp table
		db_query("DROP TABLE {webform_stats_temp_mailchimp}");		
	}
	$total = db_result($result);
	$total = intval($total);
	return $total;
	
}