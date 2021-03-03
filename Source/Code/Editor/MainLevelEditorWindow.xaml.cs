using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Interop;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;

namespace Editor
{
    /// <summary>
    /// Interaction logic for MainLevelEditorWindow.xaml
    /// </summary>
    public partial class MainLevelEditorWindow : Window
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

        public MainLevelEditorWindow()
        {
            InitializeComponent();            
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            MainLevelEditorWindow.SetAppExitFlag(true);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            LevelRenderCanvasHandle = this.WFHost.Child.Handle;

            Thread EngineThread = new Thread(new ThreadStart(() =>
            {
                MainLevelEditorWindow.SetLevelRenderCanvasHandle(LevelRenderCanvasHandle);

                MainLevelEditorWindow.StartApplication();

                MainLevelEditorWindow.RunMainLoop();

                MainLevelEditorWindow.StopApplication();

            }));

            EngineThread.Start();
        }
    }
}
