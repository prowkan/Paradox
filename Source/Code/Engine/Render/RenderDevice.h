#pragma once

struct RenderMesh {};
struct RenderTexture {};
struct RenderMaterial {};

struct RenderMeshCreateInfo;
struct RenderTextureCreateInfo;
struct RenderMaterialCreateInfo;

class RenderDevice
{
	public:

		virtual void InitDevice() = 0;
		virtual void ShutdownDevice() = 0;
		virtual void TickDevice(float DeltaTime) = 0;

		virtual RenderMesh* CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo) = 0;
		virtual RenderTexture* CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo) = 0;
		virtual RenderMaterial* CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo) = 0;

		virtual void DestroyRenderMesh(RenderMesh* renderMesh) = 0;
		virtual void DestroyRenderTexture(RenderTexture* renderTexture) = 0;
		virtual void DestroyRenderMaterial(RenderMaterial* renderMaterial) = 0;

	#if WITH_EDITOR
		void SetEditorViewportSize(const UINT Width, const UINT Height)
		{
			EditorViewportWidth = Width;
			EditorViewportHeight = Height;
		}
	#endif

	protected:

	#if WITH_EDITOR
		UINT EditorViewportWidth;
		UINT EditorViewportHeight;
	#endif

	private:

};