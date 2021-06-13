// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

namespace Editor
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        enum CompileShadersAction { Compile, Clean };

        private void Application_Startup(object sender, StartupEventArgs e)
        {
            bool CompileShaders = false;
            CompileShadersAction Action = 0;

            for (int i = 0; i < e.Args.Length; i++)
            {
                if (e.Args[i] == "CompileShaders") CompileShaders = true;
                if (CompileShaders && e.Args[i] == "Compile") Action = CompileShadersAction.Compile;
                if (CompileShaders && e.Args[i] == "Clean") Action = CompileShadersAction.Clean;
            }

            if (CompileShaders)
            {
                string ActionString = "";

                if (Action == CompileShadersAction.Compile) ActionString = "Compile";
                if (Action == CompileShadersAction.Clean) ActionString = "Clean";

                EditorEngine.CompileShaders(ActionString);
                Current.Shutdown(0);
                return;
            }

            MainLevelEditorWindow mainLevelEditorWindow = new MainLevelEditorWindow();
            mainLevelEditorWindow.Show();
        }
    }
}
