<?php

/**
* @file
* Views field handler. Adds the quantity field to the view.
* Implements the Views Form API.
*/

class commerce_add_to_cart_ajax_handler_field_quantity extends views_handler_field {
  function render($values) {
    return '<!--form-item-' . $this->options['id'] . '--' . $this->view->row_index . '-->';
  }

  function option_definition() {
    $options = parent::option_definition();

    $options['display_path'] = array('default' => FALSE);

    return $options;
  }

  /**
   * Provide the add to cart display options.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['display_path'] = array(
      '#type' => 'checkbox',
      '#title' => t("Link products added to the cart from this View to this View's path if displayed as a page."),
      '#default_value' => $this->options['display_path'],
    );
  }

  /**
   * The form which replaces the placeholder from render().
   */
  function views_form(&$form, &$form_state) {
    // The view is empty, abort.
    if (empty($this->view->result)) {
      return;
    }

    $form['#prefix'] = '<div id="form-wrapper">';
    $form['#suffix'] = '</div>';

    $form['ajax-messages'] = array(
      '#markup' => '<div id="ajax-messages"></div>'
    );

    $form[$this->options['id']] = array(
      '#tree' => TRUE,
    );

    // Get the cart object so we can correctly default the items
    global $user;

    $cart = commerce_cart_order_load($user->uid);
    $cart_items = array();
    if(!empty($cart)){
      $cart_wrapper = entity_metadata_wrapper("commerce_order", $cart);
      
      foreach ($cart_wrapper->commerce_line_items as $delta => $matching_line_item_wrapper) {
        $cart_item_product = $matching_line_item_wrapper->commerce_product->product_id->value();
        $cart_item_quantity = (int)$matching_line_item_wrapper->quantity->value();

        $cart_items[] = array(
            'product_id' => $cart_item_product,
            'quantity' => $cart_item_quantity,
          );
      }
    }

    // At this point, the query has already been run, so we can access the results
    // in order to get the product_id.
    foreach ($this->view->result as $row_index => $row) {
      $product_id = $this->get_value($row);
      //check the cart items to see if any match the current 
      $matched_line_item_quantity = 0;
      foreach ($cart_items as $delta => $line_item) {
        if ($line_item['product_id'] == $product_id) {
          $matched_line_item_quantity = $line_item['quantity'];
        }
      }

      if($matched_line_item_quantity){
        $default_value = $matched_line_item_quantity;
      }else{
        $default_value = 0;
      }
      
      $form[$this->options['id']][$row_index] = array(
        '#type' => 'textfield',
        '#product_id' => $product_id,
        '#field_name' => $this->options['id'],
        '#row_num' => $row_index,
        '#default_value' => $default_value,
        '#size' => 5,
        '#ajax' => array(
          'callback' => 'commerce_add_to_cart_ajax_set_cart_quantity',
          'wrapper' => 'ajax-messages',
          'effect' => 'fade',
          'event' => 'change',
          'validation' => 'commerce_add_to_cart_ajax_set_cart_quantity_validate',
          'progress' => array('type' => 'throbber', 'message' => t('Toevoegen aan winkelwagen')),
          ),
      );
    }
  }


  function views_form_validate($form, &$form_state) {
    $field_name = $this->options['id'];
    foreach (element_children($form[$field_name]) as $row_id) {
      // Ensure the quantity is actually a numeric value.
      if (!is_numeric($form_state['values'][$field_name][$row_id]) || $form_state['values'][$field_name][$row_id] < 0) {
        form_set_error($field_name . '][' . $row_id, t('You must specify a positive number for the quantity'));
      }

      // If the custom data type attribute of the quantity element is integer,
      // ensure we only accept whole number values.
      if ((int) $form_state['values'][$field_name][$row_id] != $form_state['values'][$field_name][$row_id]) {
        form_set_error($field_name . '][' . $row_id, t('You must specify a whole number for the quantity.'));
      }

      // Ensure the quantity is actually not empty.
      if (empty($form_state['values'][$field_name][$row_id]) || $form_state['values'][$field_name][$row_id] == "") {
        form_set_error($field_name . '][' . $row_id, t('Cannot be empty.'));
      }
    }
  }

  function views_form_submit($form, &$form_state) {
    global $user;
    
    $field_name = $this->options['id'];
    $product_quantities = array();
    foreach (element_children($form[$field_name]) as $row_id) {
      // Don't add products that have quantity 0.
      $quantity = $form_state['values'][$field_name][$row_id];
      if ($quantity > 0) {
        $product_id = $form[$field_name][$row_id]['#product_id'];
        $product_quantities[$product_id] = $quantity;
      }
    }

    $products = commerce_product_load_multiple(array_keys($product_quantities));
    foreach ($products as $product_id => $product) {
      $quantity = $product_quantities[$product_id];
      $line_item = commerce_product_line_item_new($product, $quantity);
      $line_item->data['context']['product_ids'] = array($product_id);
      $line_item->data['context']['add_to_cart_combine'] = $this->options['combine'];

      // Add the display path to the line item's context data array if specified.
      if ($this->view->display[$this->view->current_display]->display_plugin == 'page' &&
        $this->options['display_path']) {
        $line_item->data['context']['display_path'] = $this->view->display[$this->view->current_display]->display_options['path'];
      }

      // Store the View data in the context data array as well.
      $line_item->data['context']['view'] = array(
        'view_name' => $this->view->name,
        'display_name' => $this->view->current_display,
        'human_name' => $this->view->human_name,
        'page' => $this->view->current_page,
      );

      commerce_add_to_cart_ajax_product_add($user->uid, $line_item, TRUE);
    }

    if (count($product_quantities)) {
      drupal_set_message(t('The selected products have been added to <a href="!cart-url">your cart</a>.', array('!cart-url' => url('cart'))));
    }
  }

}