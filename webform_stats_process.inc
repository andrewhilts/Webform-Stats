<?php
function webform_stats_process(){
  drupal_set_message('Yarb!');
}

//Check if it's time to run the report for a statistical grouping
function webform_stats_type_check_if_time($type){
  $current_day = intval(date("w")); //Return eg "Sunday" => 0
  $current_time = intval(date("H")); //Return hour

  if($current_day == $type->scheduling_day && $current_time >= $type->scheduling_hour && $current_time <= ($type->scheduling_hour+$type->scheduling_window)){
    $is_time = TRUE;
  }
  else{
    $is_time = FALSE;
  }
  return $is_time;
}

//Check if queue exists for the statistical grouping, and if so, check the number of items in the queue
function webform_stats_type_queue_process($type_id){
  $queue_name = "webform_stats_".$type_id."_nodeprep";
  $queue = drupal_queue_get($queue_name); // grab queue
  $count = $queue->numberOfItems();

  $build_queue = drupal_queue_get('webform_stats_metric_build');
  $data_queue = drupal_queue_get('webform_stats_metric_data_get');

  $build_queue_count = $build_queue->numberOfItems();
  $data_queue_count = $data_queue->numberOfItems();

  if($count == 0 && $build_queue_count == 0 && $data_queue_count == 0){
    //build queue
    webform_stats_type_queue_build($type_id,$queue_name);
    dsm('queue created: '.$queue_name);
  }
  else{
    //queue gets worked on.
  }
}

function webform_stats_type_queue_build ($type_id,$queue_name){
  $type = webform_stats_get_type_details($type_id);
  $queue = drupal_queue_get($queue_name);

  foreach(webform_stats_get_type_webforms($type_id,'detailed') as $i => $node_info){
    if($node_info->complete == 0){
      $items[] = array(
        'node_info' => $node_info,
        'base_date' => $type->base_date,
        'interval' => $type->report_interval,
        'type_id' => $type_id,
      );
    }
  }

  $queue->createQueue(); // initialize
  foreach ($items as $item) {
    $queue->createItem($item); // each item passed as param to worker
  }

    //for nodes :: Build node queue
    //for node report type metric :: Build metric queue
      //check dates required for this metric
      //for each metric - dates :: Build metric date queue
}

function webform_stats_check_for_submissions($nid){
  $sql = "SELECT MIN(submitted) as earliest_submission, MAX(submitted) as latest_submission FROM {webform_submissions} WHERE nid=%d GROUP BY nid LIMIT 1";
  $result = db_query($sql,$nid);
  $submission_data = db_fetch_object($result);
  if(!$submission_data){
    drupal_set_message('webform_stats','no submissions found for node: '.$nid);
    return false;
  }
  if(!$submission_data->earliest_submission){
    drupal_set_message('webform_stats','no submissions found for node: '.$nid);
    return false;
  }
  if(!$submission_data->earliest_submission > 0){
    drupal_set_message('webform_stats','no submissions found for node: '.$nid);
    return false;
  }
  return $submission_data;
}

function webform_stats_type_get_latest_report($nid,$type_id){
  $sql =
  "SELECT report_period_end
    FROM {webform_stats_reports} r
  WHERE r.nid=%d
    AND r.type_id=%d
  ORDER BY report_period_end DESC
  LIMIT 1";

  $result = db_query($sql,$nid,$type_id);
    while($report = db_fetch_object($result)) {
      if($report->report_period_end > 0){
        return $report->report_period_end;
      }
      else{
        return false;
      }
    }
  return false;
}

function webform_stats_type_metric_get_latest_report($nid,$type_id,$metric_id){
  $sql =
  "SELECT report_period_end
    FROM {webform_stats_reports} r
  LEFT JOIN {webform_stats_report_data} d
    ON (r.report_id = d.report_id)
  WHERE r.nid=%d
    AND r.type_id=%d
    AND d.metric_id = %d
  ORDER BY report_period_end DESC
  LIMIT 1";

  $result = db_query($sql,$nid,$type_id,$metric_id);
    while($report = db_fetch_object($result)) {
      if(!$report->report_period_end > 0){
        return false;
      }
      else{
        return $report->report_period_end;
      }
    }
  return false;
}

