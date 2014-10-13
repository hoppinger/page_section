<?php

/**
 * @file
 * Provide the Field integration for Page Section.
 */

/**
 * Implements hook_field_info().
 */
function page_section_field_info() {
  return array(
    'page_section' => array(
      'label' => t('Page section'),
      'description' => t('This field stores references to page sections, which itself may contain any number of fields.'),
      'instance_settings' => array(),
      'default_widget' => 'page_section_form',
      'default_formatter' => 'page_section_view',
      'settings' => array(),
      'property_type' => 'page_section',
    ),
  );
}

/**
 * Implements hook_field_settings_form().
 */
function page_section_field_settings_form($field, $instance, $has_data) {
  $settings = $field['settings'];

  $types = page_section_type_info();
  $type_names = array();
  foreach ($types as $key => $info) {
    $type_names[$key] = $info->label;
  }

  $form = array();
  $form['referenceable_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Page section types that can be referenced'),
    '#description' => t('If not types are selected, all types can be referenced'),
    '#multiple' => TRUE,
    '#default_value' => empty($settings['referenceable_types']) ? array() : $settings['referenceable_types'],
    '#options' => array_map('check_plain', $type_names),
  );

  return $form;
}

/**
 * Implements hook_field_presave().
 */
function page_section_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
  foreach ($items as &$item) {
    if (isset($item['section'])) {
      page_section_save($item['section']);
      $item = array('sid' => $item['section']->sid);
    }
  }
}

/**
 * Implements hook_field_prepare_translation().
 */
function page_section_field_prepare_translation($entity_type, $entity, $field, $instance, $langcode, &$items, $source_entity, $source_langcode) {
  $info = entity_get_info('page_section');
  foreach ($items as $delta => $item) {
    if (isset($item['sid'])) {
      $page_section_source = page_section_load($item['sid']);
      unset($items[$delta]['sid']);

      $page_section_new = entity_create('page_section', array('type' => $page_section_source->type));
      field_attach_prepare_translation('page_section', $page_section_new, $langcode, $page_section_source, $source_langcode);

      $items[$delta]['section'] = $page_section_new;
    }
  }
}

/**
 * Implements hook_field_is_empty().
 */
function page_section_field_is_empty($item, $field) {
  if (!empty($item['sid'])) {
    return FALSE;
  }
  if (isset($item['section'])) {
    return page_section_section_is_empty($item['section']);
  }
  return TRUE;
}

/**
 * Determine if a Page Section is empty.
 */
function page_section_section_is_empty($section) {
  $instances = field_info_instances('page_section', $section->type);

  if (empty($instances)) {
    return FALSE; // If there are no fields in the page section, we want to continue
  }

  foreach ($instances as $instance) {
    $field_name = $instance['field_name'];
    $field = field_info_field($field_name);

    // Determine the list of languages to iterate on.
    $languages = field_available_languages('page_section', $field);

    foreach ($languages as $langcode) {
      if (!empty($section->{$field_name}[$langcode])) {
        // If at least one section-field is not empty; the
        // page section is not empty.
        foreach ($section->{$field_name}[$langcode] as $field_item) {
          if (!module_invoke($field['module'], 'field_is_empty', $field_item, $field)) {
            return FALSE;
          }
        }
      }
    }
  }

  return TRUE;
}

/**
 * Implements hook_field_formatter_info().
 */
function page_section_field_formatter_info() {
  return array(
    'page_section_view' => array(
      'label' => t('Page sections'),
      'field types' => array('page_section'),
      'settings' => array(
        'view_mode' => 'full',
      ),
    ),
  );
}

/**
 * Implements hook_field_formatter_settings_form().
 */
