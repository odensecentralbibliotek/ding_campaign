<?php

/**
 * @file
 * Contains admin backend related code.
 */

/**
 * Module main settings lay here.
 *
 * @param $form
 *   Form.
 * @param $form_state
 *   Form state.
 *
 * @return
 *   Form structure array.
 */
function ding_campaign_configure($form, $form_state) {
  $types = node_type_get_names();

  // Some key renaming routine, required in further module checks.
  foreach ($types as $k => $v) {
    $types[$k . '_node_form'] = $v;
    unset($types[$k]);
  }

  $form['ding_campaign_node_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('CAMPAIGN SELECTION NODE TYPES') . ':',
    '#default_value' => variable_get('ding_campaign_node_types', array()),
    '#options' => $types,
    '#description' => t('Select node types which shall have campaign selector present in their editing form.'),
  );

  // Prevent campaigns to be assigned to campaigns.
  unset($form['ding_campaign_node_types']['#options']['ding_campaign_node_form']);

  return system_settings_form($form);
}

/**
 * Get rules edit form.
 *
 * @param $node
 *   Campaign node object whose rules are edited.
 *
 * @return
 *   Campaign rules edit form.
 */
function ding_campaign_rules_admin($node) {
  drupal_add_js(array('ding_campaing_init' => array(
    'autocompleteUrl' => url('ding_campaign/autocomplete/'),
  )), 'setting');
  return drupal_get_form('ding_campaign_edit_rules_form', $node);
}

/**
 * Campaign rules edit form.
 *
 * @param $form
 *   Rules form.
 * @param $form_state
 *   Form state.
 * @param $node
 *   Campaign node object whose rules are edited.
 *
 * @return
 *   Campaign rules edit form.
 */
function ding_campaign_edit_rules_form($form, &$form_state, $node = NULL) {
  $num_rules = !empty($form_state['values']['howmany_select']) ? $form_state['values']['howmany_select'] : 0;

  $form['ding_campaign_rules'] = array(
    '#title' => '',
    '#prefix' => '<div id="ding-campaign-rules">',
    '#suffix' => '</div>',
    '#description' => '',
  );

  $i = 0;
  $c = 1;
  if (isset($node->camp_rule) && is_array($node->camp_rule)) {
    $c = count($node->camp_rule);
    for ($i = 0; $i < $c; $i++) {
      $form['ding_campaign_rules']['rules']['rule_' . $i] = ding_campaign_rule_form(
              isset($node->camp_rule[$i]) ? $node->camp_rule[$i] : NULL,
              isset($node->camp_type[$i]) ? $node->camp_type[$i] : 'rule_page');
    }
  }

  for (; $i < $num_rules + $c; $i++) {
    $form['ding_campaign_rules']['rules']['rule_' . $i] = ding_campaign_rule_form(NULL, 'rule_page');
  }

  $form['ding_campaign_rules']['howmany_select'] = array(
    '#type' => 'hidden',
    '#value' => $num_rules += 1,
  );

  $form['ding_campaign_rules']['rule_add'] = array(
    '#type' => 'button',
    '#value' => t('Add another rule'),
    '#ajax' => array(
      'callback' => 'ding_campaign_add_rule',
      'wrapper' => 'edit-rule-add',
      'method' => 'before',
      'effect' => 'fade',
    ),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save rules'),
  );

  $form['#submit'] = array('ding_campaign_campaign_rule_form_submit');

  drupal_add_js(
    drupal_get_path('module', 'ding_campaign') . '/js/ding_campaign_scripts.js',
    array('weight' => 10)
  );

  return $form;
}

/**
 * Appends rule form via AJAX.
 *
 * @param $form
 *   Form.
 * @param $form_state
 *   Form state.
 *
 * @return
 *   Form element - rule fields.
 */
function ding_campaign_add_rule($form, $form_state) {
  $c = count(element_children($form['ding_campaign_rules']['rules']));
  return $form['ding_campaign_rules']['rules']['rule_' . ($c - 1)];
}

/**
 * Implements hook_form_id_submit().
 */
