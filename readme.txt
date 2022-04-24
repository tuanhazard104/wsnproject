Hướng dẫn cài đặt và biên dịch dự án:
	- Yêu cầu: + cần cài đặt ArduinoIDE và các thư viện liên quan
		   + Cần 3 module esp32, 2 cho nút cảm biến và 1 cho nút gateway
	- Cách thực hiện:
		   + biên dịch file node.ino và nạp code vào nút cảm biến.
		     Cần lưu ý tên của cảm biến và sử dụng thư viện phù hợp cho cảm biến đó.
		   + biên dịch file gateway.ino và nạp code cho nút gateway.
		     Cần lưu ý nếu quá trình biên dịch gặp lỗi: "Sketch too big" thì vào Tools->Partition Scheme-> chọn mục huge app
		   + vào thư mục group4wsn, chuột phải chọn git bash here, nhập: "npm start". Chờ 1p để kết nối tới mqtt broker.
		   + đăng nhập web: mcbkdnhom4.herokuapp.com để quan sát thông số nhiệt độ, độ ẩm.
		   