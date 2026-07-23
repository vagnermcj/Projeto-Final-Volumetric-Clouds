# Projeto Final Volumetric Clouds

# Projeto Final - Volumetric Clouds

Renderizador de nuvens volumétricas em tempo real, feito em C++ com OpenGL. O projeto implementa a técnica de ray marching sobre texturas 3D de ruído (Perlin-Worley) para gerar nuvens dinâmicas, baseado na abordagem descrita por Andrew Schneider em *"The Real-Time Volumetric Cloudscapes of Horizon: Zero Dawn"* (SIGGRAPH 2015).

## Funcionalidades

- Ray marching de nuvens volumétricas em tempo real (`RayMarch.vert/frag`)
- Geração procedural de ruído 3D (Perlin, Worley e Perlin-Worley) via compute shaders (`NoiseCompute.glsl`)
- Mapa de clima (weather map) gerado em compute shader, controlando cobertura, altura e tipo de nuvem (`WeatherCompute.glsl`)
- Visualização de fatias 2D/3D das texturas de ruído geradas (`SlicePreview.frag`, `SlicePreview2D.frag`)
- Terreno com textura e normal map (`Terrain.vert/frag`)
- Skybox configurável (presets: MidDay, Mountains, Nublado, default)
- Sistema de presets de cenário (vento, atmosfera, densidade, cobertura) — ex.: Cumulus MidDay, Stratocumulus, Stratus, Sunny, Sunset
- Interface de controle em tempo real com ImGui (branch docking)
- Câmera livre (estilo free-fly)

## Tecnologias

- C++ / C
- OpenGL 4.3+ (compute shaders e SSBO obrigatórios; shaders de render em GLSL 410)
- GLFW (janela e input)
- GLAD (loader de OpenGL)
- GLM (matemática)
- Dear ImGui (branch `docking`, incluído como submódulo)
- stb (carregamento de imagens)
- Visual Studio 2022 (solution/.vcxproj, toolset v145)

## Requisitos

- Windows com Visual Studio 2022
- GPU com suporte a OpenGL 4.3 ou superior (necessário para compute shaders)

## Como compilar e rodar

1. Clone o repositório com os submódulos:
   ```
   git clone --recursive https://github.com/vagnermcj/Projeto-Final-Volumetric-Clouds.git
   ```
   Se já tiver clonado sem `--recursive`, rode:
   ```
   git submodule update --init --recursive
   ```
2. Abra `VolumetricClouds/VolumetricClouds.sln` no Visual Studio.
3. Compile em `Debug|x64` ou `Release|x64`.
4. Rode com o diretório de trabalho apontando para a raiz do projeto (`VolumetricClouds/VolumetricClouds`), pois os shaders, skyboxes, terrains e presets são carregados por caminho relativo.

## Controles

- `W A S D` — movimento da câmera
- `Espaço` / `Ctrl` — sobe / desce
- `Shift` — aumenta velocidade de movimento
- Mouse — olhar em volta (clique para capturar o cursor)

## Estrutura do projeto

```
VolumetricClouds/
├── VolumetricClouds/
│   ├── main.cpp              # loop principal, setup de framebuffers e texturas
│   ├── Camera.*               # câmera free-fly
│   ├── PresetManager.*        # carregamento/gerenciamento de presets de cenário
│   ├── PerlinNoise3D.*        # geração de ruído Perlin
│   ├── WorleyNoise3D.*        # geração de ruído Worley
│   ├── Skybox.h                # renderização do céu
│   ├── Terrain.vert/frag       # terreno
│   ├── RayMarch.vert/frag      # ray marching das nuvens
│   ├── NoiseCompute.glsl       # compute shader de geração de ruído 3D
│   ├── WeatherCompute.glsl     # compute shader do weather map
│   ├── presets/                # arquivos .preset com cenários prontos
│   ├── skyboxes/                # texturas de céu
│   ├── terrains/                # modelo e texturas do terreno
│   └── imgui/                   # submódulo (Dear ImGui, branch docking)
```

## Referência

- Schneider, A. *The Real-Time Volumetric Cloudscapes of Horizon: Zero Dawn*, SIGGRAPH 2015.
