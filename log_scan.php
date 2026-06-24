<?php
require_once 'db.php';

header('Content-Type: application/json');

// Handle both standard POST and JSON POST
$uid = isset($_POST['uid']) ? trim($_POST['uid']) : '';

if (empty($uid)) {
    $input = json_decode(file_get_contents('php://input'), true);
    if (isset($input['uid'])) {
        $uid = trim($input['uid']);
    }
}

if (empty($uid)) {
    echo json_encode(['status' => 'error', 'message' => 'Missing UID parameter']);
    exit;
}

// Ensure UID format is uppercase (standardize)
$uid = strtoupper($uid);

try {
    // 1. Check if user is registered
    $stmt = $pdo->prepare("SELECT name, role FROM rfid_users WHERE uid = ?");
    $stmt->execute([$uid]);
    $user = $stmt->fetch();

    $allowed = false;
    $name = '미등록 사용자';
    $role = 'Unknown';

    if ($user) {
        $allowed = true;
        $name = $user['name'];
        $role = $user['role'];
    }

    // 2. Insert into access logs
    // Note: Since we have a foreign key on rfid_logs.uid referencing rfid_users.uid,
    // if a user is unregistered, we cannot insert their unregistered UID directly into rfid_logs if it strictly enforces FK constraint.
    // Wait! Let's check schema.sql.
    // It has: "FOREIGN KEY (uid) REFERENCES rfid_users(uid) ON DELETE CASCADE".
    // If the UID is not in rfid_users, inserting it into rfid_logs WILL FAIL due to the Foreign Key constraint!
    // Oh! That is a critical catch!
    // If a guest/unknown card is scanned, it will not be in rfid_users, so the insert into rfid_logs will fail.
    // To solve this, we should either:
    // Option A: Allow rfid_logs.uid to reference a nullable foreign key, but if they are unregistered they don't have a record in rfid_users.
    // Option B: Remove the Foreign Key constraint so we can log unregistered cards too (recommended for security audits, since we want to know what random cards are being tapped).
    // Let's modify schema.sql to REMOVE the foreign key constraint or make it less restrictive, because we definitely need to log unregistered card attempts (which have no corresponding user).
    // Let's drop the foreign key or modify table structure on the database.
    // Let's just drop the Foreign Key constraint on rfid_logs.
    // Yes! Let's update schema.sql on the server to drop the foreign key.
    
    // For now, let's write log_scan.php. To make it work, let's ensure we can log unregistered UIDs.
    $logStmt = $pdo->prepare("INSERT INTO rfid_logs (uid, access_status) VALUES (?, ?)");
    $logStmt->execute([$uid, $allowed ? 1 : 0]);

    echo json_encode([
        'status' => 'success',
        'allowed' => $allowed,
        'uid' => $uid,
        'name' => $name,
        'role' => $role
    ]);

} catch (Exception $e) {
    echo json_encode([
        'status' => 'error',
        'message' => 'Database error: ' . $e->getMessage()
    ]);
}
