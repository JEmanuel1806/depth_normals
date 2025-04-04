# Point Cloud Normal Calculation and Visualization

This project aims to calculate normals for a point cloud with missing normal data using the depth buffer. The calculated normals are then rendered using OpenGL for visualization.

The primary goal is to provide accurate normals that can be subsequently used in other surface reconstruction algorithms.

**Key Features:**

* **Normal Calculation:** Utilizes the depth buffer to estimate normals from the point cloud data.
* **OpenGL Rendering:** Visualizes the calculated normals through OpenGL rendering.
* **Surface Reconstruction Preparation:** Designed to provide normals suitable for input into various surface reconstruction algorithms.

**Purpose:**

This project serves as a preprocessing step to enhance point cloud data by generating reliable normal information, which is crucial for accurate surface reconstruction.

![image](https://github.com/user-attachments/assets/1646ecf5-057c-4413-8e58-e420620feee9)

## Controls

| Key      | Action                             |
|----------|------------------------------------|
| `W/A/S/D`| Move camera                        |
| `Q/E`    | Rotate camera left/right           |
| `N`      | Toggle normal line visualization   |

---

**Dependencies:**
* GLEW is used for managing OpenGL extensions across platforms. (includes/GL)
* GLFW is used for cross-platform window management. (includes/GLFW)
* GLM is used for matrix operations. (includes/glm)
* STB_IMAGE is used for texture loading and processing. (includes/.)
