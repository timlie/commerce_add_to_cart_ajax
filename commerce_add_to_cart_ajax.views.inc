<?php

/**
 * Implements hook_views_data_alter().
 */
function commerce_add_to_cart_ajax_views_data_alter(&$data) {
  $data['commerce_product']['add_to_cart_quantity_ajax'] = array(
    'title' => t('AJAX Quantity input field'),
    'help' => t('Adds a quantity input field which will autoupdate the cart using AJAX'),
    'real field' => 'product_id',
    'field' => array(
      'handler' => 'commerce_add_to_cart_ajax_handler_field_quantity',
    ),
  );
}