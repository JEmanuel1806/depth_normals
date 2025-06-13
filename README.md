# Point Cloud Normal Calculation and Visualization

This project calculates surface normals for a 3D point cloud using a screen-space depth buffer and visualizes them in OpenGL. It supports both ground-truth and calculated normals and offers visual debugging tools to analyze the normal estimation process.

The system is intended as a preprocessing step for surface reconstruction, with real-time GPU-based normal estimation.

---

## Controls

| Key          | Action                                      |
|--------------|---------------------------------------------|
| `W/A/S/D`    | Move camera                                 |
| `Q/E`        | Rotate camera                               |
| `Left and Right Arrow`    | Spin point cloud left/right                 |
| `N`          | Toggle normal line visualization            |
| `I`          | Show/hide ID texture                        |
| `M`          | Show/hide normal texture                    |
| `F`          | Show/hide camera frustum                    |
| `ESC`        | Quit application                            |

---

## Input Format

- File: `.ply` format
- Required: `x`, `y`, `z` positions
- Optional:
  - `nx`, `ny`, `nz` normals 
  - `red`, `green`, `blue` for color

---

## Dependencies

| Library      | Description                                |
|--------------|--------------------------------------------|
| **GLFW**     | Cross-platform window and input handling   |
| **GLEW**     | OpenGL extension loading                   |
| **GLM**      | Matrix and vector math                     |
| **STB Easy Font** | Simple on-screen text drawing         |

All libraries are included or referenced via `includes/`.


![Point cloud rendering with normal visualization](https://github.com/user-attachments/assets/1646ecf5-057c-4413-8e58-e420620feee9)

---

## Goal

This tool provides reliable surface normals for downstream applications such as Poisson Surface Reconstruction, TSDF fusion, or real-time visualization pipelines.

---
