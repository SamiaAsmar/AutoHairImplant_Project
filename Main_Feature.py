import sys
import serial
import serial.tools.list_ports
from threading import Thread
from PyQt6.QtWidgets import (
    QApplication, QWidget, QLabel, QPushButton, QVBoxLayout,
    QHBoxLayout, QTextEdit, QMessageBox
)
from PyQt6.QtGui import QPixmap, QImage
from PyQt6.QtCore import QTimer, Qt
import numpy as np
import cv2
from picamera2 import Picamera2
import math

# Configuration Constants
class Config:
    REAL_WORLD_SIZE_MM = (70, 50)  # Width, height in mm (reference rectangle)
    POINT_SPACING_PX = 30      # Pixel spacing between generated points
    MIN_MARKER_AREA = 100      # Minimum contour area for marker detection
    QUADRANT_ANGLES = {
        "الربع الأول (أعلى اليمين)": -150,
        "الربع الثاني (أعلى اليسار)": -220,
        "الربع الثالث (أسفل اليسار)": -380,
        "الربع الرابع (أسفل اليمين)": -340
    }

class DoctorScreen(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Hair Transplant Targeting Assistant")

        # Camera setup
        self.picam2 = Picamera2()
        self.picam2.configure(self.picam2.create_preview_configuration(
            main={"format": 'XRGB8888', "size": (1280, 960)}))
        self.picam2.start()

        # Initialize variables
        self.frame = None
        self.captured_frame = None
        self.polygon_points = []
        self.generating = False
        self.points_inside = []
        self.H_global = None
        self.reference_points = None
        self.reference_center = None

        # Serial communication variables
        self.serial_port = None
        self.serial_thread = None
        self.serial_running = False
        self.points_to_send = []
        self.current_point_index = 0

        self.init_ui()
        self.init_timer()

    def init_ui(self):
        main_layout = QHBoxLayout()
        left_layout = QVBoxLayout()
        right_layout = QVBoxLayout()
        buttons_layout = QHBoxLayout()

        self.live_label = QLabel("Live Camera Feed")
        self.live_label.setFixedSize(1280, 960)
        self.live_label.setStyleSheet("background-color: black;")
        left_layout.addWidget(self.live_label)

        self.captured_label = QLabel("Captured Frame")
        self.captured_label.setFixedSize(1280, 960)
        self.captured_label.setStyleSheet("background-color: black;")
        self.captured_label.mousePressEvent = self.on_captured_frame_click
        right_layout.addWidget(self.captured_label)

        self.capture_btn = QPushButton("Capture Reference Points")
        self.generate_btn = QPushButton("Generate Points")
        self.send_btn = QPushButton("Send Points to Arduino")
        self.reset_btn = QPushButton("Reset")
        self.exit_btn = QPushButton("Exit")

        buttons_layout.addWidget(self.capture_btn)
        buttons_layout.addWidget(self.generate_btn)
        buttons_layout.addWidget(self.send_btn)
        buttons_layout.addWidget(self.reset_btn)
        buttons_layout.addWidget(self.exit_btn)
        right_layout.addLayout(buttons_layout)

        self.log_box = QTextEdit()
        self.log_box.setReadOnly(True)
        self.log_box.setFixedHeight(150)
        right_layout.addWidget(self.log_box)

        main_layout.addLayout(left_layout)
        main_layout.addLayout(right_layout)
        self.setLayout(main_layout)

        self.capture_btn.clicked.connect(self.capture_reference_points)
        self.generate_btn.clicked.connect(self.generate_points)
        self.send_btn.clicked.connect(self.start_sending_points)
        self.reset_btn.clicked.connect(self.reset_all)
        self.exit_btn.clicked.connect(self.close)

        self.generate_btn.setEnabled(False)
        self.send_btn.setEnabled(False)

    def init_timer(self):
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_live_feed)
        self.timer.start(30)

    def update_live_feed(self):
        try:
            self.frame = self.picam2.capture_array()
            resized_frame = cv2.resize(self.frame, (1280, 960), interpolation=cv2.INTER_AREA)
            rgb_frame = cv2.cvtColor(resized_frame, cv2.COLOR_BGR2RGB)
            h, w, ch = rgb_frame.shape
            bytes_per_line = ch * w
            qimg = QImage(rgb_frame.data, w, h, bytes_per_line, QImage.Format.Format_RGB888)
            self.live_label.setPixmap(QPixmap.fromImage(qimg))
        except Exception as e:
            self.log(f"Camera error: {str(e)}")

    def log(self, text):
        from datetime import datetime
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_box.append(f"[{timestamp}] {text}")

    def determine_quadrant(self, point):
        """Determine quadrant based on reference points center"""
        if self.reference_center is None:
            return ""
        
        x, y = point
        cx, cy = self.reference_center
        
        if x >= cx and y <= cy:
            return "الربع الأول (أعلى اليمين)"
        elif x <= cx and y <= cy:
            return "الربع الثاني (أعلى اليسار)"
        elif x <= cx and y >= cy:
            return "الربع الثالث (أسفل اليسار)"
        else:
            return "الربع الرابع (أسفل اليمين)"

    def capture_reference_points(self):
        try:
            if self.frame is None:
                raise ValueError("No frame available for detection")
            self.log("Detecting reference points...")
            found = self.detect_reference_points(self.frame)
            if not found:
                raise ValueError("Could not find 4 points of the same color")
            
            color, pts = next(iter(found.items()))
            self.log(f"Detected color: {color.upper()} → Points: {pts}")

            # Store reference points and calculate center
            self.reference_points = self.order_points(pts)
            self.reference_center = np.mean(self.reference_points, axis=0)
            self.log(f"Reference center: {self.reference_center}")

            real_pts = np.array([
                [0, 0],
                [70, 0],
                [70, 50],
                [0, 50]
            ], dtype="float32")

            self.H_global, _ = cv2.findHomography(self.reference_points, real_pts)
            self.captured_frame = self.frame.copy()
            self.polygon_points = []
            self.generating = False
            self.points_inside = []

            for pt in pts:
                cv2.circle(self.captured_frame, pt, 7, (0, 255, 255), -1)

            self.show_captured_frame()
            self.generate_btn.setEnabled(False)
            self.send_btn.setEnabled(False)
            self.log("Click on points on the captured frame to create polygon.")
        except Exception as e:
            self.log(f"Error: {str(e)}")
            QMessageBox.warning(self, "Error", str(e))

    def show_captured_frame(self):
        if self.captured_frame is None:
            return
        img = self.captured_frame.copy()
        
        # Draw reference points and center
        if self.reference_points is not None:
            for pt in self.reference_points:
                cv2.circle(img, tuple(map(int, pt)), 7, (0, 255, 255), -1)
            if self.reference_center is not None:
                cv2.circle(img, tuple(map(int, self.reference_center)), 5, (0, 0, 255), -1)
                cv2.line(img, (int(self.reference_center[0]), 0), 
                        (int(self.reference_center[0]), img.shape[0]), (0, 0, 255), 1)
                cv2.line(img, (0, int(self.reference_center[1])), 
                        (img.shape[1], int(self.reference_center[1])), (0, 0, 255), 1)
        
        if len(self.polygon_points) > 0:
            pts_np = np.array(self.polygon_points, np.int32)
            if len(self.polygon_points) > 2:
                cv2.polylines(img, [pts_np], isClosed=True, color=(0, 255, 255), thickness=2)
            for pt in self.polygon_points:
                cv2.circle(img, pt, 5, (0, 255, 255), -1)
        if self.generating:
            for i, pt in enumerate(self.points_inside):
                color = (0, 255, 0) if i == self.current_point_index else (255, 0, 0)
                cv2.circle(img, pt, 5, color, -1)
        rgb_img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        h, w, ch = rgb_img.shape
        qimg = QImage(rgb_img.data, w, h, ch * w, QImage.Format.Format_RGB888)
        self.captured_label.setPixmap(QPixmap.fromImage(qimg))

    def on_captured_frame_click(self, event):
        if self.captured_frame is None:
            return
        label_size = self.captured_label.size()
        pixmap = self.captured_label.pixmap()
        if pixmap is None:
            return
        pixmap_size = pixmap.size()
        pos = event.position()
        x_label, y_label = pos.x(), pos.y()
        scale_x = pixmap_size.width() / label_size.width()
        scale_y = pixmap_size.height() / label_size.height()
        x_img = int(x_label * scale_x)
        y_img = int(y_label * scale_y)
        
        if not self.generating:
            self.polygon_points.append((x_img, y_img))
            quadrant = self.determine_quadrant((x_img, y_img))
            self.log(f"Point added: ({x_img}, {y_img}) - {quadrant}")
            self.show_captured_frame()
            if len(self.polygon_points) >= 3 and self.H_global is not None:
                self.generate_btn.setEnabled(True)
        else:
            for i, pt in enumerate(self.points_inside):
                dist = np.linalg.norm(np.array(pt) - np.array([x_img, y_img]))
                if dist < 10:
                    pts = np.array([[[pt[0], pt[1]]]], dtype='float32')
                    real_pt = cv2.perspectiveTransform(pts, self.H_global)
                    rx, ry = real_pt[0][0]
                    quadrant = self.determine_quadrant(pt)
                    self.log(f"Point {i+1}/{len(self.points_inside)}: "
                           f"Image ({pt[0]}, {pt[1]}) → "
                           f"Real ({rx:.2f}mm, {ry:.2f}mm) - {quadrant}")
                    break

    def generate_points(self):
        if len(self.polygon_points) < 3 or self.H_global is None:
            self.log("Polygon not defined or homography missing.")
            return
        self.log("Generating points inside polygon...")
        self.points_inside = self.generate_points_inside_polygon(self.polygon_points, Config.POINT_SPACING_PX)
        self.generating = True
        
        # تجميع النقاط حسب الأرباع
        quadrant_points = {
            "الربع الأول (أعلى اليمين)": [],
            "الربع الثاني (أعلى اليسار)": [],
            "الربع الثالث (أسفل اليسار)": [],
            "الربع الرابع (أسفل اليمين)": []
        }
        
        for pt in self.points_inside:
            pts = np.array([[[pt[0], pt[1]]]], dtype='float32')
            real_pt = cv2.perspectiveTransform(pts, self.H_global)
            rx, ry = real_pt[0][0]
            quadrant = self.determine_quadrant(pt)
            quadrant_points[quadrant].append({
                'real_coords': (rx, ry),
                'image_coords': pt,
                'quadrant': quadrant
            })
        
        # ترتيب النقاط بحيث يتم معالجة كل رباع على حدة
        self.points_to_send = []
        for quadrant in quadrant_points:
            self.points_to_send.extend(quadrant_points[quadrant])
        
        self.current_point_index = 0
        self.show_captured_frame()
        self.log(f"Generated {len(self.points_inside)} points inside polygon.")
        self.send_btn.setEnabled(True)

    def generate_points_inside_polygon(self, polygon, spacing_px):
        polygon_np = np.array(polygon, dtype=np.int32)
        xs, ys = zip(*polygon)
        min_x, max_x = min(xs), max(xs)
        min_y, max_y = min(ys), max(ys)
        points = []
        
        y_values = sorted(set(range(min_y, max_y + 1, spacing_px)))
        
        for row_idx, y in enumerate(y_values):
            row_points = []
            for x in range(min_x, max_x + 1, spacing_px):
                if cv2.pointPolygonTest(polygon_np, (x, y), False) >= 0:
                    row_points.append((x, y))
            
            if row_idx % 2 == 1:
                row_points = row_points[::-1]
            
            points.extend(row_points)
        
        return points

    def init_serial_connection(self):
        try:
            ports = serial.tools.list_ports.comports()
            if not ports:
                self.log("No serial ports found!")
                return False
            
            for port in ports:
                if 'ACM' in port.device or 'USB' in port.device:
                    try:
                        self.serial_port = serial.Serial(
                            port=port.device,
                            baudrate=9600,
                            timeout=1
                        )
                        self.serial_running = True
                        self.serial_thread = Thread(target=self.serial_listener)
                        self.serial_thread.start()
                        self.log(f"Connected to serial port: {port.device}")
                        return True
                    except Exception as e:
                        continue
            
            self.log("Could not connect to any serial port!")
            return False
        except Exception as e:
            self.log(f"Serial connection error: {str(e)}")
            return False

    def serial_listener(self):
        while self.serial_running and self.serial_port:
            try:
                if self.serial_port.in_waiting:
                    line = self.serial_port.readline().decode('utf-8').strip()
                    if line == "Done":
                        self.send_next_point()
            except Exception as e:
                self.log(f"Serial read error: {str(e)}")
                break

    def start_sending_points(self):
        if not self.serial_port or not self.serial_port.is_open:
            if not self.init_serial_connection():
                QMessageBox.warning(self, "Error", "Failed to connect to Arduino!")
                return
        
        if not self.points_to_send:
            QMessageBox.warning(self, "Error", "No points to send!")
            return
        
        self.current_point_index = 0
        self.send_next_point()

    def send_next_point(self):
        if self.current_point_index < len(self.points_to_send):
            point = self.points_to_send[self.current_point_index]
            
            # إرسال زاوية الربع أولاً (فقط لأول نقطة في كل رباعي)
            if self.current_point_index == 0:
                quadrant = point['quadrant']
                angle = Config.QUADRANT_ANGLES.get(quadrant, 0)
                angle_command = f"ANGLE,{angle}\n"
                try:
                    if self.serial_port and self.serial_port.is_open:
                        self.serial_port.write(angle_command.encode('utf-8'))
                        self.log(f"Sent angle command: {angle}° for {quadrant}")
                except Exception as e:
                    self.log(f"Failed to send angle command: {str(e)}")
            
            # إرسال إحداثيات النقطة
            rx, ry = point['real_coords']
            point_command = f"{rx:.2f},{ry:.2f}\n"
            
            try:
                if self.serial_port and self.serial_port.is_open:
                    self.serial_port.write(point_command.encode('utf-8'))
                    self.log(f"Sent point {self.current_point_index+1}/{len(self.points_to_send)}: "
                           f"Real ({rx:.2f}mm, {ry:.2f}mm) - {point['quadrant']}")
                    self.current_point_index += 1
                    self.show_captured_frame()
            except Exception as e:
                self.log(f"Failed to send command: {str(e)}")
        else:
            self.log("All points have been sent to Arduino")

    def reset_all(self):
        self.serial_running = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        if self.serial_thread and self.serial_thread.is_alive():
            self.serial_thread.join()
        
        self.polygon_points = []
        self.points_inside = []
        self.points_to_send = []
        self.current_point_index = 0
        self.generating = False
        self.H_global = None
        self.captured_frame = None
        self.reference_points = None
        self.reference_center = None
        self.generate_btn.setEnabled(False)
        self.send_btn.setEnabled(False)
        
        self.log("Reset all data.")
        self.captured_label.clear()

    @staticmethod
    def detect_reference_points(frame):
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

        color_ranges = {
            "pink": ([140, 50, 50], [170, 255, 255]),           
            "orange": ([10, 100, 100], [20, 255, 255]),            
            "green": ([40, 40, 40], [80, 255, 255]),            
            "cyan": ([80, 50, 50], [100, 255, 255])
        }        

        detected_points = {}

        for color, (lower, upper) in color_ranges.items():
            lower_np = np.array(lower)
            upper_np = np.array(upper)
            mask = cv2.inRange(hsv, lower_np, upper_np)

            kernel = np.ones((5, 5), np.uint8)
            mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
            mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)

            contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            points = []
            for cnt in contours:
                area = cv2.contourArea(cnt)
                if area > Config.MIN_MARKER_AREA:
                    M = cv2.moments(cnt)
                    if M["m00"] == 0:
                        continue
                    cx = int(M["m10"] / M["m00"])
                    cy = int(M["m01"] / M["m00"])
                    points.append((cx, cy))
            if len(points) == 4:
                detected_points[color] = points

        return detected_points if detected_points else None

    @staticmethod
    def order_points(pts):
        pts = np.array(pts)
        rect = np.zeros((4, 2), dtype="float32")

        s = pts.sum(axis=1)
        rect[0] = pts[np.argmin(s)]
        rect[2] = pts[np.argmax(s)]

        diff = np.diff(pts, axis=1)
        rect[1] = pts[np.argmin(diff)]
        rect[3] = pts[np.argmax(diff)]

        return rect

app = QApplication(sys.argv)
window = DoctorScreen()
window.show()
sys.exit(app.exec())