function page_section_field_formatter_settings_form($field, $instance, $view_mode, $form, &$form_state) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];
  $elements = array();

  if ($display['type'] !== 'page_section_view') {

    $entity_type = entity_get_info('page_section');
    $options = array();
    foreach ($entity_type['view modes'] as $mode => $info) {
      $options[$mode] = $info['label'];
    }

    $elements['view_mode'] = array(
      '#type' => 'select',
      '#title' => t('View mode'),
      '#options' => $options,
      '#default_value' => $settings['view_mode'],
      '#description' => t('Select the view mode'),
    );
  }

  return $elements;
}

/**
 * Implements hook_field_formatter_settings_summary().
 */
function page_section_field_formatter_settings_summary($field, $instance, $view_mode) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];
  $output = array();

  if ($display['type'] == 'page_section_view') {
    $entity_type = entity_get_info('page_section');
    if (!empty($entity_type['view modes'][$settings['view_mode']]['label'])) {
      $output[] = t('View mode: @mode', array('@mode' => $entity_type['view modes'][$settings['view_mode']]['label']));
    }
  }

  return implode('<br />', $output);
}

/**
 * Implements hook_field_formatter_view().
 */
function page_section_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();
  $settings = $display['settings'];

  switch ($display['type']) {
    case 'page_section_view':
      $view_mode = !empty($display['settings']['view_mode']) ? $display['settings']['view_mode'] : 'full';

      $sids = array();
      foreach ($items as $delta => $item) {
        if (empty($item['section']) && !empty($item['sid'])) {
          $sids[] = $item['sid'];
        }
      }
      $sections_loaded = page_section_load_multiple($sids);

      $sections = array();
      foreach ($items as $delta => $item) {
        $section = NULL;

        if (!empty($item['section'])) {
          $section = $item['section'];
        }
        elseif (!empty($item['sid']) && !empty($sections_loaded[$item['sid']])) {
          $section = $sections_loaded[$item['sid']];
        }

        if (isset($section)) {
          $section->host_entity_type = $entity_type;
          $section->host_entity = $entity;
          $element[$delta] = page_section_view($section);
        }
      }
      break;
  }

  return $element;
}

/**
 * Implements hook_field_widget_info().
 */
function page_section_field_widget_info() {
  return array(
    'page_section_form' => array(
      'label' => t('Form'),
      'field types' => array('page_section'),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_CUSTOM,
        'default value' => FIELD_BEHAVIOR_DEFAULT,
      ),
    ),
  );
}

/**
 * Implements hook_field_widget_form().
 */