function ding_campaign_campaign_rule_form_submit($form, &$form_state) {
  $c = count($form_state['input']['rule_type']);

  if ($c > 0) {
    // Delete existing rules.
    db_delete('ding_campaign_rules')
      ->condition('cid', $form_state['build_info']['args'][0]->nid)
      ->execute();

    // Add new rules.
    for ($i = 0; $i < $c; $i++) {
      if ($form_state['input']['rule_value'][$i] != '' || $form_state['input']['rule_type'][$i] == 'rule_generic') {

        $nid = NULL;
        if (in_array($form_state['input']['rule_type'][$i], array('rule_page', 'rule_news', 'rule_event', 'rule_taxonomy', 'rule_library'))) {
          preg_match('/\d{1,9}/', $form_state['input']['rule_value'][$i], $matches);
          if (!empty($matches[0])) {
            $nid = $matches[0];
          }
        }

        $dt = array(
          'nid' => $nid,
          'cid' => $form_state['build_info']['args'][0]->nid,
          'rule' => ($form_state['input']['rule_type'][$i] == 'rule_generic') ? '' : $form_state['input']['rule_value'][$i],
          'type' => $form_state['input']['rule_type'][$i],
        );

        db_insert('ding_campaign_rules')
          ->fields($dt)
          ->execute();
      }
    }
    rules_invoke_event('ding_campaign_rule_update');
  }
}

/**
 * Rule form fields chunk.
 *
 * @param $rule
 *   Current rule.
 * @param $type
 *   Current type.
 *
 * @return
 *   Rule fields structure.
 */
function ding_campaign_rule_form($rule = NULL, $type = NULL) {
  $form['rule_type[]'] = array(
    '#type' => 'select',
    '#title' => t('Rule type'),
    '#default_value' => ($type) ? $type : 'rule_page',
    '#options' => array(
      'rule_page' => 'Page',
      'rule_event' => 'Event',
      'rule_news' => 'News',
      'rule_path' => 'Path',
      'rule_term' => 'Search term',
      'rule_taxonomy' => 'Taxonomy term',
      'rule_generic' => 'Generic',
      'rule_library' => 'Library',
    ),
    '#prefix' => '<div class="ding-campaign-rule"><div class="rule-type">',
    '#suffix' => '</div>',
  );

  $form['rule_value[]'] = array(
    '#type' => 'textfield',
    '#title' => t('Rule value'),
    '#default_value' => ($rule) ? $rule : '',
    '#size' => 24,
    '#maxlength' => 24,
    '#prefix' => '<div class="rule-value">',
    '#suffix' => '</div><div class="clear"></div></div>',
    '#autocomplete_path' => 'ding_campaign/autocomplete/' . $type,
  );

  return $form;
}

/**
 * Autocomplete handler.
 *
 * @param $type
 *   Type.
 * @param $input
 *   Input.
 */
function ding_campaign_admin_autocomplete($type, $input) {
  $matches = array();

  switch ($type) {
    case 'rule_page':
      $q = db_query_range("SELECT nid, title FROM {node} WHERE title LIKE(:term)", 0, 5, array(':term' => '%%' . $input . '%%'))->fetchAll();

      foreach ($q as $k => $v) {
        $matches['[nid:' . $v->nid . ']'] = $v->title;
      }
      break;
    case 'rule_taxonomy':
      $q = db_query_range("SELECT tid, name FROM {taxonomy_term_data} WHERE name LIKE(:term)", 0, 5, array(':term' => '%%' . $input . '%%'))->fetchAll();

      foreach ($q as $k => $v) {
        $matches['[tid:' . $v->tid . ']'] = $v->name;
      }
      break;
    case 'rule_library':
      $q = db_query_range("SELECT nid, title FROM {node} WHERE type='ding_library' AND title LIKE(:term)", 0, 5, array(':term' => '%%' . $input . '%%'))->fetchAll();

      foreach ($q as $k => $v) {
        $matches['[nid:' . $v->nid . ']'] = $v->title;
      }
      break;
    case 'rule_event':
      $q = db_query_range("SELECT nid, title FROM {node} WHERE type='ding_event' AND title LIKE(:term)", 0, 5, array(':term' => '%%' . $input . '%%'))->fetchAll();

      foreach ($q as $k => $v) {
        $matches['[nid:' . $v->nid . ']'] = $v->title;
      }
      break;
    case 'rule_news':
      $q = db_query_range("SELECT nid, title FROM {node} WHERE type='ding_news' AND title LIKE(:term)", 0, 5, array(':term' => '%%' . $input . '%%'))->fetchAll();

      foreach ($q as $k => $v) {
        $matches['[nid:' . $v->nid . ']'] = $v->title;
      }
      break;
  }
  drupal_json_output($matches);
}

