-- Database: lampdb

-- Drop tables if they exist (for clean setup)
DROP TABLE IF EXISTS rfid_logs;
DROP TABLE IF EXISTS rfid_users;

-- Create Users Table
CREATE TABLE rfid_users (
  id INT AUTO_INCREMENT PRIMARY KEY,
  uid VARCHAR(20) UNIQUE NOT NULL,
  name VARCHAR(50) NOT NULL,
  role VARCHAR(30) DEFAULT '임직원',
  phone VARCHAR(20) DEFAULT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Create Access Logs Table
CREATE TABLE rfid_logs (
  id INT AUTO_INCREMENT PRIMARY KEY,
  uid VARCHAR(20) NOT NULL,
  access_status TINYINT(1) NOT NULL, -- 1 = Granted, 0 = Denied
  scanned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Insert Seed Users
INSERT INTO rfid_users (uid, name, role, phone) VALUES 
('4E:1A:D8:3C', '홍길동', '관리자', '010-1234-5678'),
('23:B8:F1:C0', '김철수', '연구원', '010-9876-5432'),
('8C:5D:2A:B9', '이영희', '임직원', '010-5555-1234'),
('FA:6E:30:17', '스미스', '협력업체', '010-1111-2222');