function page_section_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  static $recursion = 0;

  if ($instance['widget']['type'] == 'page_section_form') {
    $parents = $form['#parents'];

    $page_section_types = page_section_type_info();
    $field_parents = $element['#field_parents'];
    $field_name = $element['#field_name'];
    $language = $element['#language'];

    $field_state = field_form_get_state($field_parents, $field_name, $language, $form_state);
    $field = $field_state['field'];

    $id_prefix = implode('-', array_merge($parents, array($field_name)));
    $wrapper_id = drupal_html_id($id_prefix . '-add-more-wrapper');

    if ($recursion++ > 3) {
      drupal_set_message(t('The page section form has not been embedded to avoid recursive loopage_section.'), 'error');
      return $element;
    }

    $type_info = page_section_type_info();
    $referenceable_types = empty($field['settings']['referenceable_types']) ? array_keys($type_info) : array_keys(array_filter($field['settings']['referenceable_types']));

    if (!isset($field_state['count_items'])) {
      $field_state['count_items'] = count($items);

      if ($field_state['count_items'] == 0 && count($referenceable_types) == 1) {
        $field_state['section'][0] = page_section_create(array('type' => $referenceable_types[0]));
        $field_state['count_items'] = 1;
      }
    }

    $multiple = $field['cardinality'] > 1 || $field['cardinality'] == FIELD_CARDINALITY_UNLIMITED;

    $element += array(
      '#prefix' => '<div id="' . $wrapper_id . '">',
      '#suffix' => '</div>',
      '#element_validate' => array('page_section_field_widget_page_section_form_validate'),
    );

    if ($multiple) {
      $element += array(
        '#theme' => 'field_multiple_value_form',
        '#field_name' => $field['field_name'],
        '#cardinality' => $field['cardinality'],
      );
    }
    else {
      $element += array(
        '#type' => 'fieldset',
        '#title' => $field['field_name'],
      );
    }

    for ($delta = 0; $delta < $field_state['count_items']; $delta++) {
      $delta_parents = array_merge($field_parents, array(
        $field_name,
        $language,
        $delta,
      ));

      $element[$delta] = array(
        '#parents' => $delta_parents,
        '#delta' => $delta,
        '#required' => $delta == 0 && $instance['required'],
        '#weight' => $delta,
      );

      if ($multiple) {
        $element[$delta]['_weight'] = array(
          '#type' => 'weight',
          '#title' => t('Weight for row @number', array('@number' => $delta + 1)),
          '#title_display' => 'invisible',
           // Note: this 'delta' is the FAPI 'weight' element's property.
          '#delta' => $field_state['count_items'],
          '#default_value' => isset($items[$delta]['_weight']) ? $items[$delta]['_weight'] : $delta,
          '#weight' => 100,
        );
      }

      if (isset($field_state['section'][$delta])) {
        $page_section = $field_state['section'][$delta];
      }
      else {
        $item = !empty($items[$delta]) ? $items[$delta] : array();
        if (!empty($item['section'])) {
          $page_section = $item['section'];
        }
        elseif (!empty($item['sid'])) {
          $page_section = page_section_load($item['sid']);
        }
        else {
          continue;
        }

        // Put our entity in the form state, so FAPI callbacks can access it.
        $field_state['section'][$delta] = $page_section;
      }

      field_attach_form('page_section', $page_section, $element[$delta], $form_state, $language);

      $element[$delta]['remove_button'] = array(
        '#delta' => $delta,
        '#name' => implode('_', $delta_parents) . '_remove_button',
        '#type' => 'submit',
        '#value' => t('Remove !bundle', array('!bundle' => $page_section_types[$element[$delta]['#bundle']]->label)),
        '#validate' => array(),
        '#submit' => array('page_section_remove_submit'),
        '#limit_validation_errors' => array(),
        '#ajax' => array(
          'callback' => 'page_section_remove_js',
          'wrapper' => $wrapper_id,
          'effect' => 'fade',
        ),
        '#weight' => 1000,
      );

      if (empty($element[$delta]['#required'])) {
        $element[$delta]['#after_build'][] = 'page_section_field_widget_delay_required_validation';
      }
    }

    field_form_set_state($field_parents, $field_name, $language, $form_state, $field_state);

    $element['add_more'] = array(
      '#access' => ($field['cardinality'] == FIELD_CARDINALITY_UNLIMITED) || ($field['cardinality'] > $field_state['count_items']),
    );

    foreach ($type_info as $type => $info) {
      if (!in_array($type, $referenceable_types)) {
        continue;
      }

      $element['add_more'][$type] = array(
        '#type' => 'submit',
        '#name' => strtr($id_prefix, '-', '_') . '_' . $type . '_add_more',
        '#value' => t('Add !type', array('!type' => t($info->label))),
        '#page_section_type' => $type,
        '#attributes' => array('class' => array('field-add-more-submit')),
        '#limit_validation_errors' => array(
          array_merge($parents, array($field_name, $langcode)),
        ),
        '#submit' => array('page_section_add_more_submit'),
        '#ajax' => array(
          'callback' => 'page_section_add_more_js',
          'wrapper' => $wrapper_id,
          'effect' => 'fade',
        ),
      );
    }

    $recursion--;
  }

  return $element;
}

function page_section_field_widget_delay_required_validation(&$element, &$form_state) {
  // If the process_input flag is set, the form and its input is going to be
  // validated. Prevent #required (sub)fields from throwing errors while
  // their non-#required page section is empty.
  if ($form_state['process_input']) {
    _page_section_collect_required_elements($element, $element['#page_section_required_elements']);
  }
  return $element;
}

