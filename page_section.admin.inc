<?php

/**
 * @file
 * Provide functions to manage Page Section Types.
 */

/**
 * Menu callback. Page section types overview.
 */
function page_section_overview_types() {
  $types = page_section_type_info();
  $field_ui = module_exists('field_ui');

  $header = array(t('Name'));
  $header[] = array('data' => t('Operations'), 'colspan' => $field_ui ? 3 : 1);

  $rows = array();
  foreach ($types as $type => $info) {
    $type_url_str = str_replace('_', '-', $info->name);
    $row = array(
      check_plain($info->label),
      l(t('edit'), 'admin/structure/page-sections/manage/' . $type_url_str . '/edit'),
    );

    if ($field_ui) {
      $row[] = l(t('manage fields'), 'admin/structure/page-sections/manage/' . $type_url_str . '/fields');
      $row[] = l(t('manage display'), 'admin/structure/page-sections/manage/' . $type_url_str . '/display');
    }

    $rows[] = $row;
  }

  $build = array();
  $build['page_section_type_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No page section types available.'),
  );

  return $build;
}

/**
 * Menu callback. Display page section type.
 */
function page_section_type_page($type) {
  return drupal_get_form('page_section_type_form', $type);
}

/**
 * Menu callback. Add Page Section type.
 */
function page_section_type_add_page() {
  $type = (object) array(
    'status' => PAGE_SECTION_NOT_SAVED,
  );

  return drupal_get_form('page_section_type_form', $type);
}

/**
 * Form API callback to edit a page section type.
 */
function page_section_type_form($form, &$form_state, $type) {
  $form['#page_section_type'] = $type;

  $form['label'] = array(
    '#title' => t('Label'),
    '#type' => 'textfield',
    '#default_value' => !empty($type->label) ? $type->label : '',
    '#description' => t('The human-readable name of this page secton type. This text will be displayed in the add buttons.'),
    '#required' => TRUE,
    '#size' => 30,
  );

  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => !empty($type->name) ? $type->name : '',
    '#maxlength' => 32,
    '#disabled' => $type->status > PAGE_SECTION_NOT_SAVED,
    '#machine_name' => array(
      'exists' => 'page_section_type_load',
      'source' => array('label'),
      'replace_pattern' => '[^0-9a-z_\-]',
      'error' => t('Please only use lowercase alphanumeric characters, underscores (_), and hyphens (-) for page section name.'),
    ),
    '#description' => t('A unique machine-readable name for this page section type. It must only contain lowercase letters, numbers, and underscores.'),
  );

  $form['actions'] = array('#type' => 'actions');

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Form API submit callback for page section type edit form.
 */
function page_section_type_form_submit($form, &$form_state) {
  $type = $form['#page_section_type'];

  if ($type->status == PAGE_SECTION_NOT_SAVED) {
    $type->name = $form_state['values']['name'];
  }
  $type->label = $form_state['values']['label'];

  page_section_type_save($type);

  $form_state['redirect'] = 'admin/structure/page-sections';
}
