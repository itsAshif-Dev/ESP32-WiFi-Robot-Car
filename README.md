# ESP32 WiFi Robot Car

## 📌 Overview

This project is a WiFi-controlled robot car built using the ESP32 microcontroller. The ESP32 creates its own WiFi hotspot, allowing users to connect directly from a smartphone or computer without requiring an external router.

The robot is controlled through a web interface hosted on the ESP32. The firmware uses non-blocking programming with `millis()` and an asynchronous web server to provide smooth motor control and responsive performance.

---

## 🚀 Features

- WiFi Access Point (AP) Mode
- Web-Based Remote Controller
- PWM Motor Speed Control
- Forward, Backward, Left and Right Movement
- Real-Time Battery Voltage Monitoring
- Battery Percentage Display
- Non-Blocking Programming using `millis()`
- Responsive Web Interface
- ESP32 + MX1508 Motor Driver

---

## 🛠 Hardware Used

- ESP32 Development Board
- MX1508 Motor Driver
- 2 DC Motors
- Robot Chassis
- Battery Pack

---

## 💻 Software Used

- Arduino IDE
- ESP32 Board Package
- ESPAsyncWebServer
- AsyncTCP

---

## 📂 Project Structure

```
ESP32-WiFi-Robot-Car
│
├── ESP32_Remote.ino
├── Footballcar.ino
├── README.md
└── Images
```

---

## ⚙️ How It Works

1. ESP32 creates its own WiFi hotspot.
2. User connects a phone or PC to the hotspot.
3. The browser opens the ESP32 web interface.
4. Button commands are sent to the ESP32.
5. ESP32 controls the motors using PWM through the MX1508 driver.
6. Battery voltage is measured and displayed on the webpage.

---

## 📚 Technologies Used

- Embedded C++
- ESP32
- WiFi Networking
- HTML
- CSS
- JavaScript
- PWM
- GPIO
- ADC

---

## 🎯 Learning Outcomes

This project helped me learn:

- ESP32 programming
- Embedded systems development
- WiFi communication
- Web server implementation
- Motor control
- Battery monitoring
- Real-time embedded programming

---

## 🔮 Future Improvements

- Obstacle Avoidance
- Line Following
- ESP-NOW Communication
- OTA Firmware Updates
- Mobile App Control
- Camera Module Support

---

## 📄 License

This project is licensed under the MIT License.
