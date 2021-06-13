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
        public enum ClassPropertyType { Float, Vector, Rotator, Color, EntityReference, ComponentReference, ResourceReference };

        public struct Float3
        {
            public float X, Y, Z;
        }

        public struct Rotator
        {
            public float Pitch, Yaw, Roll;
        }

        public struct Color
        {
            public float R, G, B;
        }

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

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint GetEntityPropertiesCount(string EntityName);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetEntityPropertyName(string EntityName, uint PropertyIndex);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern ClassPropertyType GetEntityPropertyType(string EntityName, uint PropertyIndex);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetEntityComponentReferenceProperty(string EntityName, string PropertyName);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetComponentClassName(IntPtr Component);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint GetComponentPropertiesCount(IntPtr Component);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetComponentPropertyName(IntPtr Component, uint PropertyIndex);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern ClassPropertyType GetComponentPropertyType(IntPtr Component, uint PropertyIndex);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern Float3 GetComponentVectorProperty(IntPtr component, string PropertyName);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern Rotator GetComponentRotatorProperty(IntPtr component, string PropertyName);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern Color GetComponentColorProperty(IntPtr component, string PropertyName);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern float GetComponentFloatProperty(IntPtr component, string PropertyName);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetComponentResourceReferenceProperty(IntPtr component, string PropertyName);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetResourceName(IntPtr Resource);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void CompileShaders(string Action);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetComponentFloatProperty(IntPtr component, string PropertyName, float PropertyValue);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetComponentVectorProperty(IntPtr component, string PropertyName, Float3 PropertyValue);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetComponentRotatorProperty(IntPtr component, string PropertyName, Rotator PropertyValue);

        [DllImport("EditorEngine.NET.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetComponentColorProperty(IntPtr component, string PropertyName, Color PropertyValue);

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
