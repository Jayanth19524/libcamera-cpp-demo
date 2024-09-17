import cv2
import numpy as np
import time

def calculate_blue_intensity(image):
    """Calculate the blue intensity of the image."""
    blue_channel = image[:, :, 0]
    return np.sum(blue_channel)

def main():
    # Initialize variables
    top_frames = [(None, -1.0) for _ in range(5)]  # (image, blue_sum)
    cap = cv2.VideoCapture(0)  # Change to your camera source if necessary

    if not cap.isOpened():
        print("Error: Could not open camera.")
        return

    start_time = time.time()
    while True:
        current_time = time.time()
        if (current_time - start_time) >= 30:
            break  # Exit loop after 30 seconds

        ret, frame = cap.read()
        if not ret:
            print("Error: Could not read frame.")
            continue

        blue_sum = calculate_blue_intensity(frame)

        # Find the index of the minimum blue intensity frame
        min_index = min(range(5), key=lambda i: top_frames[i][1])

        # Replace the frame if the new blue_sum is larger
        if blue_sum > top_frames[min_index][1]:
            top_frames[min_index] = (frame, blue_sum)

        # Display the frame
        cv2.imshow("Camera Feed", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

    # Save the top 5 frames with the highest blue content as jpg
    for i, (img, blue_sum) in enumerate(top_frames):
        if img is not None:
            filename = f"blue_frame_{i + 1}.jpg"
            cv2.imwrite(filename, img)
            print(f"Saved {filename} with blue intensity: {blue_sum}")

if __name__ == "__main__":
    main()
