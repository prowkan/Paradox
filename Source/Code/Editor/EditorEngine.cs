using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Runtime.InteropServices;

namespace Editor
{
    class EditorEngine
    {
        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void StartApplication();

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void StopApplication();

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RunMainLoop();

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void SetLevelRenderCanvasHandle(IntPtr LevelRenderCanvasHandle);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void SetAppExitFlag(bool Value);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void SetEditorViewportSize(uint Width, uint Height);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void RotateCamera(int MouseDeltaX, int MouseDeltaY);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void MoveCamera(bool bForward, bool bBackward, bool bLeft, bool bRight);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetEntityClassName(string EntityName);

        private IntPtr LevelRenderCanvasHandle;
        private Thread EngineThread;
        public uint EditorViewportWidth, EditorViewportHeight;

        public EditorEngine(IntPtr NewLevelRenderCanvasHandle)
        {
            LevelRenderCanvasHandle = NewLevelRenderCanvasHandle;
        }

        private void EditorEngineThreadFunc()
        {
            SetLevelRenderCanvasHandle(LevelRenderCanvasHandle);
            SetEditorViewportSize(EditorViewportWidth, EditorViewportHeight);
            StartApplication();
            RunMainLoop();
            StopApplication();
        }

        public void StartEditorEngine()
        {
            EngineThread = new Thread(new ThreadStart(EditorEngineThreadFunc));
            EngineThread.Start();
        }

        public void StopEditorEngine()
        {
            SetAppExitFlag(true);
            EngineThread.Join();
        }
    }
}
