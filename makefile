OBJ_DIR = intermediates\\

CPP = cl.exe
LINK = cl.exe /Zi /MDd /EHsc
INC = /I. /I./src /I./include
C_FLAGS = /c /Zi /MDd /EHsc /std:c++latest /Fo: $(OBJ_DIR) $(INC)
LOCAL_UTIL_LIBRARIES = user32.lib d3d12.lib dxgi.lib dxcompiler.lib

OBJS = $(OBJ_DIR)PSOBuilder.obj $(OBJ_DIR)main.obj $(OBJ_DIR)Renderer.obj $(OBJ_DIR)DescriptorHeapAllocator.obj $(OBJ_DIR)ObjectRenderer.obj $(OBJ_DIR)3DMath.obj $(OBJ_DIR)PrimitiveObject.obj $(OBJ_DIR)ShaderCompiler.obj $(OBJ_DIR)Scene.obj $(OBJ_DIR)View.obj $(OBJ_DIR)Controller.obj $(OBJ_DIR)FluidObject.obj $(OBJ_DIR)MPMSolver.obj

FluidSim: $(OBJS)
	$(LINK) /Fe: bin/FluidSim.exe $(OBJS) $(LOCAL_UTIL_LIBRARIES) $(GL_LIBRARIES)

{src\}.cpp{$(OBJ_DIR)}.obj::
	$(CPP) $(C_FLAGS) $<

{src\util\}.cpp{$(OBJ_DIR)}.obj::
	$(CPP) $(C_FLAGS) $<

{src\primitives\}.cpp{$(OBJ_DIR)}.obj::
	$(CPP) $(C_FLAGS) $<

{src\fluids\}.cpp{$(OBJ_DIR)}.obj::
	$(CPP) $(C_FLAGS) $<

$(OBJS):

clean:
	del $(OBJ_DIR)*.obj