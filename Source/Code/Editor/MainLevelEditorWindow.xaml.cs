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
    }
}
