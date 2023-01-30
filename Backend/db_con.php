<?php
$servername = "localhost";
$username = "SECRET_USERNAME";
$password = "SECRET_PASSWORD";
$dbname = "open_wales";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
   die();
}
?>