<?php
require_once 'db.php';

header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    exit(0);
}

// Get parameters
$uid = isset($_POST['uid']) ? trim($_POST['uid']) : '';
$name = isset($_POST['name']) ? trim($_POST['name']) : '';
$role = isset($_POST['role']) ? trim($_POST['role']) : '';
$phone = isset($_POST['phone']) ? trim($_POST['phone']) : '';

if (empty($uid)) {
    $input = json_decode(file_get_contents('php://input'), true);
    $uid = isset($input['uid']) ? trim($input['uid']) : '';
    $name = isset($input['name']) ? trim($input['name']) : '';
    $role = isset($input['role']) ? trim($input['role']) : '';
    $phone = isset($input['phone']) ? trim($input['phone']) : '';
}

if (empty($uid) || empty($name)) {
    echo json_encode(['status' => 'error', 'message' => 'UID and Name are required.']);
    exit;
}

$uid = strtoupper($uid);

try {
    // Check if user already exists
    $stmt = $pdo->prepare("SELECT id FROM rfid_users WHERE uid = ?");
    $stmt->execute([$uid]);
    $existing = $stmt->fetch();

    if ($existing) {
        // Update user
        $updateStmt = $pdo->prepare("UPDATE rfid_users SET name = ?, role = ?, phone = ? WHERE uid = ?");
        $updateStmt->execute([$name, empty($role) ? '임직원' : $role, empty($phone) ? null : $phone, $uid]);
        echo json_encode(['status' => 'success', 'message' => 'User updated successfully.']);
    } else {
        // Insert new user
        $insertStmt = $pdo->prepare("INSERT INTO rfid_users (uid, name, role, phone) VALUES (?, ?, ?, ?)");
        $insertStmt->execute([$uid, $name, empty($role) ? '임직원' : $role, empty($phone) ? null : $phone]);
        echo json_encode(['status' => 'success', 'message' => 'User registered successfully.']);
    }
} catch (Exception $e) {
    echo json_encode(['status' => 'error', 'message' => 'Database error: ' . $e->getMessage()]);
}
