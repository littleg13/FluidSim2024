# D3D12 MPM Fluid Simulation

A high-performance Material Point Method (MPM) fluid simulation rendered with Direct3D 12.

# Features and Goals

## Current Features

- Material Point Method (MPM) Simulation
  - Fluid object simulation using neo-hookean hyper-elastic solid physics
  - Physics handled entirely by compute shader passes with no CPU readback
- Primitive Rendering
  - Basic shapes: spheres, cubes, and planes
- Real-time Shader Debugging
  - Hot-reloading of shaders
  - Automatic reload on file save
  - Manual reload via keybind

## Future Goals

- Expand MPM simulation capabilities
  - Convert to proper fluid dynamics
- Enhance rendering capabilities
  - Add proper fluid rendering
- Improve user interface

 ## Prerequisites

- Windows 10 or later
- Visual Studio 2019 or later with C++ desktop development workload
- DirectX 12 compatible graphics card
- DirectX SDK (June 2010 or later)

## Installation

1. Clone the repository:
2. Navigate to the project directory:
3. Download the required [DX compiler DLLs](https://github.com/microsoft/DirectXShaderCompiler/releases) and place them in the `bin/` folder.

# Building

1. Open a command prompt in the project directory.
2. Run the following command: `nmake`
3. The build process will:
    - Place intermediate files in the `intermediates/` folder
    - Generate the executable at `bin/FluidSim.exe`


## Usage

After building the project:

1. Run the executable at `bin/FluidSim.exe`.
2. Use the following controls:
- WASD: Camera movement
- Mouse: Look around
- F5: Reload shaders
- ESC: Exit the application

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

## Acknowledgments

- [MPM Guide](https://nialltl.neocities.org/articles/mpm_guide) for the initial MPM implementation
- [Taichi MPM 88 Lines](https://github.com/yuanming-hu/taichi_mpm) for the initial MPM implementation inspiration
- [DirectX-Graphics-Samples](https://github.com/Microsoft/DirectX-Graphics-Samples) for DirectX 12 examples