//Check latest data for node-type-metric; is it out of date?
function webform_stats_node_type_metric_data_check($node_info,$type,$metric){
  $data_id = 't'.$type->type_id."/n".$node_info->nid."/m".$metric->metric_id;
  webform_stats_debug('webform_stats','Data check for: '.$data_id);
  //Check for latest report
  $latest_report = webform_stats_type_metric_get_latest_report($node_info->nid,$type->type_id,$metric->metric_id);
  if($latest_report){
    $date_range->min = $latest_report+1;
    $date_range->max = $node_info->submissions->latest_submission;
    webform_stats_debug('webform_stats', 'latest report found for '.$node_info->nid.' at time: '.$latest_report);
  }
  else{
    $date_range->min = webform_stats_base_date_factor($node_info->submissions->earliest_submission,$type->base_date,$type->report_interval);
    $date_range->max = $node_info->submissions->latest_submission;
    webform_stats_debug('webform_stats', 'NO latest report found ('.$data_id.')');
  }
  if($node_info->published == 1){
    $date_range->max = webform_stats_base_date_factor(time(),$type->base_date,$type->report_interval);
    webform_stats_debug('webform_stats', 'Node '.$node_info->nid.' is published, hence use current time as max time: '.$date_range->max.' ('.$data_id.')');
  }
  else{
    if($latest_report){
      if($latest_report+1 > $node_info->submissions->latest_submission){
        //Node should have been marked as complete. This shouldn't fire.
        return false;
      }
      else{
        $date_range->max = $node_info->submissions->latest_submission;
        webform_stats_debug('webform_stats', 'Node '.$node_info->nid.' still has submissions to collected, although unpublished.');
      }
    }
  }
  if($date_range->max == $date_range->min){
    webform_stats_debug('webform_stats', 'Not enough time has passed to warrant a new report for node '.$node_info->nid.'.');
    return false;
  }
  return $date_range;
}

function webform_stats_completeness_check($node_info,$type_id){
  //Return false if node is still published
  if($node_info->published == 1){
    return FALSE;
  }
  //Check for node's latest report for type
  $latest_report = webform_stats_type_get_latest_report($node_info->nid,$type_id);
  if(!$latest_report){
    return FALSE;
  }

  if($latest_report+1 > $node_info->submissions->latest_submission){
  //mark as complete
    webform_stats_debug('webform_stats', 'Node '.$node_info->nid.' for type '.$type_id.' marked as complete. Last submission at time '.$node_info->submissions->latest_submission.' is less than latest report at time '.$latest_report);

    webform_stats_type_node_mark_complete($node_info->nid,$type_id);

    return TRUE;
  }
  else{
    return FALSE;
  }
}

function webform_stats_build_date_range($date_range,$interval){
  webform_stats_debug('webform_stats','building date range from min: '.$date_range->min.' to max: '.$date_range->max);

  //Case when wrapping up webforms that have been unpublished
  if($date_range->max-$date_range->min <= $interval){
    $period = new Stdclass;
    $period->min = intval($date_range->min);
    $period->max = intval($date_range->min+$interval-1);
    $dates = array($period);
  }
  $last_report_start_time = $date_range->max-$interval;
  //Get date periods
  for ($i=$date_range->min; $i <= $last_report_start_time; $i = $i + $interval) {
    $period = new Stdclass;
    $period->min = intval($i);
    $period->max = intval($i+$interval-1);
    $dates[] = $period;
  }
  if(!isset($dates)){
    return FALSE;
  }
  return $dates;
}

//Get report id for node-type date period
function webform_stats_report_init($reporting_period,$nid,$type_id){
  $sql = "SELECT COUNT(DISTINCT report_id) as init, report_id FROM {webform_stats_reports} WHERE report_period_start = %d AND report_period_end = %d AND type_id = %d AND nid = %d";

  $result = db_query($sql,$reporting_period->min,$reporting_period->max,$type_id,$nid);
  $existing_report = db_fetch_object($result);
  if(0 === intval($existing_report->init)){

    $report = new Stdclass;
    $report->nid = $nid;
    $report->type_id = $type_id;
    $report->report_period_start = $reporting_period->min;
    $report->report_period_end = $reporting_period->max;
    global $user;
    $report->generated_by = $user->uid;
    $report->report_date = time();
    $result = drupal_write_record('webform_stats_reports',$report);
    if($result){
          webform_stats_debug('webform_stats',"created record");
          return webform_stats_report_init($reporting_period,$nid,$type_id);
        }
        else{
          return false;
          webform_stats_debug('webform_stats','Could not create report');
        }
  }
  elseif($existing_report->report_id){
    webform_stats_debug('webform_stats','Got report: '.$existing_report->report_id);
    return $existing_report->report_id;
  }
  else{
    return false;
    webform_stats_debug('webform_stats','Could not create report');
  }
}

function webform_stats_get_cid($nid,$metric){
  $sql = "SELECT DISTINCT cid FROM {webform_component} WHERE nid=%d AND form_key = '%s' LIMIT 1";
  $result = db_query($sql,$nid,$metric->form_key);
  $component = db_fetch_object($result);
  if(!$component->cid){
    webform_stats_debug('webform_stats',"no cid found");
    return FALSE;
  }
  return $component->cid;
}