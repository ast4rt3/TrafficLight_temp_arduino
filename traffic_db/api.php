<?php
// Traffic Controller API - XAMPP Backend
// Place in: C:\xampp\htdocs\traffic_db\api.php

header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') { exit(0); }

$host = 'localhost';
$user = 'root';
$pass = '';
$db   = 'traffic_controller';

$conn = new mysqli($host, $user, $pass, $db);
if ($conn->connect_error) {
    die(json_encode(['error' => 'DB Connection Failed: ' . $conn->connect_error]));
}

$action = $_GET['action'] ?? $_POST['action'] ?? '';

switch ($action) {

    // ==========================================
    // SEQUENCES
    // ==========================================
    case 'list_seqs':
        $result = $conn->query("SELECT id, name, command, created_at, updated_at FROM sequences ORDER BY updated_at DESC");
        $seqs = [];
        while ($row = $result->fetch_assoc()) {
            $seqs[] = $row;
        }
        echo json_encode(['sequences' => $seqs]);
        break;

    case 'get_seq':
        $name = $conn->real_escape_string($_GET['name'] ?? '');
        $result = $conn->query("SELECT * FROM sequences WHERE name = '$name'");
        if ($row = $result->fetch_assoc()) {
            echo json_encode($row);
        } else {
            echo json_encode(['error' => 'Not found']);
        }
        break;

    case 'save_seq':
        $name = $conn->real_escape_string($_REQUEST['name'] ?? '');
        $cmd  = $conn->real_escape_string($_REQUEST['cmd'] ?? '');
        if (empty($name) || empty($cmd)) {
            echo json_encode(['error' => 'Missing name or cmd']);
            break;
        }
        // Upsert: insert or update if exists
        $stmt = $conn->prepare("INSERT INTO sequences (name, command) VALUES (?, ?) ON DUPLICATE KEY UPDATE command = ?, updated_at = CURRENT_TIMESTAMP");
        $stmt->bind_param("sss", $name, $cmd, $cmd);
        $stmt->execute();
        echo json_encode(['status' => 'saved', 'name' => $name]);
        break;

    case 'rename_seq':
        $oldName = $conn->real_escape_string($_REQUEST['old_name'] ?? '');
        $newName = $conn->real_escape_string($_REQUEST['new_name'] ?? '');
        if (empty($oldName) || empty($newName)) {
            echo json_encode(['error' => 'Missing old_name or new_name']);
            break;
        }
        $stmt = $conn->prepare("UPDATE sequences SET name = ? WHERE name = ?");
        $stmt->bind_param("ss", $newName, $oldName);
        $stmt->execute();
        echo json_encode(['status' => $stmt->affected_rows > 0 ? 'renamed' : 'not_found']);
        break;

    case 'delete_seq':
        $name = $conn->real_escape_string($_REQUEST['name'] ?? '');
        $conn->query("DELETE FROM sequences WHERE name = '$name'");
        echo json_encode(['status' => 'deleted']);
        break;

    // ==========================================
    // SETTINGS
    // ==========================================
    case 'get_settings':
        $result = $conn->query("SELECT setting_key, setting_value, updated_at FROM settings");
        $settings = [];
        while ($row = $result->fetch_assoc()) {
            $settings[$row['setting_key']] = [
                'value' => $row['setting_value'],
                'updated_at' => $row['updated_at']
            ];
        }
        echo json_encode(['settings' => $settings]);
        break;

    case 'save_setting':
        $key   = $conn->real_escape_string($_REQUEST['key'] ?? '');
        $value = $conn->real_escape_string($_REQUEST['value'] ?? '');
        if (empty($key)) {
            echo json_encode(['error' => 'Missing key']);
            break;
        }
        $stmt = $conn->prepare("INSERT INTO settings (setting_key, setting_value) VALUES (?, ?) ON DUPLICATE KEY UPDATE setting_value = ?, updated_at = CURRENT_TIMESTAMP");
        $stmt->bind_param("sss", $key, $value, $value);
        $stmt->execute();
        echo json_encode(['status' => 'saved', 'key' => $key]);
        break;

    // ==========================================
    // BULK SYNC (ESP32 -> XAMPP)
    // ==========================================
    case 'sync_from_esp':
        // Receives JSON body with all ESP32 data and upserts it
        $data = json_decode(file_get_contents('php://input'), true);
        if (!$data) {
            echo json_encode(['error' => 'Invalid JSON']);
            break;
        }
        $synced = 0;
        // Sync sequences
        if (isset($data['sequences'])) {
            foreach ($data['sequences'] as $seq) {
                $stmt = $conn->prepare("INSERT INTO sequences (name, command) VALUES (?, ?) ON DUPLICATE KEY UPDATE command = ?, updated_at = CURRENT_TIMESTAMP");
                $stmt->bind_param("sss", $seq['name'], $seq['cmd'], $seq['cmd']);
                $stmt->execute();
                $synced++;
            }
        }
        // Sync settings
        if (isset($data['settings'])) {
            foreach ($data['settings'] as $key => $value) {
                $stmt = $conn->prepare("INSERT INTO settings (setting_key, setting_value) VALUES (?, ?) ON DUPLICATE KEY UPDATE setting_value = ?, updated_at = CURRENT_TIMESTAMP");
                $stmt->bind_param("sss", $key, $value, $value);
                $stmt->execute();
                $synced++;
            }
        }
        echo json_encode(['status' => 'synced', 'count' => $synced]);
        break;

    default:
        echo json_encode(['error' => 'Unknown action', 'available' => [
            'list_seqs', 'get_seq', 'save_seq', 'rename_seq', 'delete_seq',
            'get_settings', 'save_setting', 'sync_from_esp'
        ]]);
}

$conn->close();
?>
