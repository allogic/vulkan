glslangValidator -I"." -V -g -Od --target-env spirv1.6 -o debug/line.vert.spv debug/line.vert
glslangValidator -I"." -V -g -Od --target-env spirv1.6 -o debug/line.frag.spv debug/line.frag

glslangValidator -I"." -V -g -Od --target-env spirv1.6 -o vdb/soft_tracer.vert.spv vdb/soft_tracer.vert
glslangValidator -I"." -V -g -Od --target-env spirv1.6 -o vdb/soft_tracer.frag.spv vdb/soft_tracer.frag

glslangValidator -I"." -V -g -Od --target-env spirv1.6 -o vdb/mesh_renderer.task.spv vdb/mesh_renderer.task
glslangValidator -I"." -V -g -Od --target-env spirv1.6 -o vdb/mesh_renderer.mesh.spv vdb/mesh_renderer.mesh
glslangValidator -I"." -V -g -Od --target-env spirv1.6 -o vdb/mesh_renderer.frag.spv vdb/mesh_renderer.frag

glslangValidator -I"." -V -g -Od --target-env spirv1.6 -o vdb/world_generator.comp.spv vdb/world_generator.comp
glslangValidator -I"." -V -g -Od --target-env spirv1.6 -o vdb/lod_generator.comp.spv vdb/lod_generator.comp