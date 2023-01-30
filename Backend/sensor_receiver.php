<?php
/** 
 * @file sensor_receiver.php
 * @author Tobias Poppe
 * @brief Backend-Receiver-Script
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 * 
 * Receives a POST from Basemodules,
 * checks the Database for Config-Values,
 * calculates waterlevel,
 * builds a JSON and send it via MQTT to the frontend
 */

include 'db_con.php'; //Include MySQL credentials and connect to DB
require __DIR__ . '/vendor/autoload.php'; //Include MQTT-Client
use PhpMqtt\Client\MqttClient;

$frontend_url = 'https://**************'; //ThingsBoard URL
$received_uuid = $conn->real_escape_string(json_encode($_GET));
$received_data = $conn->real_escape_string(file_get_contents('php://input'));

//Check for UUID in Config-DB and load config-Values if true
$sql = "SELECT * FROM `sensor_config` WHERE `sensor_id` LIKE '" . $received_uuid . "'";
$sql_result = mysqli_query($conn, $sql);
if (mysqli_num_rows($sql_result) != 0) {
    $config = mysqli_fetch_array($config_erg, MYSQLI_ASSOC);
    $result_array = json_decode($received_data, TRUE);
    $received_timestamp = strtotime($result_array['last_timestamp']) . "000";

    //Calc values and build MQTT-JSON
    $mqtt_data_array = array();
    $mqtt_data_array['ts'] = $received_timestamp;
    $mqtt_data_array['values'] = array();
    $mqtt_data_array['values']['bat_spannung'] = $received_post['intern']['bat_spannung'];
    $mqtt_data_array['values']['lufttemperatur'] = $received_post['intern']['lufttemperatur'];
    $mqtt_data_array['values']['luftdruck'] = $received_post['intern']['luftdruck'];
    $mqtt_data_array['values']['luftfeuchtigkeit'] = $received_post['intern']['luftfeuchtigkeit'];
    $mqtt_data_array['values']['carrier'] = $received_post['intern']['carrier'];
    $mqtt_data_array['values']['mobile_reception'] = $received_post['intern']['mobile_reception'];

    if (isset($received_post['MS5837']['wasserdruck'])) {
        $mqtt_data_array['values']['MS5837_wassertemperatur'] = $received_post['MS5837']['wassertemperatur'];
        $mqtt_data_array['values']['MS5837_wasserdruck'] = $received_post['MS5837']['wasserdruck'] / 10;
        $mqtt_data_array['values']['MS5837_druckdifferenz'] = $mqtt_data_array['values']['MS5837_wasserdruck'] - $mqtt_data_array['values']['luftdruck'];
        $mqtt_data_array['values']["MS5837_pegel"] = (($mqtt_data_array['values']['MS5837_druckdifferenz'] * 10) + $config['offset']) / 1000;
    }
    if (isset($received_post['LPS28DFW']['wasserdruck'])) {
        $mqtt_data_array['values']['LPS28DFW_wassertemperatur'] = $received_post['LPS28DFW']['wassertemperatur'];
        $mqtt_data_array['values']['LPS28DFW_wasserdruck'] = $received_post['LPS28DFW']['wasserdruck'];
        $mqtt_data_array['values']['LPS28DFW_druckdifferenz'] = $mqtt_data_array['values']['LPS28DFW_wasserdruck'] - $mqtt_data_array['values']['luftdruck'];
        $mqtt_data_array['values']["LPS28DFW_pegel"] = (($mqtt_data_array['values']['LPS28DFW_druckdifferenz'] * 10) + $config['offset']) / 1000;
    }
    $mqtt_data_array['values']["lat"] = $config['lat'];
    $mqtt_data_array['values']["lon"] = $config['lon'];
    $mqtt_data_json = json_encode($mqtt_data_array);

    //Save received and calculated Data into DB
    $sql = "INSERT INTO `post_from_sensor` (`id`, `timestamp`, `uuid`, `data`) 
    VALUES 
    (NULL, '" . $received_timestamp . "', '" . $received_uuid . "', '" . $mqtt_data_json . "', '')";
    mysqli_query($conn, $sql);
    //Send Data via MQTT to ThingsBoard
    $mqtt->connect($connectionSettings, true);
    $mqtt->publish('v1/devices/me/telemetry', $mqtt_data_json, 0);
    $mqtt->disconnect();
} else {
    echo "Message not accepted, unknown UUID!";
}
?>