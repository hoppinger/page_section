<?php

/**
 * @file
 * Provides the Devel Generate integration for Page Section.
 */

/**
 * Implements hook_devel_generate().
 */
function page_section_devel_generate($object, $field, $instance, $bundle) {
  if (field_behaviors_widget('multiple values', $instance) == FIELD_BEHAVIOR_CUSTOM) {
    $object_field = array();

    switch ($field['cardinality']) {
      case FIELD_CARDINALITY_UNLIMITED:
        // Just an arbitrary number for 'unlimited'.
        $max = rand(3, 8);
        break;

      default:
        $max = $field['cardinality'] - 1;
        break;
    }
    for ($i = 0; $i <= $max; $i++) {
      $result = _page_section_devel_generate($object, $field, $instance, $bundle);
      if (!empty($result)) {
        $object_field[$i] = $result;
      }
    }

    return $object_field;
  }
  else {
    return _page_section_devel_generate($object, $field, $instance, $bundle);
  }
}

/**
 * Internal function to generate a single page section and field item.
 */
function _page_section_devel_generate($object, $field, $instance, $bundle) {
  $types = page_section_type_info();
  if (empty($types)) {
    return NULL;
  }

  $type = array_rand($types);
  $page_section = page_section_create(array('type' => $type));
  devel_generate_fields($page_section, 'page_section', $type);
  page_section_save($page_section);

  return array('sid' => $page_section->sid);
}
