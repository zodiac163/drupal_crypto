<?php

function custom_it_cryptonator_admin_settings_form($form, &$form_state) {
	$form = array(); 
	
	$form['submit_button_1'] = array (
		'#type' => 'submit',
		'#value' => t('Привязать карту'),
		'#submit' => array('custom_it_cryptonator_card_bind'),
	);
	
	$form['submit_button_2'] = array (
		'#type' => 'submit',
		'#value' => t('Протестировать безакцептный платеж'),
		'#submit' => array('custom_it_cryptonator_direct_payment'),
	);
	
	return $form;
}

//Привязка карты для безакцептных платежей
function custom_it_cryptonator_card_bind($form, &$form_state) {
	//Создаем программно заказ
	$order = custom_it_cryptonator_fake_order_create();
	//echo "<pre>"; var_dump($order); exit;
	$payment = commerce_payment_method_instance_load('commerce_tinkoff|commerce_payment_commerce_tinkoff');	
	$arrFields = custom_it_cryptonator_create_order_data($order);
	
	$Tinkoff = new TinkoffMerchantAPI( $payment["settings"]["merchant_id"], $payment["settings"]["secret_key"], $payment["settings"]["lifetime"] );
	//Привязка клиента
	$arrFields2 = array(
		'CustomerKey' => variable_get('site_mail', ''),
	);
	$request = $Tinkoff->buildQuery('AddCustomer ', $arrFields2);
	$request = json_decode($request);
	echo "<pre>"; var_dump($request); exit;
	//Запрос счета
	$request = $Tinkoff->buildQuery('Init', $arrFields);
    $request = json_decode($request);
	//echo "<pre>"; var_dump($request); exit;
	$form['#method'] = 'GET';
    // Create Form to post to Tinkoff
    if(isset($request->PaymentURL)){
		$order = commerce_order_status_update($order, 'checkout_payment');
		drupal_set_message(l($request->PaymentURL,$request->PaymentURL));
    }
    else{
		drupal_set_message($request->Message . ' '. $request->Details, 'error');        
    }
	
	return $form;
}

function custom_it_cryptonator_fake_order_create() {
	global $user;
	$product_id = 20508;
	
	$order = commerce_order_new($user->uid, 'checkout_checkout');
	commerce_order_save($order);
	$product = commerce_product_load($product_id);
	$line_item = commerce_product_line_item_new($product, 1, $order->order_id);

	// Save the line item to get its ID.
	commerce_line_item_save($line_item);

	// Add the line item to the order using fago's rockin' wrapper.
	$order_wrapper = entity_metadata_wrapper('commerce_order', $order);
	$order_wrapper->commerce_line_items[] = $line_item;
	
	$order->bind_card = 'bind';
	// Save the order again to update its line item reference field.
	commerce_order_save($order);
	
	return $order;
}

function custom_it_cryptonator_create_order_data($order, $price = 100, $recurrent = 'Y') {
	$Receipt = array();
	$Receipt['Email'] = variable_get('site_mail', '');
	$Receipt['Taxation'] = 'usn_income_outcome';
	
	$Items = array();	
	//Номер заказа отличный от других
	$orderId = drupal_random_key(12);
	$Items[] = array(
		'Name' => 'Привязка карты',
		'Price' => $price,
		'Quantity' => 1,
		'Amount' => 100,
		'Tax' => 'none',
	);
	
	$Receipt['Items'] = $Items;
	
	$arrFields = array(
        'OrderId'			=> $order->order_id,
        'Amount'			=> $price,
        'Description'		=> $_SERVER['HTTP_HOST']." # ".$order->order_id,
		'Recurrent'			=> $recurrent,
		'Receipt' => $Receipt,
		'CustomerKey'       => variable_get('site_mail', ''),
    );
	
	return $arrFields;
}

function custom_it_cryptonator_direct_payment() {
	//Создаем программно заказ
	$order = custom_it_cryptonator_fake_order_create();	
	$payment = commerce_payment_method_instance_load('commerce_tinkoff|commerce_payment_commerce_tinkoff');	
	$paymentCrypto = commerce_payment_method_instance_load('cryptonator|commerce_payment_commerce_payment_cryptonator');		
	$arrFields = custom_it_cryptonator_create_order_data($order, 200, 'N');
	//Создаем платежный метод и отправляем Init
	$Tinkoff = new TinkoffMerchantAPI( $payment["settings"]["merchant_id"], $payment["settings"]["secret_key"], $payment["settings"]["lifetime"] );
	$request = $Tinkoff->buildQuery('Init', $arrFields);
    $request = json_decode($request);
	echo "<pre>"; var_dump($request);
	$arrFields = array(
		'PaymentId' => $request->PaymentId,
		'RebillId' => $paymentCrypto["settings"]["rebillid"],
	); echo "<pre>"; var_dump($arrFields);
	//Отправляем второй запрос Charge
	$request = $Tinkoff->buildQuery('Charge ', $arrFields);
	$request = json_decode($request);
	echo "<pre>"; var_dump($request); exit;
	
	$form['#method'] = 'GET';
    // Create Form to post to Tinkoff
    if(isset($request->PaymentURL)){
		$order = commerce_order_status_update($order, 'checkout_payment');
		drupal_set_message(l($request->PaymentURL,$request->PaymentURL));
    }
    else{
		drupal_set_message($request->Message . ' '. $request->Details, 'error');        
    }
	
	return $form;
}