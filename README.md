﻿# Point Cloud Normal Calculation and Visualization 

This project calculates **surface normals** for a 3D point cloud using a **screen‑space depth buffer** and visualizes them in OpenGL. It supports both **ground‑truth** and **calculated** normals and offers **visual debugging tools** to analyze and compare the normal estimation.

The system is intended as a preprocessing step for **surface reconstruction** (e.g., Poisson) and for **real‑time** visualization pipelines.


---

## Controls

| Key / Combo          | Action                                                   |
| -------------------- | -------------------------------------------------------- |
| `W/A/S/D`            | Move camera                                              |
| `Q/E`                | Rotate camera (roll/pitch, depending on your binding)    |
| `Left / Right Arrow` | Spin point cloud left / right                            |
| `Mouse Drag`         | Orbit / look around                                      |
| `N`                  | Toggle normal line visualization                         |
| `I`                  | Toggle **ID texture** overlay                            |
| `O`                  | Show/Hide **ID map** (alternative/extended ID view)      |
| `M`                  | Toggle **normal texture** debug overlay                  |
| `F`                  | Toggle camera frustum                                    |
| `Ctrl/Strg`          | **Hide points** (toggle visibility)                      |
| `Ctrl/Strg + S`      | **Export PLY** (current point cloud with normals/colors) |
| `ESC`                | Quit                                                     |

---

## Input

* **File:** `.ply`
* **Required:** `x, y, z` positions
* **Optional:** `nx, ny, nz` normals, `red, green, blue` color

Ground‑truth normals (if present) are used for comparison; otherwise only estimated normals are shown.

---

## Build & Dependencies

| Library           | Purpose                  |
| ----------------- | ------------------------ |
| **GLFW**          | Window + input           |
| **GLEW**          | OpenGL extension loading |
| **GLM**           | Math (matrices/vectors)  |
| **STB Easy Font** | Simple on‑screen text    |

* Requires **OpenGL 4.5** (compute shaders).
* Optional: `GL_NV_shader_atomic_float` (or vendor equivalent) if you use float atomics; otherwise use workgroup/shared‑memory reduction.

---

## Quick Overview

1. **Depth + ID Pass (small splats)** → linearized depth + stable per‑pixel IDs
2. **Screen‑Space Derivatives** → edge‑aware dx/dy using ID match + threshold
3. **View‑space Normal** from cross(dy, dx), then **world‑space** via inverse‑transpose(view)
4. **Per‑Point Accumulation** in SSBO: sum + counter, NaN/denorm guards
5. **Finalize Normals** per point (normalize sum/counter). Optional temporal smoothing
6. **GT Comparison**: compute angle (flip‑tolerant optional), store for visualization
7. **Visualization**: large splats; color by error angle (or show lines/textures/ID maps)

<img width="1895" height="1065" alt="image" src="https://github.com/user-attachments/assets/b402ae80-85f0-4db3-8323-fdd1e0a4a9f5" />



## Goal

Provide reliable surface normals for downstream applications (e.g., **Poisson Surface Reconstruction**) and robust real‑time debugging/visualization tools for point‑cloud pipelines.
