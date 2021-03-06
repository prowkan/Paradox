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
        EditorEngine editorEngine;

        public MainLevelEditorWindow()
        {
            InitializeComponent();
            
            for (int i = 0; i < 20000; i++)
            {
                var EntityItem = new ListBoxItem();
                EntityItem.Content = "StaticMeshEntity_" + i;
                LevelEnitiesList.Items.Add(EntityItem);
            }

            for (int i = 0; i < 10000; i++)
            {
                var EntityItem = new ListBoxItem();
                EntityItem.Content = "PointLightEntity_" + i;
                LevelEnitiesList.Items.Add(EntityItem);
            }
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            editorEngine.StopEditorEngine();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            editorEngine = new EditorEngine(this.WFHost.Child.Handle);
            editorEngine.EditorViewportWidth = (uint)this.WFHost.Child.Width;
            editorEngine.EditorViewportHeight = (uint)this.WFHost.Child.Height;
            editorEngine.StartEditorEngine();
        }

        [DllImport("user32.dll")]
        private extern static int ShowCursor(bool bShow);

        [DllImport("user32.dll")]
        private extern static bool SetCursorPos(int X, int Y);

        private void LevelEnitiesList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

        }

        private int X, Y;
        private bool bIsMouseCaptured;

        private void LevelRenderPanel_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                bIsMouseCaptured = true;
                ShowCursor(false);

                var Pnt = WFHost.Child.PointToScreen(new System.Drawing.Point(e.X, e.Y));

                X = Pnt.X;
                Y = Pnt.Y;                
            }
        }

        private void LevelRenderPanel_MouseUp(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                bIsMouseCaptured = false;
                ShowCursor(true);
            }
        }

        private void LevelRenderPanel_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (bIsMouseCaptured)
            {
                var Pnt = WFHost.Child.PointToScreen(new System.Drawing.Point(e.X, e.Y));

                EditorEngine.RotateCamera(Pnt.X - X, Pnt.Y - Y);

                SetCursorPos(X, Y);
            }
        }

        private bool bForward, bBackward, bLeft, bRight;

        private void WFHost_KeyUp(object sender, System.Windows.Input.KeyEventArgs e)
        {
            if (e.Key == Key.W) bForward = false;
            if (e.Key == Key.S) bBackward = false;
            if (e.Key == Key.A) bLeft = false;
            if (e.Key == Key.D) bRight = false;           
        }

        private void WFHost_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            if (e.Key == Key.W) bForward = true;
            if (e.Key == Key.S) bBackward = true;
            if (e.Key == Key.A) bLeft = true;
            if (e.Key == Key.D) bRight = true;

            EditorEngine.MoveCamera(bForward, bBackward, bLeft, bRight);
        }

        private void LevelRenderPanel_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            
        }

        private void LevelRenderPanel_KeyUp(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            
        }
    }
}
