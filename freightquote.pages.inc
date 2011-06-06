<?php

/**
 * Settings page callback.
 */
function freightquote_settings_form_page_callback() {
  return drupal_get_form('freightquote_settings_form');
}

/**
 * FreightQuote settings form
 */
function freightquote_settings_form() {
  $form['freightquote_username'] = array(
    '#type' => 'textfield',
    '#required' => TRUE,
    '#title' => 'FreightQuote username',
    '#default_value' => variable_get('freightquote_username', 'xmltest@freightquote.com'),
  );

  $form['freightquote_password'] = array(
    '#type' => 'password',
    '#title' => 'FreightQuote password',
    '#default_value' => '',
    '#description' => 'Leave blank to keep the old value.',
  );
  
  $form['freightquote_HazardousMaterialContactName'] = array(
    '#type' => 'textfield',
    '#required' => TRUE,
    '#title' => 'The name of the hazardous materials contact',
    '#description' => 'This parameter is required by freightquote service.',
    '#default_value' => variable_get('freightquote_HazardousMaterialContactName', ''),
  );

  $form['freightquote_HazardousMaterialContactPhone'] = array(
    '#type' => 'textfield',
    '#required' => TRUE,
    '#title' => 'The phone number for the hazardous materials contact',
    '#description' => 'This parameter is required by freightquote service.',
    '#default_value' => variable_get('freightquote_HazardousMaterialContactPhone', ''),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Submit handler for settings form.
 */
function freightquote_settings_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  
  $keys[] = 'freightquote_username';
  $keys[] = 'freightquote_HazardousMaterialContactName';
  $keys[] = 'freightquote_HazardousMaterialContactPhone';
  foreach ($keys as $key) {
    variable_set($key, $values[$key]);
  }

  if (!empty($values['freightquote_password'])) {
    variable_set('freightquote_password', $values['freightquote_password']);
  }
}

/**
 * FreightQuote WSDL functions & types.
 */
function freightquote_wsdl_page_callback() {
  $wsdl = new SoapClient(FREIGHTQUOTE_SERVICE_URL . '?wsdl');

  $strings = '[\.a-zA-Z0-9_]+';

  // Format functions
  if (is_object($wsdl)) {
    foreach ($wsdl->__getFunctions() as $function) {
      $find = "/^($strings)\s+($strings)\(($strings) (.+)\)$/";
      $replace = '<a name="function-$2"></a>';
      $replace .= '<a href="#struct-$1" style="color: #333;">$1</a> ';
      $replace .= '<a href="#function-$2">$2</a> (<br />';
      $replace .= '&nbsp;&nbsp;';
      $replace .= '<em><a href="#struct-$3">$3</a></em> ';
      $replace .= '<span style="color: red;">$4</span><br />)';
      $functions[] = preg_replace($find, $replace, $function);
    }

    // Format structs
    foreach ($wsdl->__getTypes() as $struct) {
      $find = "/^\s($strings) ($strings);.*$/m";
      $replace = '<tr>';
      $replace .= '<td width="40%"><a href="#struct-$1"><em>$1</em></a></td>';
      $replace .= '<td><span style="color: red;">$2</span>;</td>';
      $replace .= '</tr>';
      $struct = preg_replace($find, $replace, $struct);

      $find = "/^struct ($strings) \{([.\s\t\S]+)\}$/m";
      $replace = '<a name="struct-$1"></a>';
      $replace .= 'struct <a href="#struct-$1">$1</a> {';
      $replace .= '<table style="margin-left: 15px; width: 95%;">$2</table>';
      $replace .= '}';
      $structs[] = preg_replace($find, $replace, $struct);
    }

    $rows[] = array(
      'functions' => array('data' => theme('item_list', $functions), 'valign' => 'top'),
      'types'     => array('data' => theme('item_list', $structs), 'valign' => 'top'),
    );
  }
 
  $header[] = array('data' => 'Functions', 'width' => '50%');
  $header[] = array('data' => 'Data Types', 'width' => '50%');
  
  drupal_add_js(drupal_get_path('module', 'collab_soap') . '/js/collab_soap.wdsl.js');

  return theme('table', $header, $rows);
}

/**
 * Helper function to get FreightQuote wsdl info.
 *
 * @param string $type function/type
 * @param string $name name of function/type
 * @return mixed
 */
function _freightquote_wsdl_info($type = NULL, $name = NULL) {
  $client = new SoapClient(FREIGHTQUOTE_SERVICE_URL . '?wsdl');

  $wsdl = array(
    'functions' => $client->__getFunctions(),
    'types' => $client->__getTypes(),
  );

  if (!is_null($type) && !is_null($name)) {
    $type = $type == 'function' ? 'functions' : 'types';
    foreach ($wsdl[$type] as $info) {
      $return = $type === 'function' && strpos($info, $name) === 0;
      $return = $return || ($type === 'types' && strpos("struct {$info}", $name) === 0);

      if ($return) {
        return $wsdl[$type];
      }
    }
    
    return false;
  }

  return $wsdl;
}

function _freightquote_product_form(&$form, $form_state) {
  $node = &$form['#node'];

  $form['shipping']['freightquote'] = array(
    '#type' => 'fieldset',
    '#title' => 'Freightquote',
    '#collapsible' => TRUE,
    '#tree' => TRUE,
  );
  
  $form['shipping']['freightquote']['class'] = array(
    '#type' => 'select',
    '#title' => t('Class'),
    '#options' => freightquote_classes(),
    '#default_value' => isset($node->freightquote['class']) ? $node->freightquote['class'] : NULL,
    '#description' => 'Freight class is recommended for the most accurate rate.
        If you don\'t know the class of the freight, please call customer service
        at 800.323.5441',
  );

  $form['shipping']['freightquote']['package_type'] = array(
    '#type' => 'select',
    '#title' => t('Package type'),
    '#options' => freightquote_package_types(),
    '#default_value' => isset($node->freightquote['package_type']) ? $node->freightquote['package_type'] : NULL,
  );
  
  $form['shipping']['freightquote']['commodity_type'] = array(
    '#type' => 'select',
    '#title' => t('Commodity Type'),
    '#options' => freightquote_commodity_types(),
    '#default_value' => isset($node->freightquote['commodity_type']) ? $node->freightquote['commodity_type'] : NULL,
  );

  $form['shipping']['freightquote']['content_type'] = array(
    '#type' => 'select',
    '#title' => t('Content Type'),
    '#options' => freightquote_content_types(),
    '#default_value' => isset($node->freightquote['content_type']) ? $node->freightquote['content_type'] : NULL,
  );
}
