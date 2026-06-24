<?php
require_once 'db.php';

header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');

try {
    // 1. Get total registered users
    $regQuery = $pdo->query("SELECT COUNT(*) as count FROM rfid_users");
    $registeredCount = $regQuery->fetch()['count'];

    // 2. Get today's counts (granted and denied)
    // Using KST timezone. Since server might be in UTC, we compare using DATE(CONVERT_TZ(scanned_at, '+00:00', '+09:00')) or simple date string comparisons.
    // Let's get today's date in local KST timezone (which is UTC+9)
    $timezone = new DateTimeZone('Asia/Seoul');
    $todayStart = new DateTime('today', $timezone);
    $todayEnd = new DateTime('tomorrow', $timezone);

    // Convert DateTime to UTC strings for DB query if DB stores in UTC.
    // However, MariaDB TIMESTAMP stores in UTC but displays/queries in server timezone.
    // To be perfectly robust across system timezone settings, let's query with UTC bounds.
    $todayStartUTC = $todayStart->setTimezone(new DateTimeZone('UTC'))->format('Y-m-d H:i:s');
    $todayEndUTC = $todayEnd->setTimezone(new DateTimeZone('UTC'))->format('Y-m-d H:i:s');

    // Granted scans today
    $grantedStmt = $pdo->prepare("SELECT COUNT(*) as count FROM rfid_logs WHERE access_status = 1 AND scanned_at >= ? AND scanned_at < ?");
    $grantedStmt->execute([$todayStartUTC, $todayEndUTC]);
    $grantedToday = $grantedStmt->fetch()['count'];

    // Denied scans today
    $deniedStmt = $pdo->prepare("SELECT COUNT(*) as count FROM rfid_logs WHERE access_status = 0 AND scanned_at >= ? AND scanned_at < ?");
    $deniedStmt->execute([$todayStartUTC, $todayEndUTC]);
    $deniedToday = $deniedStmt->fetch()['count'];

    // 3. Get recent 20 logs with user info
    $logsQuery = $pdo->query("
        SELECT 
            l.uid, 
            l.access_status, 
            l.scanned_at, 
            u.name, 
            u.role 
        FROM rfid_logs l 
        LEFT JOIN rfid_users u ON l.uid = u.uid 
        ORDER BY l.scanned_at DESC 
        LIMIT 20
    ");
    $recentLogs = $logsQuery->fetchAll();

    // Map scanned_at UTC time back to Seoul Time (KST) for frontend display if needed, 
    // or return formatted date/time strings in KST.
    foreach ($recentLogs as &$log) {
        if (empty($log['name'])) {
            $log['name'] = '미등록 사용자';
            $log['role'] = 'Unknown';
        }
        
        // Convert scanned_at to KST timezone
        $dt = new DateTime($log['scanned_at'], new DateTimeZone('UTC'));
        $dt->setTimezone(new DateTimeZone('Asia/Seoul'));
        $log['time_kst'] = $dt->format('H:i:s');
        $log['date_kst'] = $dt->format('Y.m.d');
    }

    // 4. Get all registered users
    $usersQuery = $pdo->query("SELECT name, uid, role, phone FROM rfid_users ORDER BY name ASC");
    $users = $usersQuery->fetchAll();

    echo json_encode([
        'status' => 'success',
        'registered_count' => (int)$registeredCount,
        'granted_today' => (int)$grantedToday,
        'denied_today' => (int)$deniedToday,
        'recent_logs' => $recentLogs,
        'users' => $users
    ]);

} catch (Exception $e) {
    echo json_encode([
        'status' => 'error',
        'message' => 'Database error: ' . $e->getMessage()
    ]);
}
