<?php

/**
 * Implements hook_features_api().
 */
function page_section_features_api() {
  return array(
    'page_section' => array(
      'name' => t('Page section types'),
      'feature_source' => TRUE,
      'default_hook' => 'page_section_type_info',
      'file' => drupal_get_path('module', 'page_section') . '/page_section.features.inc',
    ),
  );
}

/**
 * Implements hook_features_export_options().
 */
function page_section_features_export_options() {
  $types = page_section_type_info();

  $result = array();
  foreach ($types as $type => $info) {
    $result[$type] = $info->label;
  }

  return $result;
}

/**
 * Implements hook_features_export.
 */
function page_section_features_export($data, &$export, $module_name = '') {
  $pipe = array();
  $map = features_get_default_map('page_section');
  $types = page_section_type_info();

  foreach ($data as $type) {
    if (!empty($types[$type])) {
      // If this node type is provided by a different module, add it as a dependency
      if (isset($map[$type]) && $map[$type] != $module_name) {
        $export['dependencies'][$map[$type]] = $map[$type];
      }
      // Otherwise export the node type.
      else {
        $export['features']['page_section'][$type] = $type;
        $export['dependencies']['page_section'] = 'page_section';
        $export['dependencies']['features'] = 'features';
      }

      $fields = field_info_instances('page_section', $type);
      foreach ($fields as $name => $field) {
        $pipe['field'][] = "page_section-{$field['bundle']}-{$field['field_name']}";
      }
    }
  }

  return $pipe;
}

/**
 * Implements hook_features_export_render().
 */
function page_section_features_export_render($module, $data, $export = NULL) {
  $types = page_section_type_info();

  $output = array();
  $output[] = '  $items = array(';
  foreach ($data as $type) {
    if (!empty($types[$type])) {
      $output[] = "    '{$type}' => array(";
      $output[] = "      'label' => \"" . addslashes($types[$type]->label) . "\",";
      $output[] = "    ),";
    }
  }
  $output[] = '  );';
  $output[] = '  return $items;';
  $output = implode("\n", $output);
  return array('page_section_type_info' => $output);
}

/**
 * Implements hook_features_revert().
 *
 * @param $module
 * name of module to revert content for
 */
function page_section_features_revert($module = NULL) {
  if ($default_types = features_get_default('page_section', $module)) {
    foreach ($default_types as $type_name => $type_info) {
      db_delete('page_section_type')
        ->condition('name', $type_name)
        ->execute();
    }
    page_section_type_info(TRUE);
    menu_rebuild();
  }
}