function _page_section_collect_required_elements(&$element, &$required_elements) {
  // Recurse through all children.
  foreach (element_children($element) as $key) {
    if (isset($element[$key]) && $element[$key]) {
      _page_section_collect_required_elements($element[$key], $required_elements);
    }
  }
  if (!empty($element['#required'])) {
    $element['#required'] = FALSE;
    $required_elements[] = &$element;
    $element += array('#pre_render' => array());
    array_unshift($element['#pre_render'], 'page_section_field_widget_render_required');
  }
}

function page_section_field_widget_render_required($element) {
  $element['#required'] = TRUE;
  return $element;
}

/**
 * Form API submit callback for adding more Page Sections.
 */
function page_section_add_more_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];

  $element = drupal_array_get_nested_value($form, array_slice($button['#array_parents'], 0, -2));
  $field_name = $element['#field_name'];
  $langcode = $element['#language'];
  $parents = $element['#field_parents'];

  $form_state['rebuild'] = TRUE;

  // Increment the items count.
  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);
  $field = $field_state['field'];
  if ($field['cardinality'] != FIELD_CARDINALITY_UNLIMITED) {
    if ($field_state['count_items'] >= $field['cardinality']) {
      return;
    }
  }

  $field_state['count_items']++;
  $field_state['section'][$field_state['count_items'] - 1] = page_section_create(array('type' => $button['#page_section_type']));
  field_form_set_state($parents, $field_name, $langcode, $form_state, $field_state);
}

/**
 * Form API Ajax callback for adding more Page Sections.
 */
function page_section_add_more_js($form, $form_state) {
  $button = $form_state['triggering_element'];

  $element = drupal_array_get_nested_value($form, array_slice($button['#array_parents'], 0, -2));
  $field_name = $element['#field_name'];
  $langcode = $element['#language'];
  $parents = $element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);

  $field = $field_state['field'];

  // Add a DIV around the delta receiving the Ajax effect.
  $delta = $field_state['count_items'] - 1;
  $element[$delta]['#prefix'] = '<div class="ajax-new-content">' . (isset($element[$delta]['#prefix']) ? $element[$delta]['#prefix'] : '');
  $element[$delta]['#suffix'] = (isset($element[$delta]['#suffix']) ? $element[$delta]['#suffix'] : '') . '</div>';

  return $element;
}

/**
 * Form API submit callback for removing Page Sections.
 */
function page_section_remove_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];
  $delta = $button['#delta'];

  // Where in the form we'll find the parent element.
  $address = array_slice($button['#array_parents'], 0, -2);
  $parent_element = drupal_array_get_nested_value($form, $address);

  $field_name = $parent_element['#field_name'];
  $langcode = $parent_element['#language'];
  $parents = $parent_element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);
  for ($i = $delta; $i <= $field_state['count_items']; $i++) {
    $old_element_address = array_merge($address, array($i + 1));
    $new_element_address = array_merge($address, array($i));

    $moving_element = drupal_array_get_nested_value($form, $old_element_address);
    $moving_element_value = drupal_array_get_nested_value($form_state['values'], $old_element_address);
    $moving_element_input = drupal_array_get_nested_value($form_state['input'], $old_element_address);

    $moving_element['#parents'] = $new_element_address;
    form_set_value($moving_element, $moving_element_value, $form_state);
    drupal_array_set_nested_value($form_state['input'], $moving_element['#parents'], $moving_element_input);

    if (isset($field_state['section'][$i + 1])) {
      $field_state['section'][$i] = $field_state['section'][$i + 1];
    }
    else {
      unset($field_state['section'][$i]);
    }
  }

  $i = $field_state['count_items'] - 1;
  $removed_element_address = array_merge($address, array($i));
  $removed_element = drupal_array_get_nested_value($form, $removed_element_address);
  form_set_value($removed_element, NULL, $form_state);
  drupal_array_set_nested_value($form_state['input'], $removed_element['#parents'], NULL);

  if (isset($field_state['section'][$i])) {
    unset($field_state['section'][$i]);
  }

  if ($field_state['count_items'] > 0) {
    $field_state['count_items']--;
  }

  $input = drupal_array_get_nested_value($form_state['input'], $address);
  // Sort by weight.
  uasort($input, '_field_sort_items_helper');

  // Reweight everything in the correct order.
  $weight = -1 * $field_state['items_count'];
  foreach ($input as $key => $item) {
    if ($item) {
      $input[$key]['_weight'] = $weight++;
    }
  }
  drupal_array_set_nested_value($form_state['input'], $address, $input);

  field_form_set_state($parents, $field_name, $langcode, $form_state, $field_state);

  $form_state['rebuild'] = TRUE;
}

