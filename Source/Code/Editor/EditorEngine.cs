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
        [DllImport("EditorEngine.NET.dll")]
        static extern void StartApplication();

        [DllImport("EditorEngine.NET.dll")]
        static extern void StopApplication();

        [DllImport("EditorEngine.NET.dll")]
        static extern void RunMainLoop();

        [DllImport("EditorEngine.NET.dll")]
        static extern void SetLevelRenderCanvasHandle(IntPtr LevelRenderCanvasHandle);

        [DllImport("EditorEngine.NET.dll")]
        static extern void SetAppExitFlag(bool Value);

        private IntPtr LevelRenderCanvasHandle;
        private Thread EngineThread;

        public EditorEngine(IntPtr NewLevelRenderCanvasHandle)
        {
            LevelRenderCanvasHandle = NewLevelRenderCanvasHandle;
        }

        private void EditorEngineThreadFunc()
        {
            SetLevelRenderCanvasHandle(LevelRenderCanvasHandle);
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
