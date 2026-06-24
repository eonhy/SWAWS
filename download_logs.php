<?php
require_once 'db.php';

// Set headers for CSV download
header('Content-Type: text/csv; charset=utf-8');
header('Content-Disposition: attachment; filename="rfid_access_logs_' . date('Ymd_His') . '.csv"');

// Output UTF-8 BOM for proper Korean characters display in Excel
echo "\xEF\xBB\xBF";

// Open output stream
$output = fopen('php://output', 'w');

// Set headers
fputcsv($output, ['번호(No)', '카드 UID(UID)', '이름(Name)', '역할(Role)', '상태(Status)', '태그 시간(Timestamp KST)']);

try {
    // Joint query to retrieve all logs with user names and roles
    $stmt = $pdo->query("
        SELECT 
            l.id, 
            l.uid, 
            u.name, 
            u.role, 
            l.access_status, 
            l.scanned_at 
        FROM rfid_logs l 
        LEFT JOIN rfid_users u ON l.uid = u.uid 
        ORDER BY l.scanned_at DESC
    ");
    
    while ($row = $stmt->fetch()) {
        $name = empty($row['name']) ? '미등록 사용자' : $row['name'];
        $role = empty($row['role']) ? 'Unknown' : $row['role'];
        $status = $row['access_status'] == 1 ? '승인 (Granted)' : '거부 (Denied)';
        
        // Convert timestamp to KST
        $dt = new DateTime($row['scanned_at'], new DateTimeZone('UTC'));
        $dt->setTimezone(new DateTimeZone('Asia/Seoul'));
        $timeKST = $dt->format('Y-m-d H:i:s');
        
        fputcsv($output, [
            $row['id'],
            $row['uid'],
            $name,
            $role,
            $status,
            $timeKST
        ]);
    }
} catch (Exception $e) {
    fputcsv($output, ['Error generating logs', $e->getMessage()]);
}

fclose($output);
exit;
