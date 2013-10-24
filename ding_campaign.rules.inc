<?php
/**
 * Implements hook_rules_event_info().
 */
function ding_campaign_rules_event_info() {
  $defaults = array(
    'group' => t('Ding Campaign'),
    'module' => t('ding_campaign'),
  );
  return array(
    'ding_campaign_rule_update' => $defaults + array(
      'label' => t('After changing campaign rules'),
    ),
  );
}