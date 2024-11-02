

import cv2
import numpy as np
import time
import os
import struct
import json
from pycoral.adapters.common import input_size
from pycoral.adapters.classify import get_classes
from pycoral.adapters.detect import BBox
from pycoral.adapters.detect import get_objects
from pycoral.adapters import common
from pycoral.utils.edgetpu import make_interpreter
import subprocess



# Function to read and preprocess images for inference
def read_inference_image(image, img_size=(256, 256), normalize=False):
    try:
        if image is None:
            raise ValueError("Error: Failed to load image")

        # Calculate aspect ratio
        h, w, _ = image.shape
        target_h, target_w = img_size

        if h > w:
            start_h = (h - w) // 2
            image = image[start_h:start_h + w, :, :]
        else:
            start_w = (w - h) // 2
            image = image[:, start_w:start_w + h, :]

        image = cv2.resize(image, (target_w, target_h))

        if normalize:
            image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
            image = image.astype(np.float32) / 255.0

        return image
    except Exception as e:
        print(f"Exception in reading image: {e}")
        return None

# Function to perform inference on a single image
def inference(interpreter, img, output_path, img_size, index, folder_name):
    original_image_path = os.path.join(output_path, folder_name, "originalCropped")
    os.makedirs(original_image_path, exist_ok=True)
    cv2.imwrite(os.path.join(original_image_path, f"original_image_{index}.jpg"), img)

    img_infer = read_inference_image(img, img_size, normalize=True)
    if img_infer is None:
        print(f"Skipping inference for image index {index} due to preprocessing error.")
        return

    img_infer_expanded = np.expand_dims(img_infer, axis=0)
    common.set_input(interpreter, img_infer_expanded)
    interpreter.invoke()

    output_tensor = common.output_tensor(interpreter, 0)
    output_image = np.squeeze(output_tensor)
    output_image = (output_image * 255).astype(np.uint8)
    output_image = cv2.cvtColor(output_image, cv2.COLOR_RGB2BGR)

    result_image_path = os.path.join(output_path, folder_name, "inferenceResult")
    os.makedirs(result_image_path, exist_ok=True)
    output_image_path = os.path.join(result_image_path, f"inference_result_{index}.jpg")
    cv2.imwrite(output_image_path, output_image)

    print(f"Inference result {index} saved at {output_image_path}")

# Function to run inference on all images in day and night folders
def run_inference_on_folder(interpreter, folder_name, output_path, img_size):
    folder_path = os.path.join(".", folder_name)  # Assuming day and night folders are in the current directory
    images = [img for img in os.listdir(folder_path) if img.endswith(".jpg")]
    
    for index, image_name in enumerate(images):
        image_path = os.path.join(folder_path, image_name)
        image = cv2.imread(image_path)
        
        if image is not None:
            inference(interpreter, image, output_path, img_size, index, folder_name)
        else:
            print(f"Failed to read image: {image_path}")

if __name__ == "__main__":
    model_path = "./pretrained_model/model.tflite"
    output_path = "./output"
    img_size = [256, 256]

    try:
        interpreter = make_interpreter(f"{model_path}")
        interpreter.allocate_tensors()
        print("Model loaded and tensors allocated successfully.")
    except Exception as e:
        print(f"Failed to load model: {e}")
        exit(1)

    # Run inference on 'day' and 'night' folders
    for folder in ["day", "night"]:
        run_inference_on_folder(interpreter, folder, output_path, img_size)