/**
 * Form API Ajax callback for removing Page Sections.
 */
function page_section_remove_js($form, $form_state) {
  $button = $form_state['triggering_element'];

  $element = drupal_array_get_nested_value($form, array_slice($button['#array_parents'], 0, -2));
  $field_name = $element['#field_name'];
  $langcode = $element['#language'];
  $parents = $element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);

  $field = $field_state['field'];

  return $element;
}

/**
 * Form API element validate callback for a page section entity form embed.
 */
function page_section_field_widget_page_section_form_validate($element, &$form_state, $complete_form) {
  $instance = field_widget_instance($element, $form_state);
  $field = field_widget_field($element, $form_state);
  $field_parents = $element['#field_parents'];
  $field_name = $element['#field_name'];
  $language = $element['#language'];

  $field_state = field_form_get_state($field_parents, $field_name, $language, $form_state);

  for ($delta = 0; $delta < $field_state['count_items']; $delta++) {
    $page_section = $field_state['section'][$delta];

    // Attach field API validation of the embedded form.
    field_attach_form_validate('page_section', $page_section, $element[$delta], $form_state);

    if (!page_section_section_is_empty($page_section) && !empty($element[$delta]['#page_section_required_elements'])) {
      foreach ($element[$delta]['#page_section_required_elements'] as &$elements) {

        if (isset($elements['#needs_validation'])) {
          $is_empty_multiple = (!count($elements['#value']));
          $is_empty_string = (is_string($elements['#value']) && drupal_strlen(trim($elements['#value'])) == 0);
          $is_empty_value = ($elements['#value'] === 0);
          $is_empty_option = (isset($elements['#options']['_none']) && $elements['#value'] == '_none');
          if ($is_empty_multiple || $is_empty_string || $is_empty_value || $is_empty_option) {
            if (isset($elements['#title'])) {
              form_error($elements, t('!name field is required.', array('!name' => $elements['#title'])));
            }
            else {
              form_error($elements);
            }
          }
        }
      }
    }
  }

  // Only if the form is being submitted, finish the collection entity and
  // prepare it for saving.
  $items = array();
  if ($form_state['submitted'] && !form_get_errors()) {
    for ($delta = 0; $delta < $field_state['count_items']; $delta++) {
      $page_section = $field_state['section'][$delta];

      // Attach field API validation of the embedded form.
      field_attach_submit('page_section', $page_section, $element[$delta], $form_state);

      // Load initial form values into $item, so any other form values below the
      // same parents are kept. Fix found in field collection module issue
      // http://drupal.org/node/1545584#comment-6171596
      $items[$delta] = drupal_array_get_nested_value($form_state['values'], $element[$delta]['#parents']);

      if (isset($element[$delta]['_weight']) && ($field['cardinality'] > 1 || $field['cardinality'] == FIELD_CARDINALITY_UNLIMITED)) {
        $items[$delta]['_weight'] = $element[$delta]['_weight']['#value'];
      }

      $items[$delta]['section'] = $page_section;
    }

    form_set_value($element, $items, $form_state);
  }
}