/**
 * Ajax callback for selecting campaign on the current page.
 *
 * @param $type
 *   Type.
 * @param $input
 *   Input.
 */
function ding_campaign_admin_select($js = NULL, $step = NULL) {
  if (!$js) {
    return drupal_get_form('ding_campaign_admin_select_form');
  }
  ctools_include('modal');
  ctools_include('ajax');
  $form_state = array(
    'title' => t('Select campaigns'),
    'ajax' => TRUE,
  );
  $output = ctools_modal_form_wrapper('ding_campaign_admin_select_form', $form_state);
  print ajax_render($output);
  exit;
}

function ding_campaign_admin_select_form($form, &$form_state) {
  $destination = drupal_get_destination();
  $split_destination = parse_url($destination['destination']);
  $path = $split_destination['path'];
  
  $path_alias = drupal_get_path_alias($path);
  $front_page = variable_get('site_frontpage', 'node');
  if ($path == $front_page) {
    $path_alias = '<front>';
  }
  $form['path'] = array(
    '#type' => 'hidden',
    '#default_value' => $path,
  );
  $form['path_alias'] = array(
    '#type' => 'hidden',
    '#default_value' => $path_alias,
  );
  $defaults = array();
  $path_arr = explode('/', $path);
  if ($path_arr[0] == 'node') {
    $node = node_load($path_arr[1]);
    $enabled_types = variable_get('ding_campaign_node_types', array());
    if (isset($enabled_types[$node->type . '_node_form']) && $enabled_types[$node->type . '_node_form'] === $node->type . '_node_form') {
      $form['#node'] = $node;
      if (isset($node->ding_campaigns) && is_array($node->ding_campaigns)) {
        $defaults = $node->ding_campaigns;
      }
    }
  }
  // Process path-based campaigns.
  $rules = db_query('SELECT r.cid, r.rule
    FROM {ding_campaign_rules} r, {ding_campaign} d, {node} n
    WHERE r.type = \'rule_path\'
      AND n.status = 1
      AND r.cid = d.cid
      AND r.cid = n.nid
    ')->fetchAll();

  foreach ($rules as $rule) {
    $page_match = drupal_match_path($path_alias, $rule->rule);

    if ($path != $path_alias) {
      $page_match = $page_match || drupal_match_path($path, $rule->rule);
    }

    if ($page_match) {
      if (strpos($rule->rule, "*")) {
        // Enabled for multiple paths.
        $with_wildcard[$rule->cid] = $rule->rule;
      } else {
        // Enabled for this specific path.
        $defaults[$rule->cid] = $rule->cid;
      }
    }
  }
  $form['ding_campaigns_wrap'] = array(
    '#type' => 'fieldset',
    '#title' => 'Available campaigns',
  );
  $options = ding_campaign_get_list();
  if (!empty($with_wildcard)) {
    foreach ($with_wildcard as $cid => $rule) {
      $options[$cid] .= " (also enabled by this 'path rule': <a href=http://o722.dev/node/$cid/campaign_rules>$rule</a>)";
    }
  }
  $form['ding_campaigns_wrap']['ding_campaigns'] = array(
    '#type' => 'checkboxes',
    '#default_value' => $defaults,
    '#options' => $options,
    '#description' => 'Select campaigns asociated with this node.',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}
function ding_materials_save_workaround($node)
{
    $TingMaterials = array();
    if(isset($node))
    {
      $PropertyName = null;
      foreach($node as $Property => $Value)
      {
          if (strpos($Property, '_materials') !== FALSE)
          {
              $PropertyName = $Property;
              break;
          }
      }
      if($PropertyName != null)
      {
          if(sizeof($node->$PropertyName) != 0)
          {
              $Data = $node->$PropertyName; // needed as we cant implicit reference members with dynamic property.
              foreach($Data["und"] as $Materials)
              {
                    //sanity check. if 0 or not set. break.
                if(!isset($Materials['value']->endpoints["und"]) && sizeof($Materials['value']->endpoints["und"]) != 0)
                {
                    break;
                }
                foreach($Materials['value']->endpoints["und"] as $endpoint)
                {
                    if($endpoint["entity_type"] == "ting_object")
                    {
                        //Load Materials obj.
                        $Entityid = $endpoint["entity_id"];
                        $TingMaterial = entity_load("ting_object",array($Entityid) );
                        /*
                        * Create replacement structure for ting_reference.
                         * Issues: $container["object"] is only neeeded in certain versions.
                         * Need to verify for new platforms if needed or not.
                        */
                        $container = array();
                        $container["tid"] = $TingMaterial[$Entityid]->tid;
                        $container["object"] = $TingMaterial[$Entityid];
                        $container["ting_object_id"] = $TingMaterial[$Entityid]->ding_entity_id;
                        array_push($TingMaterials,$container);
                    }
                }
              }
          }
      }
      
    }
    return $TingMaterials;
}
function ding_campaign_admin_select_form_submit($form, &$form_state) {
  $destination = drupal_get_destination();
  $path = $form_state['values']['path'];
  $path_alias = $form_state['values']['path_alias'];
  $defaults = $form['ding_campaigns_wrap']['ding_campaigns']['#default_value'];
  // Throw away all disabled campaigns.
  $campaigns = array_filter($form_state['input']['ding_campaigns']);

  if (isset($form['#node'])) {
    $raw_node = $form['#node'];
    $raw_node->ding_campaigns = $campaigns;
    //replace ting_materials_reference with workaround.
    $PropertyName = null;
    foreach($raw_node as $Property => $Value)
    {
        if (strpos($Property, '_materials') !== FALSE)
        {
            $PropertyName = $Property;
            break;
        }
    }
    $ref = $raw_node->$PropertyName;
    $ref["und"] = ding_materials_save_workaround($raw_node);
    $raw_node->$PropertyName = $ref;
    node_save($raw_node);
  } else {
    $added = array_diff_key($campaigns, $defaults);
    $removed = array_diff_key($defaults, $campaigns);
    $rule = 'rule_path';

    foreach ($added as $campaign) {
      $dt = array(
        'nid' => NULL,
        'cid' => $campaign,
        'rule' => $path_alias,
        'type' => $rule,
      );

      $success = db_insert('ding_campaign_rules')
        ->fields($dt)
        ->execute();
      if (!$success) {
        drupal_set_message(t('Could not add campaign rule'), 'error');
      }
    }

    foreach ($removed as $campaign) {
      // Remove the relevant rule_path for this campaign.
      $path_or = db_or()->condition('rule', $path)->condition('rule', $path_alias);
      $rules = db_and()->condition('type', 'rule_path')->condition($path_or);
      $success = db_delete('ding_campaign_rules')
        ->condition('cid', $campaign)
        ->condition($rules)
        ->execute();
      if (!$success) {
        drupal_set_message(t('Could not delete campaign rule'), 'error');
      }
    }
  }
  rules_invoke_event('ding_campaign_rule_update');
  if (isset($form_state['ajax']) && $form_state['ajax']) {
    ctools_include('ajax');
    $options = drupal_parse_url($destination['destination']);
    $commands[] = ctools_ajax_command_redirect($options['path'], 0, $options);
    print ajax_render($commands);
    exit;
  }
}
