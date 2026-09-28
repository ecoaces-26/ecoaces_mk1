import cv2
import time
from ultralytics import YOLO
from config import STREAM_URL

MODEL_PATH = "yolov8s.pt"
CONFIDENCE = 0.60
IMAGE_SIZE = 320


def main():
    print("Loading YOLOv8s...")
    model = YOLO(MODEL_PATH)

    while True:
        print(f"Connecting to ESP32-CAM: {STREAM_URL}")

        cap = cv2.VideoCapture(STREAM_URL)

        if not cap.isOpened():
            print("Failed to connect to ESP32-CAM.")
            print("Retrying in 5 seconds...")
            time.sleep(5)
            continue

        print("ESP32-CAM connected.")
        print("Starting YOLO inference on Raspberry Pi...")

        frame_count = 0
        fps = 0.0
        fps_start_time = time.time()

        while True:
            ret, frame = cap.read()

            if not ret:
                print("ESP32-CAM stream disconnected.")
                break

            frame_count += 1

            # YOLO inference runs on the Raspberry Pi.
            # COCO class 0 = person.
            results = model(
                frame,
                conf=CONFIDENCE,
                classes=[0],
                imgsz=IMAGE_SIZE,
                verbose=False
            )

            people_detected = 0

            for result in results:
                for box in result.boxes:
                    confidence = float(box.conf[0])

                    x1, y1, x2, y2 = map(
                        int,
                        box.xyxy[0]
                    )

                    people_detected += 1

                    cv2.rectangle(
                        frame,
                        (x1, y1),
                        (x2, y2),
                        (0, 255, 0),
                        2
                    )

                    label = f"Person {confidence:.2f}"

                    cv2.putText(
                        frame,
                        label,
                        (x1, max(y1 - 10, 10)),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        0.6,
                        (0, 255, 0),
                        2
                    )

            if frame_count % 5 == 0:
                current_time = time.time()
                elapsed = current_time - fps_start_time

                if elapsed > 0:
                    fps = 5 / elapsed

                fps_start_time = current_time

                print(
                    f"FPS: {fps:.2f} | "
                    f"People detected: {people_detected}"
                )

        cap.release()

        print("Reconnecting in 2 seconds...")
        time.sleep(2)


if __name__ == "__main__":
    main()