<?php

/**
 * @file Helper function for freightquote
 */

function freightquote_quote($origin, $destination, $products) {
  $Origin = __freighquote_location_origin($origin);
  $Destination = __freighquote_location_destination($destination);

  $params['request'] = array(
    'CustomerId' => 0,
    'QuoteType' => 'B2B',
    'ServiceType' => 'LTL',
    'QuoteShipment' => array(
      'IsBlind' => FALSE,
      'HazardousMaterialContactName' => 'HazardousMaterialContactName',
      'HazardousMaterialContactPhone' => '555-555-5555',
      'PickupDate' => __freightquote_pickup_date(),
      'SortAndSegregate' => FALSE,
      'UseStackableFlag' => FALSE,
      'ShipmentLocations' => array('Location' => array($Origin, $Destination)),
      'ShipmentProducts' => array('Product' => __freightquote_products($products)),
      'ShipmentContacts' => array('ContactAddress' => array()),
      ));

  $params['user'] = array(
    'Name' => variable_get('freightquote_username', 'xmltest@freightquote.com'),
    'Password' => variable_get('freightquote_password', 'xml')
  );

  $options = array();
  #$options['proxy_host'] = '127.0.0.1';
  #$options['proxy_port'] = '8888';
  $client = new SoapClient(FREIGHTQUOTE_SERVICE_URL . '?wsdl', $options);
  $response = $client->__soapCall('GetRatingEngineQuote', array($params));

  if (isset($response->GetRatingEngineQuoteResult)) {
    $response = $response->GetRatingEngineQuoteResult;

    if (isset($response->QuoteCarrierOptions->CarrierOption)) {
      $quotes = array();
      $quote_id = $response->QuoteId;
      $options = $response->QuoteCarrierOptions->CarrierOption;
      foreach ($options as $option) {
        $option_id = $option->CarrierOptionId;
        $id = "{$quote_id}-{$option_id}";
        $accessorials[] = $option->CarrierName;
        $quotes[] = array(
          'option_label' => sprintf('FreightQuote » %s', $option->CarrierName),
          'rate' => $option->QuoteAmount,
          'notes' => $option->IsGuaranteed ? 'Guaranteed: Yes' : '',
        );
      }

      $_SESSION['freightquote_accessorials'] = $accessorials;

      return $quotes;
    }

    if (isset($response->ValidationErrors->B2BError)) {
      foreach (__freightquote_array($response->ValidationErrors->B2BError) as $error) {
        $notes[] = "{$error->ErrorType}: {$error->ErrorMessage}";
      }

      $quote = array(
        'option_label' => 'FreightQuote Error',
        'notes' => theme('item_list', $notes),
      );

      return array($quote);
    }
  }

  return FALSE;
}

function __freightquote_products($products) {
  $Products = array();
  foreach ($products as $product) {
    if ($product->nid && isset($product->freightquote)) {
      $freightquote = $product->freightquote;

      $Products[] = array(
        'Class' => (int)$freightquote['class'],
        'Weight' => $product->weight,
        'Length' => $product->length,
        'Width' => $product->width,
        'Height' => $product->height,
        'ProductDescription' => $product->title,
        'PackageType' => $freightquote['package_type'],
        'IsStackable' => TRUE,
        'DeclaredValue' => 0.0,
        'CommodityType' => $freightquote['commodity_type'],
        'ContentType' => $freightquote['content_type'],
        'IsHazardousMaterial' => FALSE,
        'DimWeight' => 0,
        'EstimatedWeight' => $product->weight,
        'PieceCount' => $product->pkg_qty,
        'ItemNumber' => 0,
        'STC' => 0);
    }
  }
  
  return $Products;
}

function __freightquote_pickup_date() {
  if (isset($_POST['freightquote_pickup'])) {
    $date = $_POST['freightquote_pickup'];
    $_SESSION['freightquote_pickup'] = $_POST['freightquote_pickup'];
  }
  else {
    if (isset($_SESSION['freightquote_pickup'])) {
      $date = $_SESSION['freightquote_pickup'];
    }
    else {
      $date = date('Y-m-d');
    }
  }

  list($month, $day, $year) = explode('-', $date);
  $PickupDate = new DateTime();
  $PickupDate->setDate($year, $month, $day);
  $PickupDate = $PickupDate->format('Y-m-d\TH:i:s');
  return $PickupDate;
}

function __freighquote_location_destination($destination = NULL) {
  if (!empty($destination)) {
    $_SESSION['freightquote_destination'] = $destination;
  }
  else {
    $destination = $_SESSION['freightquote_destination'];
  }

  return array(
    'LocationType' => 'Destination',
    'RequiresArrivalNotification' => FALSE,
    'HasLoadingDock' => FALSE,
    'IsConstructionSite' => FALSE,
    'RequiresInsideDelivery' => FALSE,
    'IsTradeShow' => FALSE,
    'IsResidential' => FALSE,
    'RequiresLiftgate' => FALSE,
    'ContactName' => "{$destination['first_name']} {$destination['last_name']}",
    'ContactPhone' => $destination['phone'],
    'NotificationMethod' => 'None',
    'HasDeliveryAppointment' => FALSE,
    'IsLimitedAccess' => FALSE,
    'LocationAddress' => array(
      'StreetAddress' => $destination['street1'],
      'AdditionalAddress' => $destination['street2'],
      'City' => $destination['city'],
      'PostalCode' => $destination['postal_code'],
      'CountryCode' => 'US'),
    'AdditionalServices' => array());
}

function __freighquote_location_origin($origin) {
  return array(
    'LocationType' => 'Origin',
    'RequiresArrivalNotification' => FALSE,
    'HasLoadingDock' => FALSE,
    'IsConstructionSite' => FALSE,
    'RequiresInsideDelivery' => FALSE,
    'IsTradeShow' => FALSE,
    'IsResidential' => FALSE,
    'RequiresLiftgate' => FALSE,
    'ContactName' => "{$origin->first_name} {$origin->last_name}",
    'ContactPhone' => $origin->phone,
    'NotificationMethod' => 'None',
    'HasDeliveryAppointment' => FALSE,
    'IsLimitedAccess' => FALSE,
    'LocationAddress' => array(
      'AddressName' => $origin->company,
      'StreetAddress' => $origin->street1,
      'AdditionalAddress' => $origin->street2,
      'City' => $origin->city,
      'PostalCode' => $origin->postal_code,
      'CountryCode' => 'US'),
    'AdditionalServices' => array());
}

function __freightquote_array($data) {
  if (!is_array($data)) {
    return array($data);
  }
  
  return $data;
}
