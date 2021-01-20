glslc.exe shader.vert -o bin/shader.vert.spv
glslc.exe shader.frag -o bin/shader.frag.spv

glslc.exe skybox.vert -o bin/skybox.vert.spv
glslc.exe skybox.frag -o bin/skybox.frag.spv

glslc.exe grid.vert -o bin/grid.vert.spv
glslc.exe grid.frag -o bin/grid.frag.spv

glslc.exe gizmo.vert -o bin/gizmo.vert.spv
glslc.exe gizmo.frag -o bin/gizmo.frag.spv

